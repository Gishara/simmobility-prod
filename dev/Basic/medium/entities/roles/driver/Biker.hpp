//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include "BikerFacets.hpp"
#include "Driver.hpp"

namespace sim_mob
{
namespace medium
{

class BikerBehavior;
class BikerMovement;

/**
 * Mid-term motor-cyclist
 * \author Harish Loganathan
 */
class Biker: public sim_mob::medium::Driver
{
public:
	Biker(Person_MT* parent, sim_mob::medium::BikerBehavior* behavior = nullptr, sim_mob::medium::BikerMovement* movement = nullptr, std::string roleName =
			std::string(), Role<Person_MT>::Type roleType = RL_BIKER);
	virtual ~Biker();

	//Virtual overrides
	virtual sim_mob::Role<Person_MT>* clone(sim_mob::Person_MT* parent) const;
};
} //end namespace medium
} //end namespace sim_mob

