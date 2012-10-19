/*
 * Conflux.cpp
 *
 *  Created on: Oct 2, 2012
 *      Author: harish
 */

#include<map>
#include "Conflux.hpp"
using namespace sim_mob;
typedef Entity::UpdateStatus UpdateStatus;

void sim_mob::Conflux::addStartingAgent(sim_mob::Agent* ag, sim_mob::RoadSegment* rdSeg) {
	/**
	 * The agents always start at a node (for now), i.e. the start of a road segment.
	 * So for now, we will always add the agent to the road segment in "lane infinity".
	 */
	sim_mob::AgentKeeper* rdSegVehicles = segmentAgents.at(rdSeg);
	rdSegVehicles->laneInfinity.push(ag);
}

void sim_mob::Conflux::addAgent(sim_mob::Agent* ag) {
	throw std::runtime_error("Conflux::addAgent() not implemented");
}

UpdateStatus sim_mob::Conflux::update(frame_t frameNumber) {
	UpdateStatus retVal(UpdateStatus::RS_DONE);
	currFrameNumber = frameNumber;
	if (sim_mob::StreetDirectory::instance().signalAt(*multiNode) != nullptr) {
		updateSignalized();
	}
	else {
		updateUnsignalized(frameNumber);
	}
	return retVal;
}

void sim_mob::Conflux::updateSignalized() {
}

void sim_mob::Conflux::updateUnsignalized(frame_t frameNumber) {
	initCandidateAgents();
	sim_mob::Agent* ag = agentClosestToIntersection();
	while (ag != nullptr) {
		//call agent's update
		UpdateStatus res = ag->update(currFrameNumber);
		if (res.status == UpdateStatus::RS_DONE) {
			//This Entity is done; schedule for deletion.
			parentWorker->scheduleForRemoval(ag);
		} else if (res.status == UpdateStatus::RS_CONTINUE) {
			//Still going, but we may have properties to start/stop managing
			for (std::set<BufferedBase*>::iterator it=res.toRemove.begin(); it!=res.toRemove.end(); it++) {
				parentWorker->stopManaging(*it);
			}
			for (std::set<BufferedBase*>::iterator it=res.toAdd.begin(); it!=res.toAdd.end(); it++) {
				parentWorker->beginManaging(*it);
			}
		} else {
			throw std::runtime_error("Unknown/unexpected update() return status.");
		}

		// get next agent to update
		ag = agentClosestToIntersection();
	}
}

void sim_mob::Conflux::initCandidateAgents() {
	resetCurrSegsOnUpLinks();
	typedef std::map<sim_mob::Link*, std::vector<sim_mob::RoadSegment*>::const_reverse_iterator >::iterator currSegsOnUpLinksIt;
	sim_mob::Link* lnk = nullptr;
	const sim_mob::RoadSegment* rdSeg = nullptr;
	for (currSegsOnUpLinksIt i = currSegsOnUpLinks.begin(); i != currSegsOnUpLinks.end(); i++) {
		lnk = (*i).first;
		do {
			rdSeg = *((*i).second);
			segmentAgents[rdSeg]->resetFrontalAgents();
			candidateAgents[rdSeg] = segmentAgents[rdSeg]->getNext();
			if(candidateAgents[rdSeg] == nullptr) {
				// this road segment is deserted. search the next (which is technically the previous).
				currSegsOnUpLinks[lnk]++;
			}
		} while (currSegsOnUpLinks[lnk] != upstreamSegmentsMap[lnk].rend() && candidateAgents[rdSeg] == nullptr);
	}
}

void sim_mob::Conflux::prepareAgentForHandover(sim_mob::Agent* ag) {
	throw std::runtime_error("Conflux::prepareAgentForHandover() not implemented");
}

std::map<sim_mob::Lane*, std::pair<int, int> > sim_mob::Conflux::getLanewiseAgentCounts(const sim_mob::RoadSegment* rdSeg) {
	return segmentAgents[rdSeg]->getAgentCountsOnLanes();
}

unsigned int sim_mob::Conflux::numMovingInSegment(
		const sim_mob::RoadSegment* rdSeg) {
	return segmentAgents[rdSeg]->numMovingInSegment();
}

void sim_mob::Conflux::resetCurrSegsOnUpLinks() {
	typedef std::vector<sim_mob::RoadSegment*>::const_reverse_iterator rdSegIt;
	for(std::map<sim_mob::Link*, const std::vector<sim_mob::RoadSegment*> >::iterator i = upstreamSegmentsMap.begin();
			i != upstreamSegmentsMap.end(); i++) {
			currSegsOnUpLinks[(*i).first] = (*i).second.rbegin();
	}
}

sim_mob::Agent* sim_mob::Conflux::agentClosestToIntersection() {
	std::map<const sim_mob::RoadSegment*, sim_mob::Agent* >::iterator i = candidateAgents.begin();
	sim_mob::Agent* ag = nullptr;
	const sim_mob::RoadSegment* agRdSeg = nullptr;
	double minDistance = std::numeric_limits<double>::infinity();
	while(i != candidateAgents.end()) {
		if((*i).second != nullptr) {
			if(minDistance == (*i).second->distanceToEndOfSegment) {
				// If current ag and (*i) are at equal distance to the stop line, we toss a coin and choose one of them
				bool coinTossResult = ((rand() / (double)RAND_MAX) < 0.5);
				if(coinTossResult) {
					agRdSeg = (*i).first;
					ag = (*i).second;
				}
			}
			else if (minDistance > (*i).second->distanceToEndOfSegment) {
				minDistance = (*i).second->distanceToEndOfSegment;
				agRdSeg = (*i).first;
				ag = (*i).second;
			}
		}
	}
	if(ag) {
		candidateAgents[agRdSeg] = segmentAgents[agRdSeg]->getNext();
	}
	return ag;
}

unsigned int sim_mob::Conflux::numQueueingInSegment(const sim_mob::RoadSegment* rdSeg) {
	return segmentAgents[rdSeg]->numQueueingInSegment();
}
