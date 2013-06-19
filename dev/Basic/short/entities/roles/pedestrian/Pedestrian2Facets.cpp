/*
 * Pedestrian2Facets.cpp
 *
 *  Created on: May 14, 2013
 *      Author: Yao Jin
 */

#include "Pedestrian2Facets.hpp"
#include "geospatial/BusStop.hpp"
#include "entities/Person.hpp"
#include "entities/roles/passenger/Passenger.hpp"

using namespace sim_mob;

namespace sim_mob {
Pedestrian2Behavior::Pedestrian2Behavior(sim_mob::Person* parentAgent):
	BehaviorFacet(parentAgent), parentPedestrian2(nullptr) {}

Pedestrian2Behavior::~Pedestrian2Behavior() {}

void Pedestrian2Behavior::frame_init(UpdateParams& p) {
	throw std::runtime_error("Pedestrian2Behavior::frame_init is not implemented yet");
}

void Pedestrian2Behavior::frame_tick(UpdateParams& p) {
	throw std::runtime_error("Pedestrian2Behavior::frame_tick is not implemented yet");
}

void Pedestrian2Behavior::frame_tick_output(const UpdateParams& p) {
	throw std::runtime_error("Pedestrian2Behavior::frame_tick_output is not implemented yet");
}

void Pedestrian2Behavior::frame_tick_output_mpi(timeslice now) {
	throw std::runtime_error("Pedestrian2Behavior::frame_tick_output_mpi is not implemented yet");
}

double Pedestrian2Movement::collisionForce = 20;
double Pedestrian2Movement::agentRadius = 0.5; //Shoulder width of a person is about 0.5 meter

sim_mob::Pedestrian2Movement::Pedestrian2Movement(sim_mob::Person* parentAgent):
	MovementFacet(parentAgent), parentPedestrian2(nullptr), trafficSignal(nullptr),
	currCrossing(nullptr), isUsingGenPathMover(true) {
	//Check non-null parent. Perhaps references may be of use here?

	//Init
	sigColor = sim_mob::Green; //Green by default

#if 0
	sigColor = Signal::Green; //Green by default
#endif

	//Set default speed in the range of 1.2m/s to 1.6m/s
	speed = 1.2;

	xVel = 0;
	yVel = 0;

	xCollisionVector = 0;
	yCollisionVector = 0;
}

sim_mob::Pedestrian2Movement::~Pedestrian2Movement() {

}

void sim_mob::Pedestrian2Movement::frame_init(UpdateParams& p) {
	if(parentAgent) {
		parentAgent->setNextRole(nullptr);// set nextRole to be nullptr at frame_init
	}
	setSubPath();

	//dynamic_cast<PedestrianUpdateParams2&>(p).skipThisFrame = true;
}

void sim_mob::Pedestrian2Movement::frame_tick(UpdateParams& p) {
	PedestrianUpdateParams2& p2 = dynamic_cast<PedestrianUpdateParams2&>(p);

	//Is this the first frame tick?
	if (p2.skipThisFrame) {
		return;
	}

	double vel = 0;

	int signalGreen = 3;
	signalGreen = sim_mob::Green; //Green by default

#if 0
	signalGreen = Signal::Green; //Green by default
#endif
	if(pedMovement.isAtCrossing()){
		//Check whether to start to cross or not
		updatePedestrianSignal();

		if (sigColor == signalGreen) //Green phase
			vel = speed * 2.0 * 100 * ConfigParams::GetInstance().agentTimeStepInMilliSeconds() / 1000.0;
		else
			vel = 0;
	}
	else {
		if (!pedMovement.isDoneWithEntireRoute())
			vel = speed * 1.2 * 100 * ConfigParams::GetInstance().agentTimeStepInMilliSeconds() / 1000.0;
		else
		{
			//Person* person = dynamic_cast<Person*> (parent);
			if(parentAgent && (parentAgent->destNode.type_==WayPoint::BUS_STOP)) { // it is at the busstop, dont set to be removed, just changeRole
				if(!parentAgent->findPersonNextRole())// find and assign the nextRole to this Person, when this nextRole is set to be nullptr?
				{
					std::cout << "End of trip chain...." << std::endl;
				}
				Passenger* passenger = dynamic_cast<Passenger*> (parentAgent->getNextRole());
				if(passenger) {// nextRole is passenger
					const RoleFactory& rf = ConfigParams::GetInstance().getRoleFactory();
					sim_mob::Role* newRole = rf.createRole("waitBusActivityRole", parentAgent);
					parentAgent->changeRole(newRole);
					newRole->Movement()->frame_init(p);
					return;
//					passenger->busdriver.set(busDriver);// assign this busdriver to Passenger
//					passenger->BoardedBus.set(true);
//					passenger->AlightedBus.set(false);
				}
			} else {// not at the busstop, set to be removed
				parentAgent->setToBeRemoved();
			}
		}
	}

		pedMovement.advance(vel);

		parentAgent->xPos.set(pedMovement.getPosition().x);
		parentAgent->yPos.set(pedMovement.getPosition().y);
}

void sim_mob::Pedestrian2Movement::frame_tick_output(const UpdateParams& p) {
	//	if (dynamic_cast<const PedestrianUpdateParams2&>(p).skipThisFrame) {
	//		return;
	//	}

		if (ConfigParams::GetInstance().using_MPI) {
			return;
		}

	//	std::ostringstream stream;
	//	stream<<"("<<"\"pedestrian\","<<p.now.frame() <<","<<parent->getId()<<","<<"{\"xPos\":\""<<parent->xPos.get()<<"\"," <<"\"yPos\":\""<<this->parent->yPos.get()<<"\",})";
	//	std::string s=stream.str();
	//	CommunicationDataManager::GetInstance()->sendTrafficData(s);

		LogOut("("<<"\"pedestrian\","<<p.now.frame()<<","<<parentAgent->getId()<<","<<"{\"xPos\":\""<<parentAgent->xPos.get()<<"\"," <<"\"yPos\":\""<<this->parentAgent->yPos.get()<<"\",})"<<std::endl);
}

void sim_mob::Pedestrian2Movement::frame_tick_output_mpi(timeslice now) {
	if (now.frame() < 1 || now.frame() < parentAgent->getStartTime())
		return;

	if (this->parentAgent->isFake) {
		LogOut("("<<"\"pedestrian\","<<now.frame()<<","<<parentAgent->getId()<<","<<"{\"xPos\":\""<<parentAgent->xPos.get()<<"\"," <<"\"yPos\":\""<<this->parentAgent->yPos.get() <<"\"," <<"\"xVel\":\""<< this->xVel <<"\"," <<"\"yVel\":\""<< this->yVel <<"\"," <<"\"fake\":\""<<"true" <<"\",})"<<std::endl);
	} else {
		LogOut("("<<"\"pedestrian\","<<now.frame()<<","<<parentAgent->getId()<<","<<"{\"xPos\":\""<<parentAgent->xPos.get()<<"\"," <<"\"yPos\":\""<<this->parentAgent->yPos.get() <<"\"," <<"\"xVel\":\""<< this->xVel <<"\"," <<"\"yVel\":\""<< this->yVel <<"\"," <<"\"fake\":\""<<"false" <<"\",})"<<std::endl);
	}
}

void sim_mob::Pedestrian2Movement::flowIntoNextLinkIfPossible(UpdateParams& p) {

}

void sim_mob::Pedestrian2Movement::setSubPath() {
	const StreetDirectory& stdir = StreetDirectory::instance();

	StreetDirectory::VertexDesc source, destination;
	if(parentAgent->originNode.type_==WayPoint::NODE)
		source = stdir.WalkingVertex(*parentAgent->originNode.node_);
	else if(parentAgent->originNode.type_==WayPoint::BUS_STOP)
		source = stdir.WalkingVertex(*parentAgent->originNode.busStop_);

	if(parentAgent->destNode.type_==WayPoint::NODE)
		destination = stdir.WalkingVertex(*parentAgent->destNode.node_);
	else if(parentAgent->destNode.type_==WayPoint::BUS_STOP)
		destination = stdir.WalkingVertex(*parentAgent->destNode.busStop_);

	vector<WayPoint> wp_path = stdir.SearchShortestWalkingPath(source, destination);

	//Used to debug pedestrian walking paths.
	std::cout<<"Pedestrian requested path from: " <<parentAgent->originNode.getID() <<" => " <<parentAgent->destNode.node_->getID() <<"  {" <<std::endl;
	for (vector<WayPoint>::iterator it = wp_path.begin(); it != wp_path.end(); it++) {
		if (it->type_ == WayPoint::SIDE_WALK) {
			const Node* start = !it->directionReverse ? it->lane_->getRoadSegment()->getStart() : it->lane_->getRoadSegment()->getEnd();
			const Node* end = !it->directionReverse ? it->lane_->getRoadSegment()->getEnd() : it->lane_->getRoadSegment()->getStart();
			std::cout<<"  Side-walk: " <<start->originalDB_ID.getLogItem() <<" => " <<end->originalDB_ID.getLogItem() <<"   (Reversed: " <<it->directionReverse <<")" <<std::endl;
		} else if (it->type_ == WayPoint::ROAD_SEGMENT) {
			std::cout<<"  Road Segment: (not supported)" <<std::endl;
		} else if (it->type_ == WayPoint::BUS_STOP) {
			std::cout<<"  Bus Stop: (not supported) id "<< it->busStop_->id << std::endl;
		} else if (it->type_ == WayPoint::CROSSING){
			std::cout<<"  Crossing at Node: " <<StreetDirectory::instance().GetCrossingNode(it->crossing_)->originalDB_ID.getLogItem() <<std::endl;
		} else if (it->type_ == WayPoint::NODE) {
			std::cout<<"  Node: " <<it->node_->originalDB_ID.getLogItem() <<std::endl;
		} else if (it->type_ == WayPoint::INVALID) {
			std::cout<<"  <Invalid>"<<std::endl;
		} else {
			std::cout<<"  Unknown type."<<std::endl;
		}
	}
	std::cout<<"}" <<std::endl;

	pedMovement.setPath(wp_path);
}

void sim_mob::Pedestrian2Movement::updatePedestrianSignal()
{
	const MultiNode* node = StreetDirectory::instance().GetCrossingNode(pedMovement.getCurrentWaypoint()->crossing_);
	if (!node) {
		throw std::runtime_error("Coulding find Pedestrian Sginal for crossing.");
	}

/*	const RoadSegment *rs = pedMovement.getCurrentWaypoint()->crossing_->getRoadSegment();

	// find intersection's multi node,compare the distance to start ,end nodes of segment,any other way?
	Point2D currentSegmentStartLocation(rs->getStart()->location);
	Point2D currentSegmentEndLocation(rs->getEnd()->location);
	DynamicVector pedDistanceToStartLocation(pedMovement.getPosition().x, pedMovement.getPosition().y,
			currentSegmentStartLocation.getX(), currentSegmentStartLocation.getY());
	DynamicVector pedDistanceToEndLocation(pedMovement.getPosition().x, pedMovement.getPosition().y,
			currentSegmentEndLocation.getX(), currentSegmentEndLocation.getY());

	const Node* node = NULL;
	Point2D location;
	if(pedDistanceToStartLocation.getMagnitude() >= pedDistanceToEndLocation.getMagnitude())
		location = rs->getEnd()->location;
	else
		location =rs->getStart()->location;

	// we have multi node ,so get signal
	if(rs)
		node = ConfigParams::GetInstance().getNetwork().locateNode(location, true);*/
	if (node)
		trafficSignal = StreetDirectory::instance().signalAt(*node);
	else
		trafficSignal = nullptr;

	if (!trafficSignal)
		std::cout << "Traffic signal not found!" << std::endl;
	else {
		if (pedMovement.getCurrentWaypoint()->crossing_) {
			sigColor = trafficSignal->getPedestrianLight(*pedMovement.getCurrentWaypoint()->crossing_);
			//			std::cout<<"Debug: signal color "<<sigColor<<std::endl;
		} else
			std::cout << "Current crossing not found!" << std::endl;
	}
}

void sim_mob::Pedestrian2Movement::checkForCollisions()
{
	//For now, just check all agents and get the first positive collision. Very basic.
	Agent* other = nullptr;
	for (size_t i = 0; i < Agent::all_agents.size(); i++) {
		//Skip self
		other = dynamic_cast<Agent*> (Agent::all_agents[i]);
		if (!other) {
			break;
		} //Shouldn't happen; we might need to write a function for this later.

		if (other->getId() == parentAgent->getId()) {
			other = nullptr;
			continue;
		}

		//Check.
		double dx = other->xPos.get() - parentAgent->xPos.get();
		double dy = other->yPos.get() - parentAgent->yPos.get();
		double distance = sqrt(dx * dx + dy * dy);
		if (distance < 2 * agentRadius) {
			break; //Collision
		}
		other = nullptr;
	}

	//Set collision vector. Overrides previous setting, if any.
	if (other) {
		//Get a heading.
		double dx = other->xPos.get() - parentAgent->xPos.get();
		double dy = other->yPos.get() - parentAgent->yPos.get();

		//If the two agents are directly on top of each other, set
		//  their distances to something non-crashable.
		if (dx == 0 && dy == 0) {
			dx = other->getId() - parentAgent->getId();
			dy = parentAgent->getId() - other->getId();
		}

		//Normalize
		double magnitude = sqrt(dx * dx + dy * dy);
		if (magnitude == 0) {
			dx = dy;
			dy = dx;
		}
		dx = dx / magnitude;
		dy = dy / magnitude;

		//Set collision vector to the inverse
		xCollisionVector = -dx * collisionForce;
		yCollisionVector = -dy * collisionForce;
	}
}

bool sim_mob::Pedestrian2Movement::checkGapAcceptance()
{
	//	//Search for the nearest driver on the current link
	//	Agent* a = nullptr;
	//	Person* p = nullptr;
	//	double pedRelX, pedRelY, drvRelX, drvRelY;
	//	double minDist=1000000;
	//	for (size_t i=0; i<Agent::all_agents.size(); i++) {
	//		//Skip self
	//		a = Agent::all_agents[i];
	//		if (a->getId()==parent->getId()) {
	//			a = nullptr;
	//			continue;
	//		}
	//
	//		if(dynamic_cast<Person*>(a)){
	//			p=dynamic_cast<Person*>(a);
	//			if(dynamic_cast<Driver*>(p->getRole())){
	//				if(p->currentLink.get()==0){
	//					//std::cout<<"dsahf"<<std::endl;
	//					absToRel(p->xPos.get(),p->yPos.get(),drvRelX,drvRelY);
	//					absToRel(parent->xPos.get(),parent->xPos.get(),pedRelX,pedRelY);
	//					if(minDist>abs(drvRelY-pedRelY))
	//						minDist = abs(drvRelY-pedRelY);
	//				}
	//			}
	//		}
	//		p = nullptr;
	//		a = nullptr;
	//	}
	//	p = nullptr;
	//	a = nullptr;
	//
	//	if((minDist/5-30/speed)>1)
	//	{
	//
	//		return true;
	//	}
	//	else
	//		return false;

	return false;
}
}
