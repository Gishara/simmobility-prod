//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/* 
 * File:   UnitHolder.hpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 *
 * Created on March 12, 2013, 2:36 PM
 */
#pragma once
#include <list>
#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>
#include "database/entity/Unit.hpp"

namespace sim_mob {

    namespace long_term {

        /**
         * Represents any entity that can hold one or more units.
         * The implementation is thread-safe.
         * 
         * Attention: this class will hold the ownership of the Unit instances.
         */
        class UnitHolder {
        public:
            UnitHolder(int id);
            virtual ~UnitHolder() = 0;

            /**
             * Adds new unit.
             * @param unit to add.
             * @return true if unit was added, false otherwise.
             */
            bool AddUnit(Unit* unit);

            /**
             * Removes an existing unit by given id.
             * Attention: The ownership of the object will pass to the caller.
             * @param id of the unit to remove.
             */
            Unit* RemoveUnit(UnitId id);

            /**
             * Verifies if exists any unit with given id.
             * @param id of the unit.
             * @return true if unit exists, false otherwise. 
             */
            bool HasUnit(UnitId id) const;

            /**
             * Gets the Unit pointer by given id.
             * @param id of the Unit to get.
             * @return Unit instance pointer or null,
             */
            Unit* GetUnitById(UnitId id);

            /**
             * Puts all units pointers on given list.
             * 
             * Attention: The ownership of the Unit objects 
             * are not passed to the caller.
             * 
             * @param outUnits list that will receive the objects.
             */
            void GetUnits(std::list<Unit*>& outUnits);

        protected:
            /**
             * Adds new unit.
             * @param unit to add.
             * @param reOwner tells if the call will change the 
             * owner of the property or not.
             * @return true if unit was added, false otherwise.
             */
            virtual bool Add(Unit* unit, bool reOwner);

            /**
             * Verifies if contains or not the unit.
             * @param id of the unit to verify.
             * @return true if contains the unit, false otherwise.
             */
            virtual bool Contains(UnitId id) const;

            /**
             * Removes the Unit by given id.
             * Attention: the unit will be managed by the caller.
             * @param id of the unit to remove.
             * @param reOwner re-owns or not the Unit.
             * @return removed Unit pointer or 
             *  null if the Unit does not exists.
             */
            virtual Unit* Remove(UnitId id, bool reOwner);

            /**
             * Gets the Unit by given id.
             * Attention: the unit continues to be managed by the container.
             * @param id of the Unit.
             * @return Unit pointer or 
             *  null if the Unit does not exists.
             */
            virtual Unit* GetById(UnitId id);
            
        private:
            typedef std::pair<UnitId, Unit*> HoldingUnitsEntry;
            typedef boost::unordered_map<UnitId, Unit*> HoldingUnits;
            HoldingUnits holdingUnits;
            int id;
            mutable boost::shared_mutex unitsListMutex;
        };
    }
}
