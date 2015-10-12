//Copyright (c) 2015 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <set>
#include "entities/Agent.hpp"
#include "roles/DriverRequestParams.hpp"

namespace sim_mob
{

namespace medium
{

class BusControllerMT: public sim_mob::BusController
{
private:
	explicit BusControllerMT(int id = -1, const MutexStrategy& mtxStrat = sim_mob::MtxStrat_Buffered);

	virtual ~BusControllerMT();

public:
	/**
	 * Initialize a single BusController with the given start time and MutexStrategy.
	 */
	static void RegisterBusController(int id = -1, const MutexStrategy& mtxStrat = sim_mob::MtxStrat_Buffered);

	/**
	 * processes requests from all bus drivers
	 */
	virtual void processRequests();

	/**
	 * processes bus driver request
	 */
	virtual void handleRequest(sim_mob::DriverRequestParams rParams);

private:
	/**
	 * assign bus trip information to person so as to travel on the road
	 */
	virtual void assignBusTripChainWithPerson(std::set<sim_mob::Entity*>& activeAgents);
};

}

}

