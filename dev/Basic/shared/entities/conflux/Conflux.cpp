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

void sim_mob::Conflux::addAgent(sim_mob::Agent* ag) {
	/**
	 * The agents always start at a node (for now).
	 * we will always add the agent to the road segment in "lane infinity".
	 */
	sim_mob::SegmentStats* rdSegStats = segmentAgents.at(ag->getCurrSegment());
	ag->setCurrLane(rdSegStats->laneInfinity);
	ag->distanceToEndOfSegment = ag->getCurrSegment()->computeLaneZeroLength();
	rdSegStats->addAgent(rdSegStats->laneInfinity, ag);
}

UpdateStatus sim_mob::Conflux::update(timeslice frameNumber) {
	currFrameNumber = frameNumber;

	resetPositionOfLastUpdatedAgentOnLanes();

	if (sim_mob::StreetDirectory::instance().signalAt(*multiNode) != nullptr) {
		updateUnsignalized(frameNumber); //TODO: Update Signalized must be implemented
	}
	else {
		updateUnsignalized(frameNumber);
	}
	updateSupplyStats(frameNumber);
	UpdateStatus retVal(UpdateStatus::RS_CONTINUE); //always return continue. Confluxes never die.
	return retVal;
}

void sim_mob::Conflux::updateSignalized() {
	throw std::runtime_error("Conflux::updateSignalized() not implemented yet.");
}

void sim_mob::Conflux::updateUnsignalized(timeslice frameNumber) {
	initCandidateAgents();
	sim_mob::Agent* ag = agentClosestToIntersection();
	while (ag) {
		updateAgent(ag);

		// get next agent to update
		ag = agentClosestToIntersection();
	}
}

void sim_mob::Conflux::updateAgent(sim_mob::Agent* ag) {
	const sim_mob::RoadSegment* segBeforeUpdate = ag->getCurrSegment();
	const sim_mob::Lane* laneBeforeUpdate = ag->getCurrLane();
	bool isQueuingBeforeUpdate = ag->isQueuing;
	sim_mob::SegmentStats* segStatsBfrUpdt = segmentAgents.find(segBeforeUpdate)->second;

	debugMsgs << "Updating Agent " << ag->getId() << std::endl;
	std::cout << debugMsgs.str();
	debugMsgs.str("");
	UpdateStatus res = ag->update(currFrameNumber);
	if (res.status == UpdateStatus::RS_DONE) {
		//This agent is done. Remove from simulation.
		killAgent(ag, segBeforeUpdate, laneBeforeUpdate);
		return;
	} else if (res.status == UpdateStatus::RS_CONTINUE) {
		// TODO: I think there will be nothing here. Have to make sure. ~ Harish
	} else {
		throw std::runtime_error("Unknown/unexpected update() return status.");
	}

	const sim_mob::RoadSegment* segAfterUpdate = ag->getCurrSegment();
	const sim_mob::Lane* laneAfterUpdate = ag->getCurrLane();
	bool isQueuingAfterUpdate = ag->isQueuing;
	sim_mob::SegmentStats* segStatsAftrUpdt = nullptr;
	if(segmentAgents.find(segAfterUpdate) != segmentAgents.end()) {
		segStatsAftrUpdt = segmentAgents[segAfterUpdate];
	}
	else if(segmentAgentsDownstream.find(segAfterUpdate) != segmentAgentsDownstream.end()) {
		segStatsAftrUpdt = segmentAgentsDownstream[segAfterUpdate];
	}

	if((segBeforeUpdate != segAfterUpdate) || (laneBeforeUpdate == segStatsBfrUpdt->laneInfinity && laneBeforeUpdate != laneAfterUpdate))
	{
		segStatsBfrUpdt->dequeue(laneBeforeUpdate);
		if(laneAfterUpdate) {
			segStatsAftrUpdt->addAgent(laneAfterUpdate, ag);
		}
		else {
			// If we don't know which lane the agent has to go to, we add him to lane infinity.
			// NOTE: One possible scenario for this is an agent who is starting on a new trip chain item.
			ag->setCurrLane(segStatsAftrUpdt->laneInfinity);
			ag->distanceToEndOfSegment = segAfterUpdate->computeLaneZeroLength();
			segStatsAftrUpdt->addAgent(segStatsAftrUpdt->laneInfinity, ag);
		}
	}
	else if (isQueuingBeforeUpdate != isQueuingAfterUpdate)
	{
		segStatsAftrUpdt->updateQueueStatus(laneAfterUpdate, ag);
	}

	// set the position of the last updated agent in his current lane (after update)
	segStatsAftrUpdt->setPositionOfLastUpdatedAgentInLane(ag->distanceToEndOfSegment, laneAfterUpdate);

}

double sim_mob::Conflux::getSegmentSpeed(const RoadSegment* rdSeg, bool hasVehicle){
	if (hasVehicle){
		if(getSegmentAgents().find(rdSeg) != getSegmentAgents().end()){
			return getSegmentAgents()[rdSeg]->getSegSpeed(hasVehicle);
		}
		else if(getSegmentAgentsDownstream().find(rdSeg) != getSegmentAgentsDownstream().end()){
			return getSegmentAgentsDownstream()[rdSeg]->getSegSpeed(hasVehicle);
		}
	}
	//else pedestrian lanes are not handled
	return 0.0;
}

void sim_mob::Conflux::initCandidateAgents() {
	candidateAgents.clear();
	resetCurrSegsOnUpLinks();
/*	for(std::map<sim_mob::Link*, const sim_mob::RoadSegment*>::iterator i = currSegsOnUpLinks.begin();
			i != currSegsOnUpLinks.end(); i++) {
		debugMsgs << "[" << i->second->getStart()->getID() << "," << i->second->getEnd()->getID() << "]";
	}
	debugMsgs << std::endl;
	std::cout << debugMsgs.str();
	debugMsgs.str("");
*/
	sim_mob::Link* lnk = nullptr;
	const sim_mob::RoadSegment* rdSeg = nullptr;
	for (std::map<sim_mob::Link*, const std::vector<sim_mob::RoadSegment*> >::iterator i = upstreamSegmentsMap.begin(); i != upstreamSegmentsMap.end(); i++) {
		lnk = i->first;
		int count = 0;
		while (currSegsOnUpLinks.find(lnk) != currSegsOnUpLinks.end() && currSegsOnUpLinks.at(lnk)) {
			count ++;
			rdSeg = currSegsOnUpLinks.at(lnk);

			// To be removed
			if(!rdSeg){
				throw std::runtime_error("Road Segment NULL");
			}
			segmentAgents.at(rdSeg)->resetFrontalAgents();
			candidateAgents.insert(std::make_pair(rdSeg,segmentAgents.at(rdSeg)->getNext()));
			if(!candidateAgents.at(rdSeg)) {
				// this road segment is deserted. search the next (which is, technically, the previous).
				const std::vector<sim_mob::RoadSegment*> segments = i->second; // or upstreamSegmentsMap.at(lnk);
				std::vector<sim_mob::RoadSegment*>::const_iterator rdSegIt = std::find(segments.begin(), segments.end(), rdSeg);
				currSegsOnUpLinks.erase(lnk);
				if(rdSegIt != segments.begin()) {
					rdSegIt--;
					currSegsOnUpLinks.insert(std::make_pair(lnk, *rdSegIt));
				}
				else {
					currSegsOnUpLinks.erase(lnk);
					const sim_mob::RoadSegment* nullSeg = nullptr;
					currSegsOnUpLinks.insert(std::make_pair(lnk, nullSeg));
				}
			}
			else { break; }
		}
	}
}

std::map<const sim_mob::Lane*, std::pair<unsigned int, unsigned int> > sim_mob::Conflux::getLanewiseAgentCounts(const sim_mob::RoadSegment* rdSeg) {
//	typedef std::pair<unsigned int, unsigned int> countPair;
//	countPair cP(0,0);
	std::map<const sim_mob::Lane*, std::pair<unsigned int, unsigned int> > laneCountsMap;
//	laneCountsMap = laneCountsMap()(0, cP);

	if(segmentAgents.find(rdSeg) != segmentAgents.end()){
		laneCountsMap = segmentAgents[rdSeg]->getAgentCountsOnLanes();
	}
	else if(segmentAgentsDownstream.find(rdSeg) != segmentAgentsDownstream.end()){
		laneCountsMap = segmentAgentsDownstream[rdSeg]->getAgentCountsOnLanes();
	}
	return laneCountsMap;
}

unsigned int sim_mob::Conflux::numMovingInSegment(const sim_mob::RoadSegment* rdSeg, bool hasVehicle) {
	unsigned int moving = 0;
	if(segmentAgents.find(rdSeg) != segmentAgents.end()){
		moving = segmentAgents[rdSeg]->numMovingInSegment(hasVehicle);
	}
	else if(segmentAgentsDownstream.find(rdSeg) != segmentAgentsDownstream.end()){
		moving = segmentAgentsDownstream[rdSeg]->numMovingInSegment(hasVehicle);
	}
	return moving;
}

void sim_mob::Conflux::resetCurrSegsOnUpLinks() {
	currSegsOnUpLinks.clear();
	for(std::map<sim_mob::Link*, const std::vector<sim_mob::RoadSegment*> >::iterator i = upstreamSegmentsMap.begin();
			i != upstreamSegmentsMap.end(); i++) {
		currSegsOnUpLinks.insert(std::make_pair(i->first, i->second.back()));
	}
}

sim_mob::Agent* sim_mob::Conflux::agentClosestToIntersection() {
	std::map<const sim_mob::RoadSegment*, sim_mob::Agent* >::iterator i = candidateAgents.begin();
	sim_mob::Agent* ag = nullptr;
	const sim_mob::RoadSegment* agRdSeg = nullptr;
	double minDistance = std::numeric_limits<double>::max();
	while(i != candidateAgents.end()) {
		if(i->second != nullptr) {
			if(minDistance == (i->second->distanceToEndOfSegment + lengthsOfSegmentsAhead[i->first])) {
				// If current ag and (*i) are at equal distance to the stop line, we toss a coin and choose one of them
				bool coinTossResult = ((rand() / (double)RAND_MAX) < 0.5);
				if(coinTossResult) {
					agRdSeg = i->first;
					ag = i->second;
				}
			}
			else if (minDistance > (i->second->distanceToEndOfSegment + lengthsOfSegmentsAhead[i->first])) {
				minDistance = i->second->distanceToEndOfSegment + lengthsOfSegmentsAhead[i->first];
				agRdSeg = i->first;
				ag = i->second;
			}
		}
		i++;
	}
	if(ag) {
		candidateAgents.erase(agRdSeg);
		const std::vector<sim_mob::RoadSegment*> segments = agRdSeg->getLink()->getSegments();
		sim_mob::Agent* nextAg = segmentAgents[agRdSeg]->getNext();
		std::vector<sim_mob::RoadSegment*>::const_iterator rdSegIt = std::find(segments.begin(), segments.end(), agRdSeg);
		while(!nextAg && rdSegIt != segments.begin()){
			rdSegIt--;
			nextAg = segmentAgents[*rdSegIt]->getNext();
		}
		candidateAgents[agRdSeg] = nextAg;
	}
	return ag;
}

void sim_mob::Conflux::prepareLengthsOfSegmentsAhead() {
	for(std::map<sim_mob::Link*, const std::vector<sim_mob::RoadSegment*> >::iterator i = upstreamSegmentsMap.begin();
				i != upstreamSegmentsMap.end(); i++)
	{
		for(std::vector<sim_mob::RoadSegment*>::const_iterator j = i->second.begin();
				j != i->second.end(); j++)
		{
			double lengthAhead = 0.0;
			for(std::vector<sim_mob::RoadSegment*>::const_iterator k = j+1;
					k != i->second.end(); k++)
			{
				lengthAhead = lengthAhead + (*k)->computeLaneZeroLength();
			}
			lengthsOfSegmentsAhead.insert(std::make_pair(*j, lengthAhead));
		}
	}
}

unsigned int sim_mob::Conflux::numQueueingInSegment(const sim_mob::RoadSegment* rdSeg,
		bool hasVehicle) {

	unsigned int queue = 0;
	if(segmentAgents.find(rdSeg) != segmentAgents.end()){
		queue = segmentAgents[rdSeg]->numQueueingInSegment(hasVehicle);
	}
	else if(segmentAgentsDownstream.find(rdSeg) != segmentAgentsDownstream.end()){
		queue = segmentAgentsDownstream[rdSeg]->numQueueingInSegment(hasVehicle);
	}
	return queue;
}

double sim_mob::Conflux::getOutputFlowRate(const Lane* lane) {
	double outputFlowRate = 0.0;
	if(segmentAgents.find(lane->getRoadSegment()) != segmentAgents.end()){
		outputFlowRate = segmentAgents[lane->getRoadSegment()]->getLaneParams(lane)->getOutputFlowRate();
	}
	else if(segmentAgentsDownstream.find(lane->getRoadSegment()) != segmentAgentsDownstream.end()){
		outputFlowRate = segmentAgentsDownstream[lane->getRoadSegment()]->getLaneParams(lane)->getOutputFlowRate();
	}
	return outputFlowRate;
}

int sim_mob::Conflux::getOutputCounter(const Lane* lane) {
	int outputCounter = 0;
	if(segmentAgents.find(lane->getRoadSegment()) != segmentAgents.end()){
		outputCounter = segmentAgents[lane->getRoadSegment()]->getLaneParams(lane)->getOutputCounter();
	}
	else if(segmentAgentsDownstream.find(lane->getRoadSegment()) != segmentAgentsDownstream.end()){
		outputCounter = segmentAgentsDownstream[lane->getRoadSegment()]->getLaneParams(lane)->getOutputCounter();
	}
	return outputCounter;
}

void sim_mob::Conflux::absorbAgentsAndUpdateCounts(sim_mob::SegmentStats* sourceSegStats) {
	if(segmentAgents.find(sourceSegStats->getRoadSegment()) != segmentAgents.end()
			&& sourceSegStats->hasAgents()) {
		segmentAgents[sourceSegStats->getRoadSegment()]->absorbAgents(sourceSegStats);
		std::map<const sim_mob::Lane*, std::pair<unsigned int, unsigned int> > laneCounts = segmentAgents[sourceSegStats->getRoadSegment()]->getAgentCountsOnLanes();
		sourceSegStats->setPrevTickLaneCountsFromOriginal(laneCounts);
		sourceSegStats->clear();
	}
}

double sim_mob::Conflux::getAcceptRate(const Lane* lane) {
	double accRate = 0.0;
	if(segmentAgents.find(lane->getRoadSegment()) != segmentAgents.end()){
		accRate = segmentAgents[lane->getRoadSegment()]->getLaneParams(lane)->getAcceptRate();
	}
	else if(segmentAgentsDownstream.find(lane->getRoadSegment()) != segmentAgentsDownstream.end()){
		accRate = segmentAgentsDownstream[lane->getRoadSegment()]->getLaneParams(lane)->getAcceptRate();
	}
	return accRate;
}

void sim_mob::Conflux::updateSupplyStats(const Lane* lane, double newOutputFlowRate) {
	if(segmentAgents.find(lane->getRoadSegment()) != segmentAgents.end()){
		segmentAgents[lane->getRoadSegment()]->updateLaneParams(lane, newOutputFlowRate);
	}
	else if(segmentAgentsDownstream.find(lane->getRoadSegment()) != segmentAgentsDownstream.end()){
		segmentAgentsDownstream[lane->getRoadSegment()]->updateLaneParams(lane, newOutputFlowRate);
	}
}

void sim_mob::Conflux::restoreSupplyStats(const Lane* lane) {
	if(segmentAgents.find(lane->getRoadSegment()) != segmentAgents.end()){
		segmentAgents[lane->getRoadSegment()]->restoreLaneParams(lane);
	}
	else if(segmentAgentsDownstream.find(lane->getRoadSegment()) != segmentAgentsDownstream.end()){
		segmentAgentsDownstream[lane->getRoadSegment()]->restoreLaneParams(lane);
	}
}

void sim_mob::Conflux::updateSupplyStats(timeslice frameNumber) {
	std::map<const sim_mob::RoadSegment*, sim_mob::SegmentStats*>::iterator it = segmentAgents.begin();
	for( ; it != segmentAgents.end(); ++it )
	{
		(it->second)->updateLaneParams(frameNumber);
		(it->second)->reportSegmentStats(frameNumber);
	}
}

std::pair<unsigned int, unsigned int> sim_mob::Conflux::getLaneAgentCounts(
		const sim_mob::Lane* lane) {

	std::pair<unsigned int, unsigned int> counts(0,0);

	if(segmentAgents.find(lane->getRoadSegment()) != segmentAgents.end()){
		counts = segmentAgents[lane->getRoadSegment()]->getLaneAgentCounts(lane);
	}
	else if(segmentAgentsDownstream.find(lane->getRoadSegment()) != segmentAgentsDownstream.end()){
		counts = segmentAgentsDownstream[lane->getRoadSegment()]->getLaneAgentCounts(lane);
	}
	return counts;
}

unsigned int sim_mob::Conflux::getInitialQueueCount(const Lane* lane) {
	unsigned int initQcount = 0;

	if(segmentAgents.find(lane->getRoadSegment()) != segmentAgents.end()){
		initQcount = segmentAgents[lane->getRoadSegment()]->getInitialQueueCount(lane);
	}
	else if(segmentAgentsDownstream.find(lane->getRoadSegment()) != segmentAgentsDownstream.end()){
		initQcount = segmentAgentsDownstream[lane->getRoadSegment()]->getInitialQueueCount(lane);
	}
	return initQcount;
}

void sim_mob::Conflux::killAgent(sim_mob::Agent* ag, const sim_mob::RoadSegment* prevRdSeg, const sim_mob::Lane* prevLane) {
	if(segmentAgents.find(prevRdSeg) != segmentAgents.end()){
		segmentAgents[prevRdSeg]->removeAgent(prevLane, ag);
	}
	else if(segmentAgentsDownstream.find(prevRdSeg) != segmentAgentsDownstream.end()){
		segmentAgentsDownstream[prevRdSeg]->removeAgent(prevLane, ag);
	}
}

void sim_mob::Conflux::handoverDownstreamAgents() {
	for(std::map<const sim_mob::RoadSegment*, sim_mob::SegmentStats*>::iterator i = segmentAgentsDownstream.begin();
			i != segmentAgentsDownstream.end(); i++)
	{
		i->first->getParentConflux()->absorbAgentsAndUpdateCounts(i->second);
	}
}

double sim_mob::Conflux::getLastAccept(const Lane* lane) {
	double lastAcc = 0.0;
	if(segmentAgents.find(lane->getRoadSegment()) != segmentAgents.end()){
		lastAcc = segmentAgents[lane->getRoadSegment()]->getLaneParams(lane)->getLastAccept();
	}
	else if(segmentAgentsDownstream.find(lane->getRoadSegment()) != segmentAgentsDownstream.end()){
		lastAcc = segmentAgentsDownstream[lane->getRoadSegment()]->getLaneParams(lane)->getLastAccept();
	}
	return lastAcc;
}

void sim_mob::Conflux::setLastAccept(const Lane* lane, double lastAcceptTime) {
	if(segmentAgents.find(lane->getRoadSegment()) != segmentAgents.end()){
		segmentAgents[lane->getRoadSegment()]->getLaneParams(lane)->setLastAccept(lastAcceptTime);
	}
	else if(segmentAgentsDownstream.find(lane->getRoadSegment()) != segmentAgentsDownstream.end()){
		segmentAgentsDownstream[lane->getRoadSegment()]->getLaneParams(lane)->setLastAccept(lastAcceptTime);
	}
}

void sim_mob::Conflux::resetPositionOfLastUpdatedAgentOnLanes() {
	std::map<const sim_mob::RoadSegment*, sim_mob::SegmentStats*>::iterator it = segmentAgents.begin();
	for( ; it != segmentAgents.end(); ++it )
	{
		(it->second)->resetPositionOfLastUpdatedAgentOnLanes();
	}
}