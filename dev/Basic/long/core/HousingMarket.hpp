//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/* 
 * File:   HousingMarket.hpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 *
 * Created on March 11, 2013, 4:13 PM
 */
#pragma once

#include "util/UnitHolder.hpp"
#include "entities/Entity.hpp"
#include "event/EventPublisher.hpp"

namespace sim_mob {

    namespace long_term {

        /**
         * Represents the housing market.
         * Th main responsibility is the management of: 
         *  - avaliable units
         */
        class HousingMarket : public UnitHolder, public sim_mob::Entity, 
                public sim_mob::event::EventPublisher {
        public:
            HousingMarket();
            virtual ~HousingMarket();

            /**
             * Inherited from Entity
             */
            virtual UpdateStatus update(timeslice now);

        protected:

            /**
             * Inherited from UnitHolder.
             */
            bool Add(Unit* unit, bool reOwner);
            Unit* Remove(UnitId id, bool reOwner);

            /**
             * Inherited from Entity
             */
            virtual bool isNonspatial();
            virtual void buildSubscriptionList(std::vector<sim_mob::BufferedBase*>& subsList);
            
        private:
            /**
             * Initializes the market.
             */
            void Setup();
            
        private:
            volatile bool firstTime;
        };
    }
}
