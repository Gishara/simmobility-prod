//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "Driver.hpp"
#include "DriverFacets.hpp"

#include "buffering/BufferedDataManager.hpp"
#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "entities/AuraManager.hpp"
#include "entities/Person.hpp"
#include "entities/UpdateParams.hpp"
#include "entities/misc/TripChain.hpp"
#include "entities/roles/driver/BusDriver.hpp"
#include "entities/roles/pedestrian/Pedestrian2.hpp"
#include "logging/Log.hpp"
#include "partitions/PartitionManager.hpp"
#include "util/DebugFlags.hpp"
#include "util/DynamicVector.hpp"
#include "util/GeomHelpers.hpp"
#include "util/ReactionTimeDistributions.hpp"

#ifndef SIMMOB_DISABLE_MPI
#include "partitions/PackageUtils.hpp"
#include "partitions/UnPackageUtils.hpp"
#include "partitions/ParitionDebugOutput.hpp"
#endif

using namespace sim_mob;
using std::max;
using std::vector;
using std::set;
using std::map;
using std::string;
using std::endl;

Driver::Driver(Person* parent, MutexStrategy mtxStrat, DriverBehavior* behavior, DriverMovement* movement, Role::type roleType_, std::string roleName_) :
Role(behavior, movement, parent, roleName_, roleType_), currLane_(mtxStrat, NULL), currTurning_(mtxStrat, NULL), expectedTurning_(mtxStrat, NULL),
distCoveredOnCurrWayPt_(mtxStrat, 0), isInIntersection_(mtxStrat, false), latMovement_(mtxStrat, 0), fwdVelocity_(mtxStrat, 0), latVelocity_(mtxStrat, 0),
fwdAccel_(mtxStrat, 0), turningDirection_(mtxStrat, LANE_CHANGE_TO_NONE), vehicle(NULL), isVehicleInLoadingQueue(true), isVehiclePositionDefined(false),
distToIntersection_(mtxStrat, -1), perceivedAccOfFwdCar(NULL), perceivedDistToFwdCar(NULL),
perceivedDistToTrafficSignal(NULL), perceivedFwdAcc(NULL), perceivedFwdVel(NULL), perceivedTrafficColor(NULL), perceivedVelOfFwdCar(NULL),
yieldingToInIntersection(false)
{
	getParams().driver = this;
}

Driver::~Driver()
{
	safe_delete_item(perceivedFwdVel);
	safe_delete_item(perceivedFwdAcc);
	safe_delete_item(perceivedVelOfFwdCar);
	safe_delete_item(perceivedAccOfFwdCar);
	safe_delete_item(perceivedDistToFwdCar);
}

const Driver* Driver::getYieldingToDriver() const
{
	return yieldingToDriver;
}

void Driver::setYieldingToDriver(const Driver* driver)
{
	yieldingToDriver = driver;
}

const Lane* Driver::getCurrLane() const
{
	return currLane_.get();
}

void Driver::setYieldingToInIntersection(int driverId)
{
	yieldingToInIntersection = driverId;
}

double Driver::getDistToIntersection() const
{
	return distToIntersection_.get();
}

int Driver::getYieldingToInIntersection() const
{
	return yieldingToInIntersection;
}

void Driver::setCurrPosition(Point currPosition)
{
	currPos = currPosition;
}

const Point& Driver::getCurrPosition() const
{
	return currPos;
}

const double Driver::getFwdVelocity() const
{
	return fwdVelocity_.get();
}

const double Driver::getFwdAcceleration() const
{
	return fwdAccel_.get();
}

bool Driver::IsBusDriver()
{
	return isBusDriver;
}

Role* Driver::clone(Person *parent) const
{
	DriverBehavior* behavior = new DriverBehavior(parent);
	DriverMovement* movement = new DriverMovement(parent);
	Driver* driver = new Driver(parent, parent->getMutexStrategy(), behavior, movement);

	behavior->setParentDriver(driver);
	movement->setParentDriver(driver);
	movement->init();

	return driver;
}

void Driver::initReactionTime()
{
	DriverMovement* movement = dynamic_cast<DriverMovement*> (movementFacet);
	
	if (movement)
	{
		reactionTime = movement->getCarFollowModel()->nextPerceptionSize * 1000;
	}

	perceivedFwdVel = new FixedDelayed<double>(reactionTime, true);
	perceivedFwdAcc = new FixedDelayed<double>(reactionTime, true);
	perceivedVelOfFwdCar = new FixedDelayed<double>(reactionTime, true);
	perceivedAccOfFwdCar = new FixedDelayed<double>(reactionTime, true);
	perceivedDistToFwdCar = new FixedDelayed<double>(reactionTime, true);
	perceivedDistToTrafficSignal = new FixedDelayed<double>(reactionTime, true);
	perceivedTrafficColor = new FixedDelayed<TrafficColor>(reactionTime, true);
}

void Driver::make_frame_tick_params(timeslice now)
{
	getParams().reset(now, *this);
}

vector<BufferedBase*> Driver::getSubscriptionParams()
{
	vector<BufferedBase*> res;

	res.push_back(&(currLane_));
	res.push_back(&(currTurning_));
	res.push_back(&(expectedTurning_));
	res.push_back(&(distCoveredOnCurrWayPt_));
	res.push_back(&(distCoveredOnCurrWayPt_));
	res.push_back(&(distToIntersection_));
	res.push_back(&(isInIntersection_));
	res.push_back(&(latMovement_));
	res.push_back(&(latMovement_));
	res.push_back(&(fwdVelocity_));
	res.push_back(&(latVelocity_));
	res.push_back(&(fwdAccel_));
	res.push_back(&(turningDirection_));

	return res;
}

void Driver::onParentEvent(event::EventId eventId, event::Context ctxId, event::EventPublisher *sender, const event::EventArgs &args)
{
	if (eventId == event::EVT_AMOD_REROUTING_REQUEST_WITH_PATH)
	{
		AMOD::AMODEventPublisher* pub = (AMOD::AMODEventPublisher*) sender;
		const AMOD::AMODRerouteEventArgs& rrArgs = MSG_CAST(AMOD::AMODRerouteEventArgs, args);
		std::cout << "driver get reroute event <" << rrArgs.reRoutePath.size() << "> from <" << pub->id << ">" << std::endl;

		rerouteWithPath(rrArgs.reRoutePath);
	}
}

void Driver::handleUpdateRequest(MovementFacet *mFacet)
{
	if (this->isVehicleInLoadingQueue == false)
	{
		mFacet->updateNearbyAgent(this->getParent(), this);
	}
}

double Driver::gapDistance(const Driver *front)
{
	double headway = Math::FLT_INF;
	DriverMovement* mov = dynamic_cast<DriverMovement*> (Movement());

	//Check if the vehicle in front is valid
	if (front)
	{
		DriverMovement* frontMov = dynamic_cast<DriverMovement*> (front->Movement());

		//Check if the vehicle in front has arrived at its destination
		if (!frontMov->fwdDriverMovement.isDoneWithEntireRoute())
		{
			//Check if the driver in front is on the same way point as us
			if (mov->fwdDriverMovement.getCurrWayPoint() == frontMov->fwdDriverMovement.getCurrWayPoint())
			{
				headway = mov->fwdDriverMovement.getDistToEndOfCurrWayPt() - frontMov->fwdDriverMovement.getDistToEndOfCurrWayPt() - front->getVehicleLength();
			}
			else
			{
				headway = mov->fwdDriverMovement.getDistToEndOfCurrWayPt() + frontMov->fwdDriverMovement.getDistCoveredOnCurrWayPt() - front->getVehicleLength();
			}
		}
	}

	return headway;
}

void DriverUpdateParams::reset(timeslice now, const Driver& owner)
{
	UpdateParams::reset(now);

	//Set to the previous known buffered values
	currLane = owner.getCurrLane();
	
	if (currLane)
	{
		currLaneIndex = currLane->getLaneIndex();
	}

	nextLaneIndex = currLaneIndex;

	//Current lanes to the left and right. May be null
	leftLane = NULL;
	rightLane = NULL;
	leftLane2 = NULL;
	rightLane2 = NULL;

	//Reset. These will be set before they are used; the values here represent either default
	//values or are unimportant.
	currSpeed = 0;
	perceivedFwdVelocity = 0;
	perceivedLatVelocity = 0;
	trafficColor = Green;
	elapsedSeconds = ConfigManager::GetInstance().FullConfig().baseGranMS() / 1000.0;
	perceivedFwdVelocityOfFwdCar = 0;
	perceivedLatVelocityOfFwdCar = 0;
	perceivedAccelerationOfFwdCar = 0;

	//Lateral velocity of lane changing.
	laneChangingVelocity = 100;

	//Space to next car
	space = 0;

	//the acceleration of leading vehicle
	a_lead = 0;

	//the speed of leading vehicle
	v_lead = 0;

	//the distance which leading vehicle will move in next time step
	space_star = 0;

	distanceToNormalStop = 0;

	//distance to where critical location where lane changing has to be made
	distToStop = 0;

	//Set to true if we have just moved to a new segment.
	justChangedToNewSegment = false;

	//Set to true if we have just moved into an intersection.
	justMovedIntoIntersection = false;

	//If we've just moved into an intersection, is set to the amount of overflow (e.g.,
	//  how far into it we already are.)
	overflowIntoIntersection = 0;

	turningDirection = LANE_CHANGE_TO_NONE;

	nvFwd.distance = maxVisibleDis;
	nvFwd = NearestVehicle();
	nvLeftFwd = NearestVehicle();
	nvRightFwd = NearestVehicle();
	nvBack = NearestVehicle();
	nvLeftBack = NearestVehicle();
	nvRightBack = NearestVehicle();
	nvLeftFwd2 = NearestVehicle();
	nvLeftBack2 = NearestVehicle();
	nvRightFwd2 = NearestVehicle();
	nvRightBack2 = NearestVehicle();

	density = 0;
}

void Driver::resetReactionTime(double time)
{
	perceivedFwdVel->set_delay(time);
	perceivedFwdAcc->set_delay(time);
	perceivedVelOfFwdCar->set_delay(time);
	perceivedAccOfFwdCar->set_delay(time);
	perceivedDistToFwdCar->set_delay(time);
	perceivedDistToTrafficSignal->set_delay(time);
	perceivedTrafficColor->set_delay(time);
}

void Driver::rerouteWithBlacklist(const std::vector<const Link *> &blacklisted)
{
	DriverMovement *mov = dynamic_cast<DriverMovement *> (Movement());
	if (mov)
	{
		mov->rerouteWithBlacklist(blacklisted);
	}
}

void Driver::rerouteWithPath(const std::vector<WayPoint>& path)
{
	DriverMovement *mov = dynamic_cast<DriverMovement *> (Movement());
	
	if (mov)
	{
		mov->rerouteWithPath(path);
	}
}
