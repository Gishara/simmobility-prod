//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/**
 * \file main.cpp
 * A first approximation of the basic pseudo-code in C++. The main file loads several
 * properties from data/config.xml, and attempts a simulation run. Currently, the various
 * granularities and pedestrian starting locations are loaded.
 *
 * \author Seth N. Hetu
 * \author LIM Fung Chai
 * \author Xu Yan
 */
#include <vector>
#include <string>
#include <ctime>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>

//main.cpp (top-level) files can generally get away with including GenConfig.h
#include "GenConfig.h"

#include "workers/Worker.hpp"
#include "buffering/BufferedDataManager.hpp"
#include "workers/WorkGroup.hpp"
#include "geospatial/aimsun/Loader.hpp"
#include "geospatial/RoadNetwork.hpp"
#include "geospatial/MultiNode.hpp"
#include "geospatial/UniNode.hpp"
#include "geospatial/LaneConnector.hpp"
#include "geospatial/RoadSegment.hpp"
#include "geospatial/Lane.hpp"
#include "util/OutputUtil.hpp"
#include "util/DailyTime.hpp"
#include "entities/signal/Signal.hpp"
#include "conf/simpleconf.hpp"
#include "entities/AuraManager.hpp"
#include "entities/TrafficWatch.hpp"
#include "entities/Person.hpp"
#include "entities/BusStopAgent.hpp"
#include "entities/roles/Role.hpp"
#include "entities/roles/RoleFactory.hpp"
#include "entities/roles/activityRole/ActivityPerformer.hpp"
#include "entities/roles/waitBusActivityRole/WaitBusActivityRole.hpp"
#include "entities/roles/driver/BusDriver.hpp"
#include "entities/roles/driver/Driver.hpp"
#include "entities/roles/pedestrian/Pedestrian.hpp"
#include "entities/roles/pedestrian/Pedestrian2.hpp"
#include "entities/roles/passenger/Passenger.hpp"
#include "entities/profile/ProfileBuilder.hpp"
#include "geospatial/BusStop.hpp"
#include "geospatial/Roundabout.hpp"
#include "geospatial/Intersection.hpp"
#include "geospatial/Route.hpp"
#include "perception/FixedDelayed.hpp"
#include "buffering/Buffered.hpp"
#include "buffering/Locked.hpp"
#include "buffering/Shared.hpp"
#include "network/CommunicationManager.hpp"
#include "network/ControlManager.hpp"
#include "logging/Log.hpp"


//add by xuyan
#include "partitions/PartitionManager.hpp"
#include "partitions/ShortTermBoundaryProcessor.hpp"
#include "partitions/ParitionDebugOutput.hpp"
#include "util/PerformanceProfile.hpp"

//Note: This must be the LAST include, so that other header files don't have
//      access to cout if SIMMOB_DISABLE_OUTPUT is true.
#include <iostream>
#include <tinyxml.h>

using std::cout;
using std::endl;
using std::vector;
using std::string;

using namespace sim_mob;

//Start time of program
timeval start_time;

//Helper for computing differences. May be off by ~1ms
namespace {
int diff_ms(timeval t1, timeval t2) {
    return (((t1.tv_sec - t2.tv_sec) * 1000000) + (t1.tv_usec - t2.tv_usec))/1000;
}
} //End anon namespace

//Current software version.
const string SIMMOB_VERSION = string(SIMMOB_VERSION_MAJOR) + ":" + SIMMOB_VERSION_MINOR;



/**
 * Main simulation loop.
 * \note
 * For doxygen, we are setting the variable JAVADOC AUTOBRIEF to "true"
 * This isn't necessary for class-level documentation, but if we want
 * documentation for a short method (like "get" or "set") then it makes sense to
 * have a few lines containing brief/full comments. (See the manual's description
 * of JAVADOC AUTOBRIEF). Of course, we can discuss this first.
 *
 * \par
 * See Buffered.hpp for an example of this in action.
 *
 * \par
 * ~Seth
 *
 * This function is separate from main() to allow for easy scoping of WorkGroup objects.
 */
bool performMain(const std::string& configFileName,const std::string& XML_OutPutFileName) {
	cout <<"Starting SimMobility, version1 " <<SIMMOB_VERSION <<endl;

	//Enable or disable logging (all together, for now).
	//NOTE: This may seem like an odd place to put this, but it makes sense in context.
	//      OutputEnabled is always set to the correct value, regardless of whether ConfigParams()
	//      has been loaded or not. The new Config class makes this much clearer.
	if (ConfigParams::GetInstance().OutputEnabled()) {
		Log::Init("out.txt");
		Warn::Init("warn.log");
		Print::Init("<stdout>");
	} else {
		Log::Ignore();
		Warn::Ignore();
		Print::Ignore();
	}

	ProfileBuilder* prof = nullptr;
	if (ConfigParams::GetInstance().ProfileOn()) {
		ProfileBuilder::InitLogFile("profile_trace.txt");
		prof = new ProfileBuilder();
	}

	//Register our Role types.
	//TODO: Accessing ConfigParams before loading it is technically safe, but we
	//      should really be clear about when this is not ok.
	for (int i=0; i<2; i++) {
		//Set for the old-style config first, new-style config second.
		RoleFactory& rf = (i==0) ? ConfigParams::GetInstance().getRoleFactoryRW() : Config::GetInstanceRW().roleFactory();
		MutexStrategy mtx = (i==0) ? ConfigParams::GetInstance().mutexStategy : Config::GetInstance().mutexStrategy();

		rf.registerRole("driver", new sim_mob::Driver(nullptr, mtx));
		rf.registerRole("pedestrian", new sim_mob::Pedestrian2(nullptr));
		//rf.registerRole("passenger",new sim_mob::Passenger(nullptr));

		rf.registerRole("passenger",new sim_mob::Passenger(nullptr, mtx));
		rf.registerRole("busdriver", new sim_mob::BusDriver(nullptr, mtx));
		rf.registerRole("activityRole", new sim_mob::ActivityPerformer(nullptr));
		rf.registerRole("waitBusActivityRole", new sim_mob::WaitBusActivityRole(nullptr));
		//cannot allocate an object of abstract type
//		rf.registerRole("activityRole", new sim_mob::ActivityPerformer(nullptr));
		//rf.registerRole("buscontroller", new sim_mob::BusController()); //Not a role!
	}

	//Loader params for our Agents
	WorkGroup::EntityLoadParams entLoader(Agent::pending_agents, Agent::all_agents);

	//Prepare our built-in models
	//NOTE: These can leak memory for now, but don't delete them because:
	//      A) If a built-in Construct is used then it will need these models.
	//      B) We'll likely replace these with Factory classes later (static, etc.), so
	//         memory management will cease to be an issue.
	Config::BuiltInModels builtIn;
	builtIn.carFollowModels["mitsim"] = new MITSIM_CF_Model();
	builtIn.laneChangeModels["mitsim"] = new MITSIM_LC_Model();
	builtIn.intDrivingModels["linear"] = new SimpleIntDrivingModel();

	//Load our user config file
	std::cout << "start to Load our user config file." << std::endl;
	ConfigParams::InitUserConf(configFileName, Agent::all_agents, Agent::pending_agents, prof, builtIn);
	std::cout << "finish to Load our user config file." << std::endl;

	//Initialize the control manager and wait for an IDLE state (interactive mode only).
	sim_mob::ControlManager* ctrlMgr = nullptr;
	if (ConfigParams::GetInstance().InteractiveMode()) {
		std::cout<<"load scenario ok, simulation state is IDLE"<<std::endl;
		ctrlMgr = ConfigParams::GetInstance().getControlMgr();
		ctrlMgr->setSimState(IDLE);
		while(ctrlMgr->getSimState() == IDLE) {
			boost::this_thread::sleep(boost::posix_time::milliseconds(10));
		}
	}

	//Save a handle to the shared definition of the configuration.
	const ConfigParams& config = ConfigParams::GetInstance();

	//Start boundaries
	if (!config.MPI_Disabled() && config.using_MPI) {
		PartitionManager::instance().initBoundaryTrafficItems();
	}

	bool NoDynamicDispatch = config.DynamicDispatchDisabled();

	PartitionManager* partMgr = nullptr;
	if (!config.MPI_Disabled() && config.using_MPI) {
		partMgr = &PartitionManager::instance();
	}

	{ //Begin scope: WorkGroups
	//TODO: WorkGroup scope currently does nothing. We need to re-enable WorkGroup deletion at some later point. ~Seth

	//Work Group specifications
	WorkGroup* agentWorkers = WorkGroup::NewWorkGroup(config.agentWorkGroupSize, config.totalRuntimeTicks, config.granAgentsTicks, &AuraManager::instance(), partMgr);
	WorkGroup* signalStatusWorkers = WorkGroup::NewWorkGroup(config.signalWorkGroupSize, config.totalRuntimeTicks, config.granSignalsTicks);

	std::cout << "start to Load our user config file." << std::endl;

	//NOTE: I moved this from an #ifdef into a local variable.
	//      Recompiling main.cpp is much faster than recompiling everything which relies on
	//      PerformanceProfile.hpp   ~Seth
	bool doPerformanceMeasurement = false; //TODO: From config file.
	bool measureInParallel = true;
	PerformanceProfile perfProfile;
	if (doPerformanceMeasurement) {
		perfProfile.init(config.agentWorkGroupSize, measureInParallel);
	}

	//Initialize all work groups (this creates barriers, and locks down creation of new groups).
	WorkGroup::InitAllGroups();

	//Initialize each work group individually
	agentWorkers->initWorkers(NoDynamicDispatch ? nullptr :  &entLoader);
	signalStatusWorkers->initWorkers(nullptr);

	//Anything in all_agents is starting on time 0, and should be added now.
	for (vector<Entity*>::iterator it = Agent::all_agents.begin(); it != Agent::all_agents.end(); it++) {
		agentWorkers->assignAWorker(*it);
	}

	std::cout << "BusStopAgent::all_BusstopAgents_.size(): " << BusStopAgent::all_BusstopAgents_.size() << std::endl;
	for (vector<BusStopAgent*>::iterator it = BusStopAgent::all_BusstopAgents_.begin(); it != BusStopAgent::all_BusstopAgents_.end(); it++) {
		agentWorkers->assignAWorker(*it);
	}
	//Assign all signals too
	for (vector<Signal*>::iterator it = Signal::all_signals_.begin(); it != Signal::all_signals_.end(); it++) {
//		std::cout << "performmain() Signal " << (*it)->getId() << "  Has " <<  (*it)->getPhases().size()/* << "  " << (*it)->getNOF_Phases()*/ <<  " phases\n";
		signalStatusWorkers->assignAWorker(*it);
	}

	cout << "Initial Agents dispatched or pushed to pending." << endl;

	//Initialize the aura manager
	AuraManager::instance().init(config.aura_manager_impl, (doPerformanceMeasurement ? &perfProfile : nullptr));



	//////////////////////////////DEBUG CODE START
#if 0
	StreetDirectory& stdir = StreetDirectory::instance();
	const RoadNetwork& rn = ConfigParams::GetInstance().getNetwork();

	//First test: longer route on a 2-way street.
	MultiNode* aim91218  = dynamic_cast<MultiNode*>(rn.locateNode(37227139,14327875, false));
	Node* aim66508  = rn.locateNode(37250760,14355120, false);
	Node* aim103046 = rn.locateNode(37236345,14337301, true); //Part of the blacklisted segment.
	RoadSegment* blacklistSeg = nullptr;
	for (std::set<sim_mob::RoadSegment*>::const_iterator segIt=aim91218->getRoadSegments().begin(); segIt!=aim91218->getRoadSegments().end(); segIt++) {
		if ((*segIt)->getEnd()==aim91218 && (*segIt)->getStart()==aim103046) {
			blacklistSeg = *segIt;
			break;
		}
	}
	if (!blacklistSeg) { throw 1; }

	//Subtest 1: basic route
	vector<WayPoint> route = stdir.SearchShortestDrivingPath(stdir.DrivingVertex(*aim66508), stdir.DrivingVertex(*aim91218));
	LogOut("ROUTE 1:\n");
	for (vector<WayPoint>::iterator it=route.begin(); it!=route.end(); it++) {
		if (it->type_==WayPoint::ROAD_SEGMENT) {
			LogOut("  " <<it->roadSegment_->getStart()->originalDB_ID.getLogItem() <<" => " <<it->roadSegment_->getEnd()->originalDB_ID.getLogItem() <<std::endl);
		} else {
			LogOut("  <other>\n");
		}
	}

	//Subtest 2: blacklist the easiest route.
	vector<const RoadSegment*> blacklistV; blacklistV.push_back(blacklistSeg);
	route = stdir.SearchShortestDrivingPath(stdir.DrivingVertex(*aim66508), stdir.DrivingVertex(*aim91218), blacklistV);
	LogOut("ROUTE 2:\n");
	for (vector<WayPoint>::iterator it=route.begin(); it!=route.end(); it++) {
		if (it->type_==WayPoint::ROAD_SEGMENT) {
			LogOut("  " <<it->roadSegment_->getStart()->originalDB_ID.getLogItem() <<" => " <<it->roadSegment_->getEnd()->originalDB_ID.getLogItem() <<std::endl);
		} else {
			LogOut("  <other>\n");
		}
	}
#endif



	//////////////////////////////DEBUG CODE END



	///
	///  TODO: Do not delete this next line. Please read the comment in TrafficWatch.hpp
	///        ~Seth
	///
//	TrafficWatch& trafficWatch = TrafficWatch::instance();

	//Start work groups and all threads.
	WorkGroup::StartAllWorkGroups();

	//
	if (!config.MPI_Disabled() && config.using_MPI) {
		PartitionManager& partitionImpl = PartitionManager::instance();
		partitionImpl.setEntityWorkGroup(agentWorkers, signalStatusWorkers);

		std::cout << "partition_solution_id in main function:" << partitionImpl.partition_config->partition_solution_id << std::endl;
	}

	/////////////////////////////////////////////////////////////////
	// NOTE: WorkGroups are able to handle skipping steps by themselves.
	//       So, we simply call "wait()" on every tick, and on non-divisible
	//       time ticks, the WorkGroups will return without performing
	//       a barrier sync.
	/////////////////////////////////////////////////////////////////
	size_t numStartAgents = Agent::all_agents.size();
	size_t numPendingAgents = Agent::pending_agents.size();
	size_t maxAgents = Agent::all_agents.size();

	timeval loop_start_time;
	gettimeofday(&loop_start_time, nullptr);
	int loop_start_offset = diff_ms(loop_start_time, start_time);

	ParitionDebugOutput debug;


	int lastTickPercent = 0; //So we have some idea how much time is left.
	int endTick = config.totalRuntimeTicks;
	for (unsigned int currTick = 0; currTick < endTick; currTick++) {
		if (ConfigParams::GetInstance().InteractiveMode()) {
			if(ctrlMgr->getSimState() == STOP) {
				while (ctrlMgr->getEndTick() < 0) {
					ctrlMgr->setEndTick(currTick+2);
				}
				endTick = ctrlMgr->getEndTick();
			}
		}

		//xuyan:measure simulation time
		if (currTick == 600 * 5 + 1)
		{ // mins
			if (doPerformanceMeasurement) {
				perfProfile.startMeasure();
				perfProfile.markStartSimulation();
			}
		}
		if (currTick == 600 * 30 - 1)
		{ // mins
			if (doPerformanceMeasurement) {
				perfProfile.markEndSimulation();
				perfProfile.endMeasure();
			}
		}

		//Flag
		bool warmupDone = (currTick >= config.totalWarmupTicks);

		//Get a rough idea how far along we are
		int currTickPercent = (currTick*100)/config.totalRuntimeTicks;

		//Save the maximum number of agents at any given time
		maxAgents = std::max(maxAgents, Agent::all_agents.size());

		//Output
		if (ConfigParams::GetInstance().OutputEnabled()) {
			std::stringstream msg;
			msg << "Approximate Tick Boundary: " << currTick << ", ";
			msg << (currTick * config.baseGranMS) << " ms   [" <<currTickPercent <<"%]" << endl;
			if (!warmupDone) {
				msg << "  Warmup; output ignored." << endl;
			}
			PrintOut(msg.str());
		} else {
			//We don't need to lock this output if general output is disabled, since Agents won't
			//  perform any output (and hence there will be no contention)
			if (currTickPercent-lastTickPercent>9) {
				lastTickPercent = currTickPercent;
				cout <<currTickPercent <<"%" <<endl;
			}
		}

		///
		///  TODO: Do not delete this next line. Please read the comment in TrafficWatch.hpp
		///        ~Seth
		///
//		trafficWatch.update(currTick);

		//Agent-based cycle, steps 1,2,3,4 of 4
		WorkGroup::WaitAllGroups();

		//Check if the warmup period has ended.
		if (warmupDone) {
		}
	}

	//Finalize partition manager
	if (!config.MPI_Disabled() && config.using_MPI) {
		PartitionManager& partitionImpl = PartitionManager::instance();
		partitionImpl.stopMPIEnvironment();
	}

	std::cout <<"Database lookup took: " <<loop_start_offset <<" ms" <<std::endl;

	cout << "Max Agents at any given time: " <<maxAgents <<std::endl;
	cout << "Starting Agents: " << numStartAgents;
	cout << ",     Pending: ";
	if (NoDynamicDispatch) {
		cout <<"<Disabled>";
	} else {
		cout <<numPendingAgents;
	}
	cout << endl;

	//xuyan:show measure time
	if (doPerformanceMeasurement) {
		perfProfile.showPerformanceProfile();
	}

	if (Agent::all_agents.empty()) {
		cout << "All Agents have left the simulation.\n";
	} else {
		size_t numPerson = 0;
		size_t numDriver = 0;
		size_t numPedestrian = 0;
		size_t numPassenger = 0;
		for (vector<Entity*>::iterator it = Agent::all_agents.begin(); it
				!= Agent::all_agents.end(); it++) {
			Person* p = dynamic_cast<Person*> (*it);
			if (p) {
				numPerson++;
				if (p->getRole() && dynamic_cast<Driver*> (p->getRole())) {
					numDriver++;
				}
				if (p->getRole() && dynamic_cast<Pedestrian*> (p->getRole())) {
					numPedestrian++;
				}
				if (p->getRole() && dynamic_cast<Passenger*> (p->getRole())) {
					numPassenger++;
								}
			}
		}
		cout << "Remaining Agents: " << numPerson << " (Person)   "
				<< (Agent::all_agents.size() - numPerson) << " (Other)" << endl;
		cout << "   Person Agents: " << numDriver << " (Driver)   "
				<< numPedestrian << " (Pedestrian)   " << numPassenger << " (Passenger) " << (numPerson
				- numDriver - numPedestrian) << " (Other)" << endl;
	}

	if (ConfigParams::GetInstance().numAgentsSkipped>0) {
		cout <<"Agents SKIPPED due to invalid route assignment: " <<ConfigParams::GetInstance().numAgentsSkipped <<endl;
	}

	if (!Agent::pending_agents.empty()) {
		cout << "WARNING! There are still " << Agent::pending_agents.size()
				<< " Agents waiting to be scheduled; next start time is: "
				<< Agent::pending_agents.top()->getStartTime() << " ms\n";
		if (ConfigParams::GetInstance().DynamicDispatchDisabled()) {
			throw std::runtime_error("ERROR: pending_agents shouldn't be used if Dynamic Dispatch is disabled.");
		}
	}

	//Here, we will simply scope-out the WorkGroups, and they will migrate out all remaining Agents.
	}  //End scope: WorkGroups. (Todo: should move this into its own function later)
	WorkGroup::FinalizeAllWorkGroups();

	//At this point, it should be possible to delete all Signals and Agents.
	//TODO: For some reason, clear_delete_vector() does (may?) not work in INTERACTIVE mode.
	//      We can address this later, but it should *definitely* be possible to cleanly
	//      exit (even early) from the simulation.
	//TODO: I think that the WorkGroups and Workers need to have the "endTick" value propagated to
	//      them from the main loop, in the event that the simulator is shutting down early. This is
	//      probably causing the Workers to hang if clear_delete_vector is called. ~Seth
	//EDIT: Actually, Worker seems to handle the synchronization fine too.... but I still think the main
	//      loop should propagate this value down. ~Seth
	if (ConfigParams::GetInstance().InteractiveMode()) {
		Signal::all_signals_.clear();
		Agent::all_agents.clear();
	} else {
		clear_delete_vector(Signal::all_signals_);
		clear_delete_vector(Agent::all_agents);
	}

	cout << "Simulation complete; closing worker threads." << endl;

	//Delete our profiler, if it exists.
	safe_delete_item(prof);
	return true;
}

/**
 * Run the main loop of Sim Mobility, using command-line input.
 * Returns the value of the last completed run of performMain().
 */
int run_simmob_interactive_loop() {
	sim_mob::ControlManager *ctrlMgr = ConfigParams::GetInstance().getControlMgr();
	int retVal = 1;
	for (;;)
	{
		if(ctrlMgr->getSimState() == LOADSCENARIO)
		{
			ctrlMgr->setSimState(RUNNING);
			std::map<std::string,std::string> paras;
			ctrlMgr->getLoadScenarioParas(paras);
			std::string configFileName = paras["configFileName"];
			retVal = performMain(configFileName,"XML_OutPut.xml") ? 0 : 1;
			ctrlMgr->setSimState(STOP);
			ConfigParams::GetInstance().reset();
			std::cout<<"scenario finished"<<std::cout;
		}
		if(ctrlMgr->getSimState() == QUIT)
		{
			std::cout<<"Thank you for using SIMMOB. Have a good day!"<<std::endl;
			break;
		}
	}

	return retVal;
}

int main(int argc, char* argv[])
{
	std::cout << "Using New Signal Model" << std::endl;

#if 0
	std::cout << "Not Using New Signal Model" << std::endl;
#endif

	//Currently needs the #ifdef because of the way threads initialize.
#ifdef SIMMOB_INTERACTIVE_MODE
	CommunicationManager *dataServer = new CommunicationManager(13333, ConfigParams::GetInstance().getCommDataMgr(), *ConfigParams::GetInstance().getControlMgr());
	boost::thread dataWorkerThread(boost::bind(&CommunicationManager::start, dataServer));
	CommunicationManager *cmdServer = new CommunicationManager(13334, ConfigParams::GetInstance().getCommDataMgr(), *ConfigParams::GetInstance().getControlMgr());
	boost::thread cmdWorkerThread(boost::bind(&CommunicationManager::start, cmdServer));
	CommunicationManager *roadNetworkServer = new CommunicationManager(13335, ConfigParams::GetInstance().getCommDataMgr(), *ConfigParams::GetInstance().getControlMgr());
	boost::thread roadNetworkWorkerThread(boost::bind(&CommunicationManager::start, roadNetworkServer));
	boost::thread workerThread2(boost::bind(&ControlManager::start, ConfigParams::GetInstance().getControlMgr()));
#endif

	//Save start time
	gettimeofday(&start_time, nullptr);

	/**
	 * Check whether to run SimMobility or SimMobility-MPI
	 * TODO: Retrieving ConfigParams before actually loading the config file is dangerous.
	 */
	ConfigParams& config = ConfigParams::GetInstance();
	config.using_MPI = false;
#ifndef SIMMOB_DISABLE_MPI
	if (argc > 3 && strcmp(argv[3], "mpi") == 0) {
		config.using_MPI = true;
	}
#endif

	/**
	 * set random be repeatable
	 * TODO: Retrieving ConfigParams before actually loading the config file is dangerous.
	 */
	config.is_simulation_repeatable = true;

	/**
	 * Start MPI if using_MPI is true
	 */
#ifndef SIMMOB_DISABLE_MPI
	if (config.using_MPI)
	{
		PartitionManager& partitionImpl = PartitionManager::instance();
		std::string mpi_result = partitionImpl.startMPIEnvironment(argc, argv);
		if (mpi_result.compare("") != 0)
		{
			Warn() << "MPI Error:" << mpi_result << endl;
			exit(1);
		}

		ShortTermBoundaryProcessor* boundary_processor = new ShortTermBoundaryProcessor();
		partitionImpl.setBoundaryProcessor(boundary_processor);
	}
#endif

	//Argument 1: Config file
	//Note: Don't chnage this here; change it by supplying an argument on the
	//      command line, or through Eclipse's "Run Configurations" dialog.
	std::string configFileName = "data/config.xml";
	std::string XML_OutPutFileName = "data/SimMobilityInput.xml";
	if (argc > 1)
	{
		configFileName = argv[1];
	}
	else
	{
		cout << "No config file specified; using default." << endl;
	}
	cout << "Using config file: " << configFileName << endl;

	//Argument 2: Log file. Defaults to out.txt
	string logFileName = argc>2 ? argv[2] : "out.txt";
	if (ConfigParams::GetInstance().OutputEnabled()) {
		if (!Logger::log_init(logFileName)) {
			cout <<"Failed to initialized log file: \"" <<logFileName <<"\"" <<", defaulting to cout." <<endl;
		}
	}

	//Perform main loop (this differs for interactive mode)
	int returnVal = 1;
	if (ConfigParams::GetInstance().InteractiveMode()) {
		returnVal = run_simmob_interactive_loop();
	} else {
		returnVal = performMain(configFileName,"XML_OutPut.xml") ? 0 : 1;
	}

	//Close log file, return.
	if (ConfigParams::GetInstance().OutputEnabled()) {
		Logger::log_done();
	}

	cout << "Done" << endl;
	return returnVal;
}

