//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//license.txt   (http://opensource.org/licenses/MIT)


/*
 * HedonicPriceSubModel.cpp
 *
 *  Created on: 24 Dec 2015
 *  Author: chetan rogbeer <chetan.rogbeer@smart.mit.edu>
 */

#include <model/HedonicPriceSubModel.hpp>
#include "model/lua/LuaProvider.hpp"
#include <limits>
#include "core/DataManager.hpp"
#include <util/PrintLog.hpp>

using namespace sim_mob::long_term;


HedonicPrice_SubModel::HedonicPrice_SubModel(double _hedonicPrice, double _lagCoefficient, double _day, HM_Model *_hmModel,DeveloperModel * _devModel, Unit *_unit, double logsum)
                                            : hedonicPrice(_hedonicPrice), lagCoefficient(_lagCoefficient), day(_day), hmModel(_hmModel), devModel(_devModel), unit(_unit), logsum(logsum) {}

HedonicPrice_SubModel::HedonicPrice_SubModel( double _day, HM_Model *_hmModel, Unit *_unit)
                                            : hedonicPrice(0), lagCoefficient(0), day(_day), hmModel(_hmModel), devModel(_hmModel->getDeveloperModel()), unit(_unit), logsum(0) {}


HedonicPrice_SubModel::~HedonicPrice_SubModel() {}

double HedonicPrice_SubModel::ComputeLagCoefficient()
{
    //Current Quarter
    int currentQuarter = int((int(day) % 365) / 365.0 * 4.0) + 1;

    ConfigParams& config = ConfigManager::GetInstanceRW().FullConfig();
    std::string quarterStr = boost::lexical_cast<std::string>(config.ltParams.year)+"Q"+boost::lexical_cast<std::string>(currentQuarter);

    double lagCoefficient;
    double finalCoefficient = 0;

    if( unit->getUnitType() < ID_HDB3 )
    {
        lagCoefficient =  devModel->getTaoByQuarter(quarterStr)->getHdb12();

        const LagPrivateT *lag = devModel->getLagPrivateTByPropertyTypeId(7);

        finalCoefficient = (lagCoefficient * lag->getT4()) + lag->getIntercept();
    }

    else if( unit->getUnitType() == ID_HDB3 )
    {
        lagCoefficient =  devModel->getTaoByQuarter(quarterStr)->getHdb3();

        const LagPrivateT *lag = devModel->getLagPrivateTByPropertyTypeId(8);

        finalCoefficient = (lagCoefficient * lag->getT4()) + lag->getIntercept();
    }
    else if( unit->getUnitType() == ID_HDB4 )
    {
        lagCoefficient = devModel->getTaoByQuarter(quarterStr)->getHdb4();

        const LagPrivateT *lag = devModel->getLagPrivateTByPropertyTypeId(9);

        finalCoefficient = (lagCoefficient * lag->getT4()) + lag->getIntercept();
    }
    else if( unit->getUnitType() == ID_HDB5 || unit->getUnitType() == 6 || unit->getUnitType() == 65 )
    {
        lagCoefficient = devModel->getTaoByQuarter(quarterStr)->getHdb5();

        const LagPrivateT *lag = devModel->getLagPrivateTByPropertyTypeId(10);

        finalCoefficient = (lagCoefficient * lag->getT4()) + lag->getIntercept();
    }
    else if( unit->getUnitType() >= ID_EC85 and unit->getUnitType()  <= ID_EC144 )  //Executive Condominium
    {
        lagCoefficient = devModel->getTaoByQuarter(quarterStr)->getEc();

        const LagPrivateT *lag = devModel->getLagPrivateTByPropertyTypeId(11);

        finalCoefficient = (lagCoefficient * lag->getT4()) + lag->getIntercept();
    }
    else if( ( unit->getUnitType() >= ID_CONDO60 && unit->getUnitType()  <= ID_CONDO134 ) ||
             ( unit->getUnitType() >= 37 && unit->getUnitType() <= 51 ) || unit->getUnitType() == 64 ) //Condominium and mixed use
    {
        lagCoefficient = devModel->getTaoByQuarter(quarterStr)->getCondo();

        const LagPrivateT *lag = devModel->getLagPrivateTByPropertyTypeId(1);

        finalCoefficient = (lagCoefficient * lag->getT4()) + lag->getIntercept();

    }
    else if(unit->getUnitType() >= ID_APARTM70 && unit->getUnitType()  <= ID_APARTM159 ) //"Apartment"
    {
        lagCoefficient = devModel->getTaoByQuarter(quarterStr)->getApartment();

        const LagPrivateT *lag = devModel->getLagPrivateTByPropertyTypeId(2);

        finalCoefficient = (lagCoefficient * lag->getT4()) + lag->getIntercept();
    }
    else if(unit->getUnitType() >= ID_TERRACE180 && unit->getUnitType()  <= ID_TERRACE379 )  //"Terrace House"
    {
        lagCoefficient = devModel->getTaoByQuarter(quarterStr)->getTerrace();

        const LagPrivateT *lag = devModel->getLagPrivateTByPropertyTypeId(3);

        finalCoefficient = (lagCoefficient * lag->getT4()) + lag->getIntercept();
    }
    else if( unit->getUnitType() >= ID_SEMID230 && unit->getUnitType()  <= ID_SEMID499 )  //"Semi-Detached House"
    {
        lagCoefficient = devModel->getTaoByQuarter(quarterStr)->getSemi();

        const LagPrivateT *lag = devModel->getLagPrivateTByPropertyTypeId(4);

        finalCoefficient = (lagCoefficient * lag->getT4()) + lag->getIntercept();
    }
    else if( unit->getUnitType() >= ID_DETACHED480 && unit->getUnitType()  <= ID_DETACHED1199 )  //"Detached House"
    {
        lagCoefficient =  devModel->getTaoByQuarter(quarterStr)->getDetached();

        const LagPrivateT *lag = devModel->getLagPrivateTByPropertyTypeId(5);

        finalCoefficient = (lagCoefficient * lag->getT4()) + lag->getIntercept();
    }

    return finalCoefficient;
}

void HedonicPrice_SubModel::ComputeHedonicPrice( HouseholdSellerRole::SellingUnitInfo &info, HouseholdSellerRole::UnitsInfoMap &sellingUnitsMap, BigSerial agentId)
{
    double finalCoefficient = ComputeLagCoefficient();

    unit->setLagCoefficient(finalCoefficient);
    lagCoefficient = finalCoefficient;

    info.numExpectations = (info.interval == 0) ? 0 : ceil((double) info.daysOnMarket / (double) info.interval);

    ComputeExpectation(info.numExpectations, info.expectations);

    //number of expectations should match
    if (info.expectations.size() == info.numExpectations)
    {
        sellingUnitsMap.erase(unit->getId());
        sellingUnitsMap.insert(std::make_pair(unit->getId(), info));

        //just revert the expectations order.
        for (int i = 0; i < info.expectations.size() ; i++)
        {
            int dayToApply = day + (i * info.interval);
            printExpectation( day, dayToApply, unit->getId(), agentId, info.expectations[i]);
        }
    }
    else
    {
        AgentsLookupSingleton::getInstance().getLogger().log(LoggerAgent::LOG_ERROR, (boost::format( "[unit %1%] Expectations is empty.") % unit->getId()).str());
    }
}

void HedonicPrice_SubModel::ComputeHedonicPrice( RealEstateSellerRole::SellingUnitInfo &info, RealEstateSellerRole::UnitsInfoMap &sellingUnitsMap, BigSerial agentId)
{
    double finalCoefficient = ComputeLagCoefficient();

        unit->setLagCoefficient(finalCoefficient);
        lagCoefficient = finalCoefficient;

        info.numExpectations = (info.interval == 0) ? 0 : ceil((double) info.daysOnMarket / (double) info.interval);

        ComputeExpectation(info.numExpectations, info.expectations);

        //number of expectations should match
        if (info.expectations.size() == info.numExpectations)
        {
            sellingUnitsMap.erase(unit->getId());
            sellingUnitsMap.insert(std::make_pair(unit->getId(), info));

            //just revert the expectations order.
            for (int i = 0; i < info.expectations.size() ; i++)
            {
                int dayToApply = day + (i * info.interval);
                printExpectation( day, dayToApply, unit->getId(), agentId, info.expectations[i]);
            }
        }
        else
        {
            AgentsLookupSingleton::getInstance().getLogger().log(LoggerAgent::LOG_ERROR, (boost::format( "[unit %1%] Expectations is empty.") % unit->getId()).str());
        }
}


void HedonicPrice_SubModel::ComputeExpectation( int numExpectations, std::vector<ExpectationEntry> &expectations )
{
    const HM_LuaModel& luaModel = LuaProvider::getHM_Model();

    BigSerial tazId = 0;
    BigSerial addressId = 0;
    if(unit->isUnitByDevModel())
    {
        tazId = unit->getTazIdByDevModel();
        const Postcode *postcode = devModel->getPostcodeByTaz(tazId);
        if(postcode != nullptr)
        {
            addressId = devModel->getPostcodeByTaz(tazId)->getAddressId();
        }
        else
        {
            return;
        }

    }
    else
    {
        tazId = hmModel->getUnitTazId( unit->getId() );
        addressId = hmModel->getUnitSlaAddressId( unit->getId() );
    }

    double logsum = hmModel->ComputeHedonicPriceLogsumFromDatabase(tazId);

    lagCoefficient = ComputeLagCoefficient();

    if( logsum < 0.0000001)
        AgentsLookupSingleton::getInstance().getLogger().log(LoggerAgent::LOG_ERROR, (boost::format( "LOGSUM FOR UNIT %1% is 0.") %  unit->getId()).str());

    const Building *building = DataManagerSingleton::getInstance().getBuildingById(unit->getBuildingId());

    const Postcode *postcode = DataManagerSingleton::getInstance().getPostcodeById(addressId);

    const PostcodeAmenities *amenities = DataManagerSingleton::getInstance().getAmenitiesById(addressId);

    expectations = CalculateUnitExpectations(unit, numExpectations, logsum, lagCoefficient, building, postcode, amenities);
}

void HedonicPrice_SubModel::computeInitialHedonicPrice(BigSerial unitIdFromModel)
{
    static bool wasExecuted = false;
    if (!wasExecuted)
    {
        wasExecuted = true;
        ConfigParams& config = ConfigManager::GetInstanceRW().FullConfig();

        DB_Config dbConfig(LT_DB_CONFIG_FILE);
        dbConfig.load();

        // Connect to database and load data for this model.
        DB_Connection conn(sim_mob::db::POSTGRES, dbConfig);
        conn.setSchema(config.schemas.main_schema);
        conn.connect();

        DB_Connection conn_calibration(sim_mob::db::POSTGRES, dbConfig);
        conn_calibration.setSchema(config.schemas.calibration_schema);
        conn_calibration.connect();

        devModel->loadTAO(conn_calibration);
        devModel->loadHedonicCoeffsByUnitType(conn);
        devModel->loadPrivateLagT(conn);
        devModel->loadPrivateLagTByUT(conn);
        devModel->loadTaoByUnitType(conn);
    }
    Unit *unitFromModel = hmModel->getUnitById(unitIdFromModel);
    BigSerial tazId = hmModel->getUnitTazId( unitIdFromModel );
    double logsum = hmModel->ComputeHedonicPriceLogsumFromDatabase( tazId );

    double lagCoeff = ComputeLagCoefficient();

    if( logsum < 0.0000001)
        AgentsLookupSingleton::getInstance().getLogger().log(LoggerAgent::LOG_ERROR, (boost::format( "LOGSUM FOR UNIT %1% is 0.") %  unitIdFromModel).str());

    const Building *building = DataManagerSingleton::getInstance().getBuildingById(unit->getBuildingId());

    BigSerial addressId = hmModel->getUnitSlaAddressId( unit->getId() );

    const Postcode *postcode = DataManagerSingleton::getInstance().getPostcodeById(addressId);

    const PostcodeAmenities *amenities = DataManagerSingleton::getInstance().getAmenitiesById(addressId);

    double  hedonicPrice = CalculateHedonicPrice(unitFromModel, building, postcode, amenities, logsum, lagCoeff);


    hedonicPrice = exp( hedonicPrice ) / 1000000.0;

    if (hedonicPrice > 0)
    {
        writeUnitHedonicPriceToFile(unitIdFromModel,hedonicPrice);
    }
}

double HedonicPrice_SubModel::CalculateHDB_HedonicPrice(Unit *unit, const Building *building, const Postcode *postcode, const PostcodeAmenities *amenities, double logsum, double lagCoefficient)
{
    int simulationYear = HITS_SURVEY_YEAR;
    float hedonicPrice = 0;

    float ZZ_pms1km   = 0;
    float ZZ_mrt_200m = 0;
    float ZZ_mrt_400m = 0;
    float ZZ_bus_200m = 0;
    float ZZ_bus_400m = 0;
    float ZZ_express_200m = 0;

    float ZZ_logsum = logsum;

    float ZZ_hdb12 = 0;
    float ZZ_hdb3  = 0;
    float ZZ_hdb4  = 0;
    float ZZ_hdb5m = 0;

    float age = (HITS_SURVEY_YEAR - 1900) - unit->getOccupancyFromYear();

    if( age < 0 )
        age = 0;


    float ageSquared = age * age;

    double DD_logsqrtarea = log( unit->getFloorArea());
    double ZZ_dis_cbd  = amenities->getDistanceToCBD();
    double ZZ_dis_mall = amenities->getDistanceToMall();


    float distancePMS = amenities->getDistanceToPMS30();
    if(distancePMS > 100)
    {
        distancePMS = distancePMS/1000.0;
    }

    if( distancePMS <= 1 )
        ZZ_pms1km = 1;


    float distanceMRT = amenities->getDistanceToMRT();
    if(distanceMRT > 100)
    {
        distanceMRT = distanceMRT/1000.0;
    }

    if( distanceMRT <= 0.2)
    {
        ZZ_mrt_200m = 1;
    }
    else if(distanceMRT <=0.4)
    {
        ZZ_mrt_400m = 1;
    }

    float distanceExpress = amenities->getDistanceToExpress();
    if(distanceExpress > 100)
    {
        distanceExpress = distanceExpress/1000.0;
    }

    if( distanceExpress <= 0.2 )
        ZZ_express_200m = 1;

    float distanceBus = amenities->getDistanceToBus();

    if( distanceBus <= 0.2 )
    {
        ZZ_bus_200m = 1;
    }
    else if(distanceBus <= 0.4)
    {
        ZZ_bus_400m = 1;
    }


    UnitType *unitType = hmModel->getUnitTypeById(unit->getUnitType());
    HedonicCoeffsByUnitType *coeffs = const_cast<HedonicCoeffsByUnitType*>(devModel->getHedonicCoeffsByUnitTypeId(unitType->getAggregatedUnitType()));;
    BigSerial tazId = hmModel->getUnitTazId( unit->getId() );
    Taz* unitTaz =  hmModel->getTazById(tazId);
    float otherMature = 0;
    float nonMature = 0;

    if (unitTaz->getHdbTownType().compare("other-mature")==0)
    {
        otherMature = 1.0;
    }
    else if(unitTaz->getHdbTownType().compare("non-mature")==0)
    {
        nonMature = 1.0;
    }

    float storey = unit->getStorey();

    hedonicPrice =  coeffs->getIntercept()  +
                    coeffs->getLogArea()        *   DD_logsqrtarea  +
                    coeffs->getLogsumWeighted() *   ZZ_logsum       +
                    coeffs->getPms1km()         *   ZZ_pms1km       +
                    coeffs->getDistanceMallKm() *   ZZ_dis_mall     +
                    coeffs->getMrt200m()        *   ZZ_mrt_200m     +
                    coeffs->getMrt2400m()       *   ZZ_mrt_400m     +
                    coeffs->getExpress200m()    *   ZZ_express_200m +
                    coeffs->getBus2400m()       *   ZZ_bus_400m     +
                    coeffs->getAge()            *   age             +
                    coeffs->getAgeSquared()     *   ageSquared      +
                    coeffs->getNonMature()      *   nonMature       +
                    coeffs->getOtherMature()    *   otherMature     +
                    coeffs->getStorey()         * storey            ;



    hedonicPrice = hedonicPrice + lagCoefficient;
    if(hedonicPrice == 0)
    {
        PrintOutV("hedonic price is 0 for"<< unit->getId()<<std::endl);
    }

    return hedonicPrice;
}

/*
--[[
    Calculates the hedonic price for the given private Unit.
    Following the documentation prices are in (SGD per sqm).

    @param unit to calculate the hedonic price.
    @param building where the unit belongs
    @param postcode of the unit.
    @param amenities close to the unit.
    @return hedonic price value.
]]
*/

double HedonicPrice_SubModel::CalculatePrivate_HedonicPrice( Unit *unit, const Building *building, const Postcode *postcode, const PostcodeAmenities *amenities, double logsum, double lagCoefficient)
{
    double hedonicPrice = 0;
    double DD_logarea  = 0;
    double ZZ_dis_cbd  = 0;
    double ZZ_pms1km   = 0;
    double ZZ_dis_mall = 0;
    double ZZ_mrt_200m = 0;
    double ZZ_mrt_400m = 0;
    double ZZ_express_200m = 0;
    double ZZ_bus_2400m = 0;
    double ZZ_freehold = 0;
    double ZZ_logsum = logsum;
    double ZZ_bus_gt400m = 0;

    ZZ_freehold = building->getFreehold();

    double misage = 0;
    double age  = 0;

    if( (unit->getOccupancyFromDate().tm_year == 8099)|| (unit->getOccupancyFromDate().tm_year == 0))
    {
        misage = 1;
    }
    else
    {
        age= HITS_SURVEY_YEAR  - 1900  - unit->getOccupancyFromDate().tm_year;
    }


    if( age > 50 )
    {
        age = 50;
    }

    if( age < 0 )
    {
        age = 0;
        misage = 1.0;
    }

    double  ageSquared =  age *  age;

    DD_logarea  = log(unit->getFloorArea());
    ZZ_dis_cbd  = amenities->getDistanceToCBD();
    ZZ_dis_mall = amenities->getDistanceToMall();

    if( amenities->getDistanceToPMS30() < 1 )
        ZZ_pms1km = 1;


    if( amenities->getDistanceToMRT() < 0.200 )
    {
        ZZ_mrt_200m = 1;
    }
    else if( amenities->getDistanceToMRT() > 0.200 && amenities->getDistanceToMRT() < 0.400 )
    {
        ZZ_mrt_400m = 1;
    }


    if( amenities->getDistanceToExpress() < 0.200 )
        ZZ_express_200m = 1;


    if( amenities->getDistanceToBus() < 0.200 &&  amenities->getDistanceToBus() < 0.400 )
    {
        ZZ_bus_2400m = 1;
    }
    else if( amenities->getDistanceToBus() > 0.400 )
    {
        ZZ_bus_gt400m = 1;
    }

    bool condoApartment = false;
    UnitType *unitType = hmModel->getUnitTypeById(unit->getUnitType());
    HedonicCoeffsByUnitType *coeffsByUT = const_cast<HedonicCoeffsByUnitType*>(devModel->getHedonicCoeffsByUnitTypeId(unitType->getAggregatedUnitType()));

    float storey = unit->getStorey();
    hedonicPrice =  coeffsByUT->getIntercept()  +
            coeffsByUT->getLogArea()        *   DD_logarea      +
            coeffsByUT->getFreehold()       *   ZZ_freehold     +
            coeffsByUT->getLogsumWeighted() *   ZZ_logsum       +
            coeffsByUT->getPms1km()         *   ZZ_pms1km       +
            coeffsByUT->getDistanceMallKm() *   ZZ_dis_mall     +
            coeffsByUT->getMrt200m()        *   ZZ_mrt_200m     +
            coeffsByUT->getMrt2400m()       *   ZZ_mrt_400m     +
            coeffsByUT->getExpress200m()    *   ZZ_express_200m +
            coeffsByUT->getBus2400m()       *   ZZ_bus_2400m    +
            coeffsByUT->getBusGt400m()      *   ZZ_bus_gt400m   +
            coeffsByUT->getAge()            *   age             +
            coeffsByUT->getAgeSquared()     *   ageSquared      +
            coeffsByUT->getMisage()         *   misage          +
            coeffsByUT->getStorey()         * storey            +
            coeffsByUT->getStoreySquared()  *  (storey * storey);


    hedonicPrice = hedonicPrice + lagCoefficient;
    if(hedonicPrice == 0)
    {
        PrintOutV("hedonic price is 0 for"<< unit->getId()<<std::endl);
    }

    return hedonicPrice;
}

/*
--[[
    Calculates the hedonic price for the given Unit.
    Following the documentation prices are in (SGD per sqm).

    @param unit to calculate the hedonic price.
    @param building where the unit belongs
    @param postcode of the unit.
    @param amenities close to the unit.
]]
*/

double HedonicPrice_SubModel::CalculateHedonicPrice( Unit *unit, const Building *building, const Postcode *postcode, const PostcodeAmenities *amenities, double logsum, double lagCoefficient )
{
    if( unit != nullptr && building != nullptr && postcode != nullptr && amenities != nullptr )
    {
        if(unit->getUnitType() <= 6 || unit->getUnitType() == 65 )
            return CalculateHDB_HedonicPrice(unit, building, postcode, amenities, logsum, lagCoefficient);
         else
            return CalculatePrivate_HedonicPrice(unit, building, postcode, amenities, logsum, lagCoefficient);
    }

    return -1;

}

/*
--[[
    Calculates a single expectation based on given params.

    @param price of the unit.
    @param v is the last expectation.
    @param a is the ratio of events expected by the seller.
    @param b is the importance of the price for seller.
    @param cost
    @return expectation value.
]]
*/

static double CalculateExpectation(double price, double v, double a, double b, double cost)
{
    double E = exp(1.0);

    double rateOfBuyers = a - (b * price);

    //--local expectation = price
    //--                    + (math.pow(E,-rateOfBuyers*(price-v)/price)-1 + rateOfBuyers)*price/rateOfBuyers
    //--                    + math.pow(E,-rateOfBuyers)*v
    //--                    - cost

    if (rateOfBuyers > 0)
    {
        double expectation = price + (pow(E,-rateOfBuyers * (price - v) / price) - 1) * price / rateOfBuyers - cost;
        return expectation;
    }


    return v;
}


/*
--[[
    Calculates seller expectations for given unit based on timeOnMarket
    that the seller is able to wait until sell the unit.

    @param unit to sell.
    @param timeOnMarket number of expectations which are necessary to calculate.
    @param building where the unit belongs
    @param postcode of the unit.
    @param amenities close to the unit.
    @return array of ExpectationEntry's with N expectations (N should be equal to timeOnMarket).
]]
*/

vector<ExpectationEntry> HedonicPrice_SubModel::CalculateUnitExpectations (Unit *unit, double timeOnMarket, double logsum, double lagCoefficient, const Building *building, const Postcode *postcode, const PostcodeAmenities *amenitiesX)
{
    vector<ExpectationEntry> expectations;
    //-- HEDONIC PRICE in SGD in thousands with average hedonic price (500)

    PostcodeAmenities amenities = *amenitiesX;
    int tazId = amenities.getTazId();


    const ConfigParams& config = ConfigManager::GetInstance().FullConfig();


    if( config.ltParams.scenario.enabled )
    {
        std::multimap<string, StudyArea*> scenario = hmModel->getStudyAreaByScenarioName();
        auto itr_range = scenario.equal_range( config.ltParams.scenario.scenarioName );

        bool bHomeTaz = false;

        int dist = distance(itr_range.first, itr_range.second);

        tazId = hmModel->getUnitTazId( unit->getId() );

        for(auto itr = itr_range.first; itr != itr_range.second; itr++)
        {
            if( itr->second->getFmTazId()  == tazId )
                bHomeTaz = true;
        }

        if( bHomeTaz)
        {
            amenities.setDistanceToJob(amenities.getDistanceToJob() );
            amenities.setDistanceToMall(amenities.getDistanceToMall() / 2.0);
            amenities.setDistanceToCbd(amenities.getDistanceToCBD() );
            amenities.setDistanceToPms30(amenities.getDistanceToPMS30() / 2.0);
            amenities.setDistanceToExpress(amenities.getDistanceToExpress() );
            amenities.setDistanceToBus(amenities.getDistanceToBus() / 2.0);
            amenities.setDistanceToMrt(amenities.getDistanceToMRT() / 2.0);


            logsum += halfStandardDeviation;
        }


    }


    double  hedonicPrice = CalculateHedonicPrice(unit, building, postcode, &amenities, logsum, lagCoefficient);


    hedonicPrice = exp( hedonicPrice ) / 1000000.0;
    if(hedonicPrice < 0.01)
    {
        printError((boost::format("hedonic price is 0 for unit %1%") % unit->getId()).str());
    }



    if (hedonicPrice > 0)
    {
        double reservationPrice = hedonicPrice * 0.8; //  -- IMPORTANT : The reservation price should be less than the hedonic price and the asking price
        double a = 0; // -- ratio of events expected by the seller per (considering that the price is 0)
        double b = 1; // -- Importance of the price for seller.
        double cost = 0.0; // -- Cost of being in the market
        double x0 = 0; // -- starting point for price search
        double crit = 0.0001; // -- criteria
        double maxIterations = 20; // --number of iterations

        const double lowerBound = 0.85;

        for(int i=1; i <= timeOnMarket; i++)
        {
            ExpectationEntry entry = ExpectationEntry(); //--entry is a class initialized to 0, that will hold the hedonic, asking and target prices.

            if( unit->isBto() )
            {
                entry.hedonicPrice = unit->getBTOPrice();
                entry.askingPrice = unit->getBTOPrice();
                entry.targetPrice = unit->getBTOPrice();
            }
            else
            {
                /*
                 a = 1.5 * reservationPrice;
                 x0 = 1.4 * reservationPrice;

                 entry.hedonicPrice = hedonicPrice;
                 entry.askingPrice = FindMaxArgConstrained(CalculateExpectation, x0, reservationPrice, a, b, cost, crit, maxIterations, reservationPrice, 1.2 * reservationPrice );
                 entry.targetPrice = CalculateExpectation(entry.askingPrice, reservationPrice, a, b, cost );

                 reservationPrice = entry.targetPrice;
                 expectationsReverse.push_back(entry);
                */

                 entry.hedonicPrice = hedonicPrice;
                 entry.targetPrice  = hedonicPrice *  lowerBound;
                 entry.askingPrice  = hedonicPrice * (lowerBound + ((double)timeOnMarket - i) / timeOnMarket * 0.2);
                 expectations.push_back(entry);
            }
        }
    }


    return expectations;
}


double HedonicPrice_SubModel::CalculateSpeculation(ExpectationEntry entry, double unitBids)
{
    const double maximumBids = 20;
    const double a = 800000; //--a is the ratio of events expected by the seller.
    const double b = 0.3;    //--b is the importance of the price for seller.
    const double c = 1000;   //--c is the offset of the speculation price in thousands of dollars.

    return (maximumBids-unitBids) * entry.askingPrice / (a - (b * entry.askingPrice)) * c;
}



//--
//--F'(x) = (f(x + crit) - f(x - crit)) / 2*crit
//--
double HedonicPrice_SubModel::Numerical1Derivative( double (*f)(double , double , double , double , double ), double x0, double p1, double p2, double p3, double p4, double crit)
{
    return ((f((x0 + crit), p1, p2, p3, p4) - f((x0 - crit), p1, p2, p3, p4)) / (2 * crit));
}

//--
//-- F''(x) = (f(x + crit) - (2 * f(x)) + f(x - crit)) / crit^2
//--
//-- returns nan or infinite if some error occurs.
//--
double HedonicPrice_SubModel::Numerical2Derivative(double (*f)(double , double , double , double , double ), double x0, double p1, double p2, double p3, double p4, double crit)
{
    return ((f((x0 + crit), p1, p2, p3, p4) - (2 * f((x0), p1, p2, p3, p4)) + (f((x0 - crit), p1, p2, p3, p4))) / (crit * crit));
}

double HedonicPrice_SubModel::FindMaxArg(double (*f)(double , double , double , double , double ), double x0, double p1, double p2, double p3, double p4, double crit, double maxIterations)
{
    double inf = std::numeric_limits<double>::infinity();
    return FindMaxArgConstrained( f, x0, p1, p2, p3, p4, crit, maxIterations, -inf, inf);
}

double HedonicPrice_SubModel::FindMaxArgConstrained(double (*f)(double , double , double , double , double ), double x0, double p1, double p2, double p3, double p4, double crit, double maxIterations, double lowerLimit, double highLimit)
{
    double x1 = 0;
    double delta = 0;
    double iters = 0;
    double derivative1 = 0;
    double derivative2 = 0;

    do
    {
        derivative1 = Numerical1Derivative(f, x0, p1, p2, p3, p4, crit);
        derivative2 = Numerical2Derivative(f, x0, p1, p2, p3, p4, crit);

        double x_dash = 0;

        if(derivative2 != 0)
            x_dash = (derivative1 / derivative2);

        x1 = x0 - x_dash;

        //-- We are searching for a maximum within the range [lowerLimit, highLimit]
        //-- if x1 >  highLimit better we have a new maximum.
        //-- if x1 <  lowerLimit then we need to re-start with a value within the range [lowerLimit, highLimit]
        delta = abs(x1 - x0);

        if (x1 <= lowerLimit && x1 > highLimit)
           x0 = lowerLimit + ( rand() / (float)RAND_MAX) * ( highLimit - lowerLimit);
        else
           x0 = x1;

        iters= iters + 1;
    }
    while( delta > crit && iters < maxIterations && derivative1 != 0 && derivative2 != 0);

    return x0;
}

//--[[
//    Converts the given square meters value to square foot value.
//    @param sqmValue value in square meters.
//]]
double HedonicPrice_SubModel::sqmToSqf(double sqmValue)
{
    return sqmValue * 10.7639;
}

//--[[
//    Converts the given square foot value to square meter value.
//    @param sqfValue value in square foot.
//]]

double HedonicPrice_SubModel::sqfToSqm(double sqfValue)
{
    return sqfValue / 10.7639;
}
