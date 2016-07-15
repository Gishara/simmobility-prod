/*
 * SchoolAssignmentCoefficientsDao.cpp
 *
 *  Created on: 9 Mar 2016
 *      Author: gishara
 */

#include "SchoolAssignmentCoefficientsDao.hpp"
#include "DatabaseHelper.hpp"

using namespace sim_mob::db;
using namespace sim_mob::long_term;

SchoolAssignmentCoefficientsDao::SchoolAssignmentCoefficientsDao(DB_Connection& connection): SqlAbstractDao<SchoolAssignmentCoefficients>( connection, DB_TABLE_SCHOOL_ASSIGNMENT_COEFFICIENTS, EMPTY_STR, EMPTY_STR, EMPTY_STR,DB_GETALL_SCHOOL_ASSIGNMENT_COEFFICIENTS, EMPTY_STR ) {}

SchoolAssignmentCoefficientsDao::~SchoolAssignmentCoefficientsDao() {}

void SchoolAssignmentCoefficientsDao::fromRow(Row& result, SchoolAssignmentCoefficients& outObj)
{
    outObj.parameterId 		= result.get<BigSerial>("parameter_id",INVALID_ID);
    outObj.coefficientEstimate 		= result.get<double>("coeff_estimate",0.0);
}

void SchoolAssignmentCoefficientsDao::toRow(SchoolAssignmentCoefficients& data, Parameters& outParams, bool update) {}