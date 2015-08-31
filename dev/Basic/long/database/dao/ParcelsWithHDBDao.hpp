/*
 * ParcelsWithHDBDao.hpp
 *
 *  Created on: Jun 30, 2015
 *      Author: gishara
 */
#pragma once

#pragma once
#include "database/dao/SqlAbstractDao.hpp"
#include "database/entity/ParcelsWithHDB.hpp"

namespace sim_mob {

    namespace long_term {

        /**
         * Data Access Object to getParcelsWithHDB function on data source.
         */
        class ParcelsWithHDBDao : public db::SqlAbstractDao<ParcelsWithHDB> {
        public:
        	ParcelsWithHDBDao(db::DB_Connection& connection);
            virtual ~ParcelsWithHDBDao();

        private:

            /**
             * Fills the given outObj with all values contained on Row.
             * @param result row with data to fill the out object.
             * @param outObj to fill.
             */
            void fromRow(db::Row& result, ParcelsWithHDB& outObj);

            /**
             * Fills the outParam with all values to insert or update on datasource.
             * @param data to get values.
             * @param outParams to put the data parameters.
             * @param update tells if operation is an Update or Insert.
             */
            void toRow(ParcelsWithHDB& data, db::Parameters& outParams, bool update);

        };
    }
}
