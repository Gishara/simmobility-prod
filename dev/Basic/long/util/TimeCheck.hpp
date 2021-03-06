//Copyright (c) 2015 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//license.txt   (http://opensource.org/licenses/MIT)

/*
 * TimeCheck.hpp
 *
 *  Created on: 9 Dec 2015
 *  Author: Chetan Rogbeer <chetan.rogbeer@smart.mit.edu>
 */

#pragma once
#include <ctime>
#include <cmath>
#include <boost/chrono.hpp>
#include <boost/type_traits.hpp>


namespace sim_mob
{
    namespace long_term
    {
        class TimeCheck
        {
        public:
            TimeCheck();
            virtual ~TimeCheck();

            double getClockTime();
            double getClockTime_sec();
            double getProcessTime();

        private:

            time_t  start_clock;
            clock_t start_process;
            boost::chrono::system_clock::time_point start;
        };
    }
}



