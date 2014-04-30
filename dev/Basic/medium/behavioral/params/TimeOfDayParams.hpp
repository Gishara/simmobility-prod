//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * TimeOfDayParams.hpp
 *
 *  Created on: Nov 21, 2013
 *      Author: Harish Loganathan
 */

#pragma once
#include <vector>

namespace sim_mob {
namespace medium {

/**
 * Simple class to store information pertaining tour time of day models
 * \note This class is used by the mid-term behavior models.
 *
 * \author Harish Loganathan
 */
class TourTimeOfDayParams {
public:
	TourTimeOfDayParams(std::vector<double>& travelTimesFirstHalfTour, std::vector<double>& travelTimesSecondHalfTour)
	: numTimeWindows(48), costHT1_AM(0), costHT1_PM(0), costHT1_OP(0), costHT2_AM(0), costHT2_PM(0), costHT2_OP(0),
	  travelTimesFirstHalfTour(travelTimesFirstHalfTour), travelTimesSecondHalfTour(travelTimesSecondHalfTour)
	{}
	virtual ~TourTimeOfDayParams() {}

	/**
	 * Function to get a specific element from first half tour vector travelTimesFirstHalfTour
	 *
	 * @param index time window of the day
	 * @returns travel time of the time window for first half tour
	 */
	double getTT_FirstHalfTour(int index) const{
		if(index < 1 || index > travelTimesFirstHalfTour.size()) {
			throw std::runtime_error("TourTimeOfDayParams::getTT_FirstHalfTour() - Travel time requested for invalid time window");
		}
		return travelTimesFirstHalfTour[index-1];
	}

	/**
	 * Function to get a specific element from second half tour vector travelTimesSecondHalfTour
	 *
	 * @param index time window of the day
	 * @returns travel time of the time window for second half tour
	 */
	double getTT_SecondHalfTour(int index) const{
		if(index < 1 || index > travelTimesSecondHalfTour.size()) {
			throw std::runtime_error("TourTimeOfDayParams::getTT_SecondHalfTour() - Travel time requested for invalid time window");
		}
		return travelTimesSecondHalfTour[index-1];
	}

	int getCostHt1Am() const {
		return costHT1_AM;
	}

	int getCostHt1Op() const {
		return costHT1_OP;
	}

	int getCostHt1Pm() const {
		return costHT1_PM;
	}

	int getCostHt2Am() const {
		return costHT2_AM;
	}

	int getCostHt2Op() const {
		return costHT2_OP;
	}

	int getCostHt2Pm() const {
		return costHT2_PM;
	}

	/**
	 * Vector storing the travel times for first and second half-tours in all half-hour windows within a day.
	 * The day starts at 0300Hrs and ends at 2659Hrs.
	 * The half-hour windows are 0300-0330, 0330-0400, 0400-0430, ... , 2600-2630, 2630-0300
	 * E.g.
	 * travelTimesFirstHalfTour[0] is the travel time for 0300hrs to 0330hrs (first half-hour window)
	 * travelTimesFirstHalfTour[47] is the travel time for 2630hrs to 0300hrs (last half-hour window)
	 *
	 */
	std::vector<double> travelTimesFirstHalfTour;
	std::vector<double> travelTimesSecondHalfTour;

private:
	int numTimeWindows;
	int costHT1_AM;
	int costHT1_PM;
	int costHT1_OP;
	int costHT2_AM;
	int costHT2_PM;
	int costHT2_OP;

};

/**
 * class to store parameters for the stop time of day model
 * \note This class is used by the mid-term behavior models.
 *
 * \author Harish Loganathan
 */
class StopTimeOfDayParams {
public:
	StopTimeOfDayParams(int stopType, int firstBound)
	: stopType(stopType), firstBound(firstBound), numTimeWindows(48), todHigh(0.0), todLow(0.0) {
		for(int i=1; i<=48; i++) {
			availability.push_back(true);
		}
	}

	virtual ~StopTimeOfDayParams() {}

	double getTimeWindow(int choice_idx) {
		//There are 48 time windows from 3.25 (index 1) to 26.75 (index 48).
		//We can get the time window by applying a simple calculation on the index value.
		return ((choice_idx * 0.5) + 2.75);
	}

	int getFirstBound() const {
		return firstBound;
	}

	int getSecondBound() const {
		return !firstBound;
	}

	int getStopType() const {
		return stopType;
	}

	double getTodHigh() const {
		return todHigh;
	}

	void setTodHigh(double todHigh) {
		this->todHigh = todHigh;
	}

	double getTodLow() const {
		return todLow;
	}

	void setTodLow(double todLow) {
		this->todLow = todLow;
	}

	/**
	 * Function to get a specific element from vector travelTimes
	 *
	 * @returns the value from the vector if index < 48; returns -1 otherwise;
	 * \note -1 is an invalid value for travel time and the caller must check for this value.
	 */
	double getTravelTime(unsigned index){
		if(index < numTimeWindows) {
			return travelTimes[index];
		}
		return -1;
	}

	/**
	 * Function to get a specific element from vector travelCost
	 *
	 * @returns the value from the vector if index < 48; returns -1 otherwise;
	 * \note -1 is an invalid value for travel time and the caller must check for this value.
	 */
	double getTravelCost(unsigned index){
		if(index < numTimeWindows) {
			return travelCost[index];
		}
		return -1;
	}

	/**
	 * Sets the availabilities of all time windows before low tod and after high tod to false
	 */
	void updateAvailabilities() {
		for(int i=0; i<availability.size(); i++) {
			int wndw = i+1;
			if(wndw <= todLow || wndw >= todHigh) {
				availability[i] = false;
			}
		}
	}

	/**
	 * Function to get the availability of an alternative
	 */
	int getAvailability(unsigned index){
		if(index < numTimeWindows && index > 0) {
			return availability[index-1];
		}
		return 0; // anything else is unavailable
	}

	/**
	 * Vectors storing travel times and travel costs for each half-hour time window from 3.25 to 26.75
	 *
	 * E.g.
	 * travelTimes[0] is the travel time for 3.25 window
	 * travelTimes[3] is the travel time for 4.75 window
	 * travelCost[47] is the travel cost for 26.75 window
	 */
	std::vector<double> travelTimes;
	std::vector<double> travelCost;
	std::vector<bool> availability;

private:
	int stopType;
	int firstBound;
	double todHigh; // upper limit for time of day for this stop
	double todLow; // lower limit for time of day for this stop
	int numTimeWindows;
};
} // end namespace medium
} // end namespace sim_mob