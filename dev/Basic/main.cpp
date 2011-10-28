/* Copyright Singapore-MIT Alliance for Research and Technology */

/**
 * \file main.cpp
 * A first approximation of the basic pseudo-code in C++. The main file loads several
 * properties from data/config.xml, and attempts a simulation run. Currently, the various
 * granularities and pedestrian starting locations are loaded.
 */
#include <iostream>
#include <vector>
#include <string>
#include <boost/thread.hpp>

#include "simple_classes.h"
#include "constants.h"
#include "stubs.h"

#include "workers/Worker.hpp"
#include "workers/EntityWorker.hpp"
#include "workers/ShortestPathWorker.hpp"
#include "buffering/BufferedDataManager.hpp"
#include "WorkGroup.hpp"
#include "geospatial/aimsun/Loader.hpp"
#include "geospatial/RoadNetwork.hpp"
#include "geospatial/UniNode.hpp"
#include "geospatial/RoadSegment.hpp"
#include "geospatial/Lane.hpp"
#include "util/OutputUtil.hpp"

//Just temporarily, so we know it compiles:
#include "entities/Signal.hpp"
#include "conf/simpleconf.hpp"
#include "entities/AuraManager.hpp"
#include "entities/Bus.hpp"
#include "geospatial/BusStop.hpp"
#include "geospatial/Route.hpp"
#include "geospatial/BusRoute.hpp"
#include "perception/FixedDelayed.hpp"


using std::cout;
using std::endl;
using std::vector;
using boost::thread;


using namespace sim_mob;


//Helper
typedef WorkGroup<Entity> EntityWorkGroup;


/**
 * First "loading" step is special. Initialize all agents using work groups in parallel.
 */
void InitializeAll(vector<Agent*>& agents, vector<Region*>& regions, vector<TripChain*>& trips,
		      vector<ChoiceSet*>& choiceSets);



//TEST
void entity_worker(sim_mob::Worker<sim_mob::Entity>& wk, frame_t frameNumber)
{
	for (std::vector<sim_mob::Entity*>::iterator it=wk.getEntities().begin(); it!=wk.getEntities().end(); it++) {
		(*it)->update(frameNumber);
	}
}
void signal_status_worker(sim_mob::Worker<sim_mob::Entity>& wk, frame_t frameNumber)
{
	for (std::vector<sim_mob::Entity*>::iterator it=wk.getEntities().begin(); it!=wk.getEntities().end(); it++) {
            (*it)->update(frameNumber);
	}
}





/**
 * Main simulation loop.
 * \note
 * For doxygen, I'd like to have the variable JAVADOC AUTOBRIEF set to "true"
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
bool performMain(const std::string& configFileName)
{
  //Initialization: Scenario definition
  vector<Agent*>& agents = Agent::all_agents;
  vector<Region*> regions;
  vector<TripChain*> trips;
  vector<ChoiceSet*> choiceSets;

  //Load our user config file; save a handle to the shared definition of it.
  if (!ConfigParams::InitUserConf(configFileName, agents, regions, trips, choiceSets)) {   //Note: Agent "shells" are loaded here.
	  return false;
  }
  const ConfigParams& config = ConfigParams::GetInstance();

  //Initialization: Server configuration
  setConfiguration();

  //Initialization: Network decomposition among multiple machines.
  loadNetwork();



  ///////////////////////////////////////////////////////////////////////////////////
  // NOTE: Because of the way we cache the old values of agents, we need to run our
  //       initialization workers and then flip their values (otherwise there will be
  //       no data to read.) The other option is to load all "properties" with a default
  //       value, but at the moment we don't even have a "properties class"
  ///////////////////////////////////////////////////////////////////////////////////
  cout <<"Beginning Initialization" <<endl;
  InitializeAll(agents, regions, trips, choiceSets);
  cout <<"  " <<"Initialization done" <<endl;

  //Sanity check (simple)
  if (!checkIDs(agents, trips, choiceSets)) {
	  return false;
  }

  //Sanity check (nullptr)
  void* x = nullptr;
  if (x) {
	  return false;
  }

  //Output
  cout <<"  " <<"...Sanity Check Passed" <<endl;


  //Initialize our work groups, assign agents randomly to these groups.
  EntityWorkGroup agentWorkers(WG_AGENTS_SIZE, config.totalRuntimeTicks, config.granAgentsTicks, true);
  Worker<sim_mob::Entity>::actionFunction entityWork = boost::bind(entity_worker, _1, _2);
  agentWorkers.initWorkers(&entityWork);
  for (size_t i=0; i<agents.size(); i++) {
	  agentWorkers.migrate(agents[i], -1, i%WG_AGENTS_SIZE);
  }

  //Initialize our signal status work groups
  //  TODO: There needs to be a more general way to do this.
  EntityWorkGroup signalStatusWorkers(WG_SIGNALS_SIZE, config.totalRuntimeTicks, config.granSignalsTicks);
  Worker<sim_mob::Entity>::actionFunction spWork = boost::bind(signal_status_worker, _1, _2);
  signalStatusWorkers.initWorkers(&spWork);
  for (size_t i=0; i<Signal::all_signals_.size(); i++) {
	  signalStatusWorkers.migrate(const_cast<Signal*>(Signal::all_signals_[i]), -1, i%WG_SIGNALS_SIZE);
  }

  //Initialize our shortest path work groups
  //  TODO: There needs to be a more general way to do this.
  //EntityWorkGroup shortestPathWorkers(WG_SHORTEST_PATH_SIZE, config.totalRuntimeTicks, config.granPathsTicks);
  //shortestPathWorkers.initWorkers();
  /////////////////////////////////////////////////////////////////////////////
  // NOTE: Currently, an Agent can only be "managed" by one Worker. We need a way to
  //       say that the agent is the data object of a worker, but WON'T be managed by it.
  // For example, shortest-path-worker only needs to read X/Y, but will update "shortestPath"
  //        (which the EntityWorker won't touch).
  /////////////////////////////////////////////////////////////////////////////
  /*for (size_t i=0; i<agents.size(); i++) {
	  shortestPathWorkers.migrate(&agents[i], -1, i%WG_SHORTEST_PATH_SIZE);
  }*/

  //Start work groups
  agentWorkers.startAll();
  signalStatusWorkers.startAll();
  //shortestPathWorkers.startAll();

  AuraManager& auraMgr = AuraManager::instance();
  auraMgr.init();

  /////////////////////////////////////////////////////////////////
  // NOTE: WorkGroups are able to handle skipping steps by themselves.
  //       So, we simply call "wait()" on every tick, and on non-divisible
  //       time ticks, the WorkGroups will return without performing
  //       a barrier sync.
  /////////////////////////////////////////////////////////////////
  for (unsigned int currTick=0; currTick<config.totalRuntimeTicks; currTick++) {
	  //Output
	  {
		  boost::mutex::scoped_lock local_lock(sim_mob::Logger::global_mutex);
          cout <<"Approximate Tick Boundary: " <<currTick <<", " <<(currTick*config.baseGranMS) <<" ms" <<endl;
	  }

	  //Update the signal logic and plans for every intersection grouped by region
	  signalStatusWorkers.wait();

	  //Update weather, traffic conditions, etc.
	  updateTrafficInfo(regions);

	  //Longer Time-based cycle
	  //shortestPathWorkers.wait();

	  //Longer Time-based cycle
	  //TODO: Put these on Worker threads too.
	  agentDecomposition(agents);

	  //Agent-based cycle
	  agentWorkers.wait();

      auraMgr.update(currTick);
	  agentWorkers.waitExternAgain(); // The workers wait on the AuraManager.

	  //Surveillance update
	  updateSurveillanceData(agents);

	  //Check if the warmup period has ended.
	  if (currTick >= config.totalWarmupTicks) {
		  updateGUI(agents);
		  saveStatistics(agents);
	  } else {
		  boost::mutex::scoped_lock local_lock(sim_mob::Logger::global_mutex);
		  cout <<"  Warmup; output ignored." <<endl;
	  }

	  saveStatisticsToDB(agents);
  }

  cout <<"Simulation complete; closing worker threads." <<endl;
  if (Agent::all_agents.empty()) {
	  cout <<"NOTE: No agents were processed." <<endl;
  }
  return true;
}



int main(int argc, char* argv[])
{
	//Argument 1: Config file
	//Note: Don't chnage this here; change it by supplying an argument on the
	//      command line, or through Eclipse's "Run Configurations" dialog.
	std::string configFileName = "data/config.xml";
	if (argc>1) {
		configFileName = argv[1];
	} else {
		cout <<"No config file specified; using default." <<endl;
	}
	cout <<"Using config file: " <<configFileName <<endl;

	//Argument 2: Log file
	if (argc>2) {
		if (!Logger::log_init(argv[2])) {
			cout <<"Loading output file failed; using cout" <<endl;
			cout <<argv[2] <<endl;
		}
	} else {
		Logger::log_init("");
		cout <<"No output file specified; using cout." <<endl;
	}

	//This should be moved later, but we'll likely need to manage random numbers
	//ourselves anyway, to make simulations as repeatable as possible.
	time_t t = time(NULL);
	srand (t);
	cout <<"Random Seed Init: " <<t <<endl;

	//Perform main loop
	int returnVal = performMain(configFileName) ? 0 : 1;

	//Close log file, return.
	Logger::log_done();
	cout <<"Done" <<endl;
	return returnVal;
}



/**
 * Parallel initialization step.
 */
void InitializeAll(vector<Agent*>& agents, vector<Region*>& regions, vector<TripChain*>& trips,
	      vector<ChoiceSet*>& choiceSets)
{
	  //Our work groups. Will be disposed after this time tick.
	  SimpleWorkGroup<TripChain> tripChainWorkers(WG_TRIPCHAINS_SIZE, 1);
	  WorkGroup<sim_mob::Agent> createAgentWorkers(WG_CREATE_AGENT_SIZE, 1);
	  SimpleWorkGroup<ChoiceSet> choiceSetWorkers(WG_CHOICESET_SIZE, 1);

	  //Create object from DB; for long time spans objects must be created on demand.
	  Worker<TripChain>::actionFunction func1 = boost::bind(load_trip_chain, _1, _2);
	  tripChainWorkers.initWorkers(&func1);
	  for (size_t i=0; i<trips.size(); i++) {
		  tripChainWorkers.migrate(trips[i], -1, i%WG_TRIPCHAINS_SIZE);
	  }

	  //Agents and choice sets
	  Worker<sim_mob::Agent>::actionFunction func2 = boost::bind(load_agents, _1, _2);
	  createAgentWorkers.initWorkers(&func2);
	  for (size_t i=0; i<agents.size(); i++) {
		  createAgentWorkers.migrate(agents[i], -1, i%WG_CREATE_AGENT_SIZE);
	  }
	  Worker<ChoiceSet>::actionFunction func3 = boost::bind(load_choice_sets, _1, _2);
	  choiceSetWorkers.initWorkers(&func3);
	  for (size_t i=0; i<choiceSets.size(); i++) {
		  choiceSetWorkers.migrate(choiceSets[i], -1, i%WG_CHOICESET_SIZE);
	  }

	  //Start
	  cout <<"  Starting threads..." <<endl;
	  tripChainWorkers.startAll();
	  createAgentWorkers.startAll();
	  choiceSetWorkers.startAll();

	  //Flip once
	  tripChainWorkers.wait();
	  createAgentWorkers.wait();
	  choiceSetWorkers.wait();

	  cout <<"  Closing all work groups..." <<endl;
}




