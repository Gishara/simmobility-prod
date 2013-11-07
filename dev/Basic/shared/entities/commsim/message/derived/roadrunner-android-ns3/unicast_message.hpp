//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * UNICAST_Message.h
 *
 *  Created on: May 9, 2013
 *      Author: vahid
 *      unicast messages come from android clients(only) and
 *      are supposed to be routed to ns3
 */

#pragma once

#include "entities/commsim/message/Types.hpp"
#include "entities/commsim/message/base/Message.hpp"
#include "entities/commsim/message/base/Handler.hpp"
#include "entities/commsim/message/base/UnicastHandler.hpp"

namespace sim_mob {
class Agent;
class Broker;
class ClientHandler;

namespace rr_android_ns3 {

///Android unicast message (no documentation provided).
class ANDROID_MSG_UNICAST : public sim_mob::comm::Message {
public:
	ANDROID_MSG_UNICAST(sim_mob::comm::MsgData& data_);
	sim_mob::Handler* newHandler();
};

//Handler to the above message
/*class ANDROID_HDL_UNICAST : public sim_mob::roadrunner::UnicastHandler{
protected:
	virtual void postProcess(sim_mob::Broker& broker, const sim_mob::Agent& destAgent, sim_mob::ClientHandler& destCliHandler, const std::string andrSensorId, const std::string& andrSensorType, sim_mob::comm::MsgData &data);
};*/

///NS3 unicast message (no documentation provided).
class NS3_MSG_UNICAST : public sim_mob::comm::Message {
public:
	NS3_MSG_UNICAST(sim_mob::comm::MsgData& data_);
	sim_mob::Handler* newHandler();
};

//Handler to the above message
class NS3_HDL_UNICAST : public sim_mob::Handler {
public:
	void handle(sim_mob::comm::MsgPtr message_, sim_mob::Broker* broker);
};

}}
