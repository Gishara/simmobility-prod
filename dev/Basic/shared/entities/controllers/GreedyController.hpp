/*
 * GreedyController.hpp
 *
 *  Created on: Apr 18, 2017
 *      Author: Akshay Padmanabha
 */

#pragma once

#include <vector>

#include "entities/Agent.hpp"
#include "OnCallController.hpp"

namespace sim_mob
{

class GreedyController : public OnCallController
{
public:
	GreedyController
			(const MutexStrategy &mtxStrat, unsigned int computationPeriod, unsigned id, TT_EstimateType ttEstimateType)
			:
			OnCallController(mtxStrat, computationPeriod, MobilityServiceControllerType::SERVICE_CONTROLLER_GREEDY, id,
			                 ttEstimateType)
	{
	}


protected:
	/**
	 * Performs the controller algorithm to assign vehicles to requests
	 */
	virtual void computeSchedules();
};
}
