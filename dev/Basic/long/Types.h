/* 
 * File:   Types.h
 * Author: Pedro Gandola
 *
 * Created on April 4, 2013, 2:04 PM
 */

#pragma once

#include "stddef.h"
#include "util/LangHelpers.hpp"

typedef int UnitId;

enum Sex {
    UNKNOWN_SEX = 0,
    MASCULINE = 1,
    FEMININE = 2
};

enum Race {
    UNKNOWN_RACE = 0,
    CHINISE = 1,
    MALAY = 2,
    INDIAN = 3,
    OTHER = 4,
};

enum EmploymentStatus {
    UNKNOWN_ESTATUS = -1,
    UNEMPLOYED = 0,
    EMPLOYED = 1
};

static Race ToRace(int value) {
    switch (value) {
        case CHINISE: return CHINISE;
        case MALAY: return MALAY;
        case INDIAN: return INDIAN;
        case OTHER: return OTHER;
        default: return UNKNOWN_RACE;
    }
}

static Sex ToSex(int value) {
    switch (value) {
        case MASCULINE: return MASCULINE;
        case FEMININE: return FEMININE;
        default: return UNKNOWN_SEX;
    }
}

static EmploymentStatus ToEmploymentStatus(int value) {
    switch (value) {
        case UNEMPLOYED: return UNEMPLOYED;
        case EMPLOYED: return EMPLOYED;
        default: return UNKNOWN_ESTATUS;
    }
}

