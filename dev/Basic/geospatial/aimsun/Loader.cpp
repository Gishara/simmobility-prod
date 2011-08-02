/* Copyright Singapore-MIT Alliance for Research and Technology */

#include "Loader.hpp"

//NOTE: Ubuntu is pretty bad about where it puts the SOCI headers.
//      "soci-postgresql.h" is supposed to be in "$INC/soci", but Ubuntu puts it in
//      "$INC/soci/postgresql". For now, I'm just referencing it manually, but
//      we might want to use something like pkg-config to manage header file directories
//      eventually.
#include "soci/soci.h"
#include "soci-postgresql.h"

#include "../Node.hpp"
#include "../Link.hpp"
#include "../RoadSegment.hpp"
#include "../LaneConnector.hpp"
#include "../RoadNetwork.hpp"

#include "Node.hpp"
#include "Section.hpp"
#include "Turning.hpp"
#include "Polyline.hpp"
#include "SOCI_Converters.hpp"


using namespace sim_mob::aimsun;
using std::vector;
using std::map;
using std::pair;
using std::multimap;


namespace {

void LoadNodes(soci::session& sql, map<int, Node>& nodelist)
{
	//Our SQL statement
	soci::rowset<Node> rs = (sql.prepare <<"select * from get_node()");

	//Exectue as a rowset to avoid repeatedly building the query.
	nodelist.clear();
	for (soci::rowset<Node>::const_iterator it=rs.begin(); it!=rs.end(); ++it)  {
		if (nodelist.count(it->id)>0) {
			throw std::runtime_error("Duplicate AIMSUN node.");
		}
		nodelist[it->id] = *it;
	}
}


void LoadSections(soci::session& sql, map<int, Section>& sectionlist, map<int, Node>& nodelist)
{
	//Our SQL statement
	soci::rowset<Section> rs = (sql.prepare <<"select * from get_section()");

	//Exectue as a rowset to avoid repeatedly building the query.
	sectionlist.clear();
	for (soci::rowset<Section>::const_iterator it=rs.begin(); it!=rs.end(); ++it)  {
		//Check nodes
		if(nodelist.count(it->TMP_FromNodeID)==0 || nodelist.count(it->TMP_ToNodeID)==0) {
			throw std::runtime_error("Invalid From or To node.");
		}

		//Note: Make sure not to resize the Node map after referencing its elements.
		it->fromNode = &nodelist[it->TMP_FromNodeID];
		it->toNode = &nodelist[it->TMP_ToNodeID];
		sectionlist[it->id] = *it;
	}
}


void LoadTurnings(soci::session& sql, map<int, Turning>& turninglist, map<int, Section>& sectionlist)
{
	//Our SQL statement
	soci::rowset<Turning> rs = (sql.prepare <<"select * from get_lane_connector()");

	//Exectue as a rowset to avoid repeatedly building the query.
	turninglist.clear();
	for (soci::rowset<Turning>::const_iterator it=rs.begin(); it!=rs.end(); ++it)  {
		//Check nodes
		bool fromMissing = sectionlist.count(it->TMP_FromSection)==0;
		bool toMissing = sectionlist.count(it->TMP_ToSection)==0;
		if(fromMissing || toMissing) {
			std::cout <<"Turning " <<it->id <<" skipped, missing section" <<(fromMissing&&toMissing?"s:  ":":  ");
			if (fromMissing) {
				std::cout <<it->TMP_FromSection <<(toMissing?", ":"\n");
			}
			if (toMissing) {
				std::cout <<it->TMP_ToSection <<"\n";
			}
			continue;
			//throw std::runtime_error("Invalid From or To section.");
		}

		//Note: Make sure not to resize the Section map after referencing its elements.
		it->fromSection = &sectionlist[it->TMP_FromSection];
		it->toSection = &sectionlist[it->TMP_ToSection];
		turninglist[it->id] = *it;
	}
}

void LoadPolylines(soci::session& sql, multimap<int, Polyline>& polylinelist, map<int, Section>& sectionlist)
{
	//Our SQL statement
	soci::rowset<Polyline> rs = (sql.prepare <<"select * from get_section_polyline()");

	//Exectue as a rowset to avoid repeatedly building the query.
	polylinelist.clear();
	for (soci::rowset<Polyline>::const_iterator it=rs.begin(); it!=rs.end(); ++it)  {
		//Check nodes
		if(sectionlist.count(it->TMP_SectionId)==0) {
			throw std::runtime_error("Invalid polyline section reference.");
		}

		//Note: Make sure not to resize the Section map after referencing its elements.
		it->section = &sectionlist[it->TMP_SectionId];
		polylinelist.insert(std::make_pair(it->section->id, *it));
		//polylinelist[it->id] = *it;
	}
}



void LoadBasicAimsunObjects(map<int, Node>& nodes, map<int, Section>& sections, map<int, Turning>& turnings, multimap<int, Polyline>& polylines)
{
	//Connect
	//NOTE: Our git repository is private (SSH-only) for now, so just storing the password to the DB here.
	soci::session sql(soci::postgresql, "host=localhost port=5432 dbname=SimMobility_DB user=postgres password=S!Mm0bility");

	//Load all nodes
	LoadNodes(sql, nodes);

	//Load all sections
	LoadSections(sql, sections, nodes);

	//Load all turnings
	LoadTurnings(sql, turnings, sections);

	//Load all polylines
	LoadPolylines(sql, polylines, sections);
}


void DecorateAndTranslateObjects(map<int, Node>& nodes, map<int, Section>& sections, map<int, Turning>& turnings, multimap<int, Polyline>& polylines)
{
	//Step 1: Tag all Nodes with the Sections that meet there.
	for (map<int,Section>::iterator it=sections.begin(); it!=sections.end(); it++) {
		it->second.fromNode->sectionsAtNode.push_back(&(it->second));
		it->second.toNode->sectionsAtNode.push_back(&(it->second));
	}

	//Step 2: Tag all Nodes that have only two Sections; these may become RoadSegmentNodes.
	for (map<int,Node>::iterator it=nodes.begin(); it!=nodes.end(); it++) {
		it->second.candidateForSegmentNode = it->second.sectionsAtNode.size()==2;
		if (it->second.candidateForSegmentNode && it->second.isIntersection) {
			throw std::runtime_error("Apparent RoadSegmentNode is tagged as an Intersection.");
		}
	}

	//Step 3: Tag all Sections with Turnings that apply to that Section
	for (map<int,Turning>::iterator it=turnings.begin(); it!=turnings.end(); it++) {
		it->second.fromSection->connectedTurnings.push_back(&(it->second));
		it->second.toSection->connectedTurnings.push_back(&(it->second));
	}

	//Step 4: Add polyline entries to Sections.
	for (map<int,Polyline>::iterator it=polylines.begin(); it!=polylines.end(); it++) {
		it->second.section->polylineEntries.push_back(&(it->second));
	}
}


void SaveSimMobilityNetwork(sim_mob::RoadNetwork& res, map<int, Node>& nodes, map<int, Section>& sections, map<int, Turning>& turnings, multimap<int, Polyline>& polylines)
{
	//First, Nodes. These match cleanly to the Sim Mobility data structures
	std::cout <<"Warning: Units are not considered when converting AIMSUN data.\n";
	for (map<int,Node>::iterator it=nodes.begin(); it!=nodes.end(); it++) {
		sim_mob::aimsun::Loader::ProcessNode(res, it->second);
	}

	//Next, Links and RoadSegments. See comments for our approach.
	for (map<int,Section>::iterator it=sections.begin(); it!=sections.end(); it++) {
		sim_mob::aimsun::Loader::ProcessSection(res, it->second);
	}
	//Scan the vector to see if any skipped Sections were not filled in later.
	for (map<int,Section>::iterator it=sections.begin(); it!=sections.end(); it++) {
		if (!it->second.hasBeenSaved) {
			throw std::runtime_error("Section was skipped.");
		}
	}

	//Next, Turnings. These generally match up.
	std::cout <<"Warning: Lanes-Left-of-Divider incorrect when converting AIMSUN data.\n";
	for (map<int,Turning>::iterator it=turnings.begin(); it!=turnings.end(); it++) {
		sim_mob::aimsun::Loader::ProcessTurning(res, it->second);
	}

}


} //End anon namespace




void sim_mob::aimsun::Loader::ProcessNode(sim_mob::RoadNetwork& res, Node& src)
{
	src.hasBeenSaved = true;

	sim_mob::Node* newNode = new sim_mob::Node();
	newNode->xPos = src.xPos;
	newNode->yPos = src.yPos;
	res.nodes.push_back(newNode);

	//For future reference
	src.generatedNode = newNode;

	//TODO: "SegmentNodes" should be built separately. They're still a type of Node.
}


void sim_mob::aimsun::Loader::ProcessSection(sim_mob::RoadNetwork& res, Section& src)
{
	//Skip Sections which start at a non-intersection. These will be filled in later.
	if (src.fromNode->candidateForSegmentNode) {
		return;
	}

	//Process this section, and continue processing Sections along the direction of
	// travel until one of these ends on an intersection.
	Section* currSection = &src;
	sim_mob::Link* ln = new sim_mob::Link();
	ln->roadName = currSection->roadName;
	ln->start = src.fromNode->generatedNode;
	//ln->end = nullptr;
	for (;;) {
		//Update
		ln->end = currSection->toNode->generatedNode;
		if (currSection->hasBeenSaved) {
			throw std::runtime_error("Section processed twice.");
		}
		currSection->hasBeenSaved = true;

		//Check name
		if (ln->roadName != currSection->roadName) {
			throw std::runtime_error("Road names don't match up on RoadSegments in the same Link.");
		}

		//Process
		sim_mob::RoadSegment* rs = new sim_mob::RoadSegment();
		rs->maxSpeed = currSection->speed;
		rs->length = currSection->length;
		for (size_t laneID=0; laneID<currSection->numLanes; laneID++) {
			rs->lanes.push_back(new sim_mob::Lane());
		}

		//TODO: How do we determine if lanesLeftOfDivider should be 0 or lanes.size()
		//      In other words, how do we apply driving direction?
		//      For now, setting to a clearly incorrect value.
		rs->lanesLeftOfDivider = 0xFF;

		currSection->fromNode->generatedNode->itemsAt.push_back(rs);
		currSection->toNode->generatedNode->itemsAt.push_back(rs);
		ln->segments.push_back(rs);

		//Save for later
		currSection->generatedSegment = rs;

		//Break?
		if (!currSection->toNode->candidateForSegmentNode) {
			break;
		}


		//Increment.
		Section* nextSection = nullptr;
		for (vector<Section*>::iterator it2=currSection->toNode->sectionsAtNode.begin(); it2!=currSection->toNode->sectionsAtNode.end(); it2++) {
			if ((*it2)->id!=currSection->id) {
				nextSection = *it2;
				break;
			}
		}
		if (!nextSection) {
			std::cout <<"PATH ERROR:\n";
			std::cout <<"  Starting at Node: " <<src.fromNode->id <<"\n";
			std::cout <<"  Currently at Node: " <<currSection->toNode->id <<"\n";
			throw std::runtime_error("No path reachable from RoadSegment.");
		}
		currSection = nextSection;
	}

	//Now add the link
	res.links.push_back(ln);
}




void sim_mob::aimsun::Loader::ProcessTurning(sim_mob::RoadNetwork& res, Turning& src)
{
	//Check
	if (src.fromSection->toNode->id != src.toSection->fromNode->id) {
		throw std::runtime_error("Turning doesn't match with Sections and Nodes.");
	}

	//Essentially, just expand each turning into a set of LaneConnectors.
	//TODO: This becomes slightly more complex at RoadSegmentNodes, since these
	//      only feature one primary connector per Segment pair.
	for (int fromLaneID=src.fromLane.first; fromLaneID<=src.fromLane.second; fromLaneID++) {
		for (int toLaneID=src.toLane.first; toLaneID<=src.toLane.second; toLaneID++) {
			sim_mob::LaneConnector* lc = new sim_mob::LaneConnector();
			lc->laneFrom.first = src.fromSection->generatedSegment;
			lc->laneFrom.second = fromLaneID;
			lc->laneTo.first = src.toSection->generatedSegment;
			lc->laneTo.second = toLaneID;
			src.fromSection->toNode->generatedNode->connectors.push_back(lc);
		}
	}


}





bool sim_mob::aimsun::Loader::LoadNetwork(sim_mob::RoadNetwork& rn)
{
	//try {
		//Temporary AIMSUN data structures
		map<int, Node> nodes;
		map<int, Section> sections;
		map<int, Turning> turnings;
		multimap<int, Polyline> polylines;

		//Step One: Load
		LoadBasicAimsunObjects(nodes, sections, turnings, polylines);

		//Step Two: Translate
		DecorateAndTranslateObjects(nodes, sections, turnings, polylines);

		//Step Three: Save
		SaveSimMobilityNetwork(rn, nodes, sections, turnings, polylines);



	/*} catch (std::exception& ex) {
		//TODO: We can provide ex.what() (the message) to the developer once
		//      we decide if our library will throw exceptions, use error codes,
		//      return pair<bool, string>, etc.
		std::cout <<"Error: " <<ex.what() <<std::endl;

		return false;
	}*/
	return true;
}

