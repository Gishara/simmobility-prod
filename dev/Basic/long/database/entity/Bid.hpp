//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/* 
 * File:   Bid.hpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 *
 * Created on April 5, 2013, 5:03 PM
 */
#pragma once

#include "Common.hpp"
#include "Types.hpp"
#include "metrics/Frame.hpp"
#include "message/MessageHandler.hpp"

namespace sim_mob {

    namespace long_term {

        /**
         * Represents a bid to a unit.
         */
        class Bid {
        public:
            Bid(UnitId id, int bidderId, messaging::MessageHandler* bidder, float value, 
                    timeslice& time);
            Bid(const Bid& source);
            virtual ~Bid();

            /**
             * An operator to allow the bid copy.
             * @param source an Bid to be copied.
             * @return The Bid after modification
             */
            Bid& operator=(const Bid& source);

            /**
             * Gets the Unit unique identifier.
             * @return value with Unit identifier.
             */
            UnitId GetUnitId() const;

            /**
             * Gets the Bidder unique identifier.
             * @return value with Bidder identifier.
             */
            int GetBidderId() const;
            
            /**
             * Gets the Bidder pointer.
             * @return bidder pointer.
             */
            messaging::MessageHandler* GetBidder() const;

            /**
             * Gets the value of the bid.
             * @return the value of the bid.
             */
            float GetValue() const;
            
            /**
             * Gets the time of the bid.
             * @return the value of the bid.
             */
            const timeslice& GetTime() const;
            
            /**
             * Operator to print the Bid data.  
             */
            friend std::ostream& operator<<(std::ostream& strm, const Bid& data) {
                return strm << "{"
                        << "\"unitId\":\"" << data.unitId << "\","
                        << "\"bidderId\":\"" << data.bidderId << "\","
                        << "\"value\":\"" << data.value << "\","
                        << "\"day\":\"" << data.time.ms() << "\""
                        << "}";
            }
        private:
            //TODO: FUTURE friend class BidDao;
            /**
             * Private constructor for DAO use. 
             */
            Bid();
        private:
            timeslice time;
            UnitId unitId;
            int bidderId;
            float value;
            messaging::MessageHandler* bidder;
        };
    }
}
