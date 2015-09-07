//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * Taz.cpp
 *
 *  Created on: 23 Jun, 2015
 *  Author: Chetan Rogbeer <chetan.rogbeer@smart.mit.edu>
 */

#include <database/entity/Taz.hpp>
#include "util/Utils.hpp"

using namespace sim_mob::long_term;

Taz::Taz(BigSerial id, const std::string& name, float area, int surcharge): id(id), name(name), area(area), surcharge(surcharge) {}

Taz::~Taz() {}

BigSerial Taz::getId() const
{
    return id;
}

const std::string& Taz::getName() const
{
    return name;
}

void Taz::setName(const std::string& name)
{
    this->name = name;
}

float Taz::getArea() const
{
	return area;
}

void Taz::setArea(float value)
{
	area = value;
}

int Taz::getSurchage() const
{
	return surcharge;
}

void Taz::setSurchage( int value)
{
	surcharge = value;
}

namespace sim_mob
{
    namespace long_term
    {
        std::ostream& operator<<(std::ostream& strm, const Taz& data)
        {
            return strm << "{"
                    << "\"id\":\"" << data.id << "\","
                    << "\"name\":\"" << data.name << "\","
                    << "\"area\":\"" << data.area << "\","
                    << "\"surcharge\":\"" << data.surcharge << "\""
                    << "}";
        }
    }
}
