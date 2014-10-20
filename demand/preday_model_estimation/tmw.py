from biogeme import *
from headers import *
from nested import *
from loglikelihood import *
from statistics import *
#import random

cons_bus = Beta('bus cons',0,-10,10,0)
cons_mrt = Beta('MRT cons',0,-10,10,0)
cons_privatebus=Beta('private bus cons',0,-10,10,0)
cons_drive1=Beta('drive alone cons',0,-10,10,1)
cons_share2=Beta('share2 cons',0,-10,10,0)
cons_share3=Beta('share3 plus cons',0,-10,10,0)
cons_motor=Beta('motor cons',0,-10,10,0)
cons_walk=Beta('walk cons',0,-10,10,0)
cons_taxi=Beta('taxi cons',0,-10,10,0)

beta1_1_tt = Beta('travel time beta1_1 ivt',0,-10,10,0)
beta1_2_tt = Beta('travel time beta1_2 waiting',0,-10,10,0)
beta1_3_tt = Beta('travel time beta1_3 walk',0,-10,10,0)

beta_private_1_tt=Beta('travel time beta_private_1 ivt',0,-10,10,0)
#beta_private_2_tt=Beta('travel time beta_private_2 wait',0,-10,10,0)
#beta_private_3_tt=Beta('travel time beta_private_2 walk',0,-10,10,0)

#beta2_tt = Beta('travel time beta2',0,-10,10,0)

beta2_tt_drive1 = Beta('travel time beta drive1',0,-10,10,0)
beta2_tt_share2 = Beta('travel time beta share2',0,-10,10,0)
beta2_tt_share3 = Beta('travel time beta share3',0,-10,10,0)
beta2_tt_motor = Beta('travel time beta motor',0,-10,10,0)

beta_tt_walk =Beta('travel time beta walk',0,-10,10,0)
beta_tt_taxi =Beta('travel time beta taxi',0,-10,10,0)


bound=15
beta4_1_cost = Beta('travel cost beta4_1',0,-bound,bound,0)
beta4_2_cost = Beta('travel cost beta4_2',0,-bound,bound,0)
beta5_1_cost = Beta('travel cost beta5_1',0,-bound,bound,0)
beta5_2_cost = Beta('travel cost beta5_2',0,-bound,bound,0)
beta6_1_cost = Beta('travel cost beta6_1',0,-bound,bound,0)
beta6_2_cost = Beta('travel cost beta6_2',0,-bound,bound,0)
beta7_1_cost = Beta('travel cost beta7_1',0,-bound,bound,0)
beta7_2_cost = Beta('travel cost beta7_2',0,-bound,bound,0)
beta8_1_cost = Beta('travel cost beta8_1',0,-bound,bound,0)
beta8_2_cost = Beta('travel cost beta8_2',0,-bound,bound,0)
beta9_1_cost = Beta('travel cost beta9_1',0,-bound,bound,0)
beta9_2_cost = Beta('travel cost beta9_2',0,-bound,bound,0)
beta10_1_cost = Beta('travel cost beta10_1',0,-bound,bound,0)
beta10_2_cost = Beta('travel cost beta10_2',0,-bound,bound,0)


beta_central_bus=Beta('central dummy in bus',0,-10,10,0)
beta_central_mrt=Beta('central dummy in mrt',0,-10,10,0)
beta_central_privatebus=Beta('central dummy in privatebus',0,-10,10,0)
beta_central_share2=Beta('central dummy in share2',0,-10,10,0)
beta_central_share3=Beta('central dummy in share3 plus',0,-10,10,0)
beta_central_motor=Beta('central dummy in motor',0,-10,10,0)
beta_central_taxi=Beta('central dummy in taxi',0,-10,10,0)
beta_central_walk=Beta('central dummy in walk',0,-10,10,0)


beta_female_bus=Beta('female dummy in bus',0,-10,10,0)
beta_female_mrt=Beta('female dummy in mrt',0,-10,10,0)
beta_female_privatebus=Beta('female dummy in privatebus',0,-10,10,0)

beta_female_drive1=Beta('female dummy in drive1',0,-10,10,1)
beta_female_share2=Beta('female dummy in share2',0,-10,10,0)
beta_female_share3=Beta('female dummy in share3 plus',0,-10,10,0)

beta_female_motor=Beta('female dummy in motor',0,-10,10,0)
beta_female_taxi=Beta('female dummy in taxi',0,-10,10,0)
beta_female_walk=Beta('female dummy in walk',0,-10,10,0)


#beta_autoown_cardriver=Beta('auto ownership in cardriver',0,-10,10,0)
#beta_autoown_carpassenger=Beta('auto ownership in carpassenger',0,-10,10,0)
#beta_motorown=Beta('motorcycle ownership in motor',0,-10,10,0)

beta_zero_drive1=Beta('zero cars in drive1',0,-10,10,1)
beta_oneplus_drive1=Beta('one plus cars in drive1',0,-10,10,1)
beta_twoplus_drive1=Beta('two plus cars in drive1',0,-10,10,0)
beta_threeplus_drive1=Beta('three plus cars in drive1',0,-10,10,0)

beta_zero_share2=Beta('zero cars in share2',0,-10,10,1)
beta_oneplus_share2=Beta('one plus cars in share2',0,-10,10,0)
beta_twoplus_share2=Beta('two plus cars in share2',0,-10,10,0)
beta_threeplus_share2=Beta('three plus cars in share2',0,-10,10,0)

beta_zero_share3=Beta('zero cars in share3 plus',0,-10,10,1)
beta_oneplus_share3=Beta('one plus cars in share3 plus',0,-10,10,0)
beta_twoplus_share3=Beta('two plus cars in share3 plus',0,-10,10,0)
beta_threeplus_share3=Beta('three plus cars in share3 plus',0,-30,10,1)


beta_zero_motor=Beta('zero motors in motor',0,-10,10,1)
beta_oneplus_motor=Beta('one plus motors in motor',0,-10,10,0)
beta_twoplus_motor=Beta('two plus motors in motor',0,-10,10,0)
beta_threeplus_motor=Beta('three plus motors in motor',0,-10,10,0)


beta_transfer=Beta('average transfer number in bus and mrt', 0,-10,10,0)

beta_distance=Beta('distance in private bus',0,-10,10,1)
beta_residence=Beta('home zone residential size in private bus',0,-10,10,0)
beta_residence_2=Beta('square of home zone residential size in private bus',0,-10,10,1)
beta_attraction=Beta('work zone work attraction in private bus',0,-10,10,0)
beta_attraction_2=Beta('square of work zone work attraction in private bus',0,-10,10,1)

MU1 = Beta('MU for car',1,1,100,1)
MU2 = Beta('MU for PT', 2.0,1,100,0)

#define cost and travel time


cost_bus=cost_public_first+cost_public_second
cost_mrt=cost_public_first+cost_public_second
cost_privatebus=cost_public_first+cost_public_second


cost_cardriver=cost_car_ERP_first+cost_car_ERP_second+cost_car_OP_first+cost_car_OP_second+cost_car_parking
cost_carpassenger=cost_car_ERP_first+cost_car_ERP_second+cost_car_OP_first+cost_car_OP_second+cost_car_parking


cost_motor=0.5*(cost_car_ERP_first+cost_car_ERP_second+cost_car_OP_first+cost_car_OP_second)+0.65*cost_car_parking

d1=walk_distance1
d2=walk_distance2
cost_taxi_1=3.4+((d1*(d1>10)-10*(d1>10))/0.35+(d1*(d1<=10)+10*(d1>10))/0.4)*0.22+ cost_car_ERP_first + Central_dummy*3
cost_taxi_2=3.4+((d2*(d2>10)-10*(d2>10))/0.35+(d2*(d2<=10)+10*(d2>10))/0.4)*0.22+ cost_car_ERP_second + Central_dummy*3
cost_taxi=cost_taxi_1+cost_taxi_2

cost_over_income_bus=30*cost_bus/(0.5+Income_mid)
cost_over_income_mrt=30*cost_mrt/(0.5+Income_mid)
cost_over_income_privatebus=30*cost_privatebus/(0.5+Income_mid)
cost_over_income_cardriver=30*cost_cardriver/(0.5+Income_mid)
cost_over_income_carpassenger=30*cost_carpassenger/(0.5+Income_mid)
cost_over_income_motor=30*cost_motor/(0.5+Income_mid)
cost_over_income_taxi=30*cost_taxi/(0.5+Income_mid)

tt_bus_ivt=tt_public_ivt_first+tt_public_ivt_second
tt_bus_wait=tt_public_waiting_first+tt_public_waiting_second
tt_bus_walk=tt_public_walk_first+tt_public_walk_second
tt_bus_all=tt_bus_ivt+tt_bus_wait+tt_bus_walk

tt_mrt_ivt=tt_public_ivt_first+tt_public_ivt_second
tt_mrt_wait=tt_public_waiting_first+tt_public_waiting_second
tt_mrt_walk=tt_public_walk_first+tt_public_walk_second
tt_mrt_all=tt_mrt_ivt+tt_mrt_wait+tt_mrt_walk

#tt_privatebus_ivt=tt_public_ivt_first+tt_public_ivt_second
tt_privatebus_ivt=tt_ivt_car_first+tt_ivt_car_second

tt_privatebus_wait=tt_public_waiting_first+tt_public_waiting_second
tt_privatebus_walk=tt_public_walk_first+tt_public_walk_second
tt_privatebus_all=tt_privatebus_ivt+tt_privatebus_wait+tt_privatebus_walk

tt_cardriver_ivt=tt_ivt_car_first+tt_ivt_car_second
tt_cardriver_out=1.0/6
tt_cardriver_all=tt_cardriver_ivt+tt_cardriver_out

tt_carpassenger_ivt=tt_ivt_car_first+tt_ivt_car_second
tt_carpassenger_out=1.0/6
tt_carpassenger_all=tt_carpassenger_ivt+tt_carpassenger_out

tt_motor_ivt=tt_ivt_car_first+tt_ivt_car_second
tt_motor_out=1.0/6
tt_motor_all=tt_motor_ivt+tt_motor_out

tt_walk=walk_time_first+walk_time_second

tt_taxi_ivt=tt_ivt_car_first+tt_ivt_car_second
tt_taxi_out=1.0/6
tt_taxi_all=tt_cardriver_ivt+tt_cardriver_out


residential_size=resident_size/origin_area/10000.0
work_attraction=work_op/destination_area/10000.0

#V1=public bus   -bus
#V2=MRT          -MRT
#V3=Private bus  -privatebus
#V4=car driver   -cardriver (base)
#V5=car passenger-carpassenger
#V6=motor        -motor
#V7=walk         -walk

V1 = cons_bus + beta1_1_tt * tt_bus_ivt + beta1_2_tt * tt_bus_walk + beta1_3_tt * tt_bus_wait + beta4_1_cost * cost_over_income_bus * (1-missing_income) + beta4_2_cost * cost_bus *missing_income + beta_central_bus * Central_dummy + beta_transfer * average_transfer_number+beta_female_bus * Female_dummy
V2 = cons_mrt + beta1_1_tt * tt_mrt_ivt + beta1_2_tt * tt_mrt_walk + beta1_3_tt * tt_mrt_wait + beta4_1_cost * cost_over_income_mrt * (1-missing_income) + beta4_2_cost * cost_mrt *missing_income +beta_central_mrt * Central_dummy + beta_transfer  * average_transfer_number+beta_female_mrt * Female_dummy
V3 = cons_privatebus + beta_private_1_tt * tt_privatebus_ivt +beta5_1_cost * cost_over_income_privatebus * (1-missing_income)+beta5_2_cost * cost_privatebus * missing_income+ beta_central_privatebus * Central_dummy+beta_distance*(d1+d2)+beta_residence*residential_size+beta_attraction*work_attraction+beta_residence_2*residential_size**2+beta_attraction_2*work_attraction**2+beta_female_privatebus* Female_dummy


V4 = cons_drive1 + beta2_tt_drive1 * tt_cardriver_all + beta6_1_cost * cost_over_income_cardriver * (1-missing_income) + beta6_2_cost * cost_cardriver *missing_income + beta_female_drive1 * Female_dummy + beta_zero_drive1 * zero_car + beta_oneplus_drive1 * one_plus_car + beta_twoplus_drive1 * two_plus_car + beta_threeplus_drive1 * three_plus_car

V5 = cons_share2 + beta2_tt_share2 * tt_carpassenger_all + beta7_1_cost * cost_over_income_carpassenger/2 * (1-missing_income) + beta7_2_cost * cost_carpassenger/2 *missing_income  + beta_central_share2 * Central_dummy + beta_female_share2 * Female_dummy + beta_zero_share2 * zero_car + beta_oneplus_share2 * one_plus_car + beta_twoplus_share2 * two_plus_car + beta_threeplus_share2 * three_plus_car

V6 = cons_share3 + beta2_tt_share3 * tt_carpassenger_all + beta8_1_cost * cost_over_income_carpassenger/3 * (1-missing_income) + beta8_2_cost * cost_carpassenger/3 *missing_income  + beta_central_share3 * Central_dummy + beta_female_share3 * Female_dummy + beta_zero_share3 * zero_car + beta_oneplus_share3 * one_plus_car + beta_twoplus_share3 * two_plus_car + beta_threeplus_share3 * three_plus_car

V7 = cons_motor + beta2_tt_motor * tt_motor_all + beta9_1_cost * cost_over_income_motor * (1-missing_income) + beta9_2_cost * cost_motor *missing_income  + beta_central_motor * Central_dummy + beta_zero_motor * zero_motor + beta_oneplus_motor * one_plus_motor + beta_twoplus_motor * two_plus_motor + beta_threeplus_motor * three_plus_motor + beta_female_motor * Female_dummy

V8 = cons_walk  + beta_tt_walk * tt_walk + beta_central_walk * Central_dummy+ beta_female_walk * Female_dummy
V9 = cons_taxi + beta_tt_taxi * tt_taxi_all + beta10_1_cost * cost_over_income_taxi * (1-missing_income) + beta10_2_cost * cost_taxi *missing_income + beta_central_taxi * Central_dummy + beta_female_taxi * Female_dummy

V = {1:V1,2: V2,3:V3,4:V4,5:V5,6:V6,7:V7,8:V8,9:V9}
av= {1:bus_avail_dummy,2:mrt_avail_dummy,3:private_bus_avail_dummy,4:car_driver_avail_dummy,5:car_passenger_avail_dummy,6: car_passenger_avail_dummy,7:motor_avail_dummy_all,8:walk_avail_dummy,9:taxi_avail_dummy}

#Definition of nests:
# 1: nests parameter
# 2: list of alternatives
car = MU1 , [4,5,6]
PT = MU2 , [1,2,3]
other = 1.0, [7,8,9]
#private= 1.0, [3]
#motor = 1.0, [6]
#walk = 1.0, [7]
#taxi = 1.0, [8]

nests = car,PT,other
#nests=car,PT,private,motor,walk,taxi

# The choice model is a nested logit, with availability conditions
prob = nested(V,av,nests,choice_new)

rowIterator('obsIter') 
BIOGEME_OBJECT.ESTIMATE = Sum(log(prob),'obsIter')


exclude = ((choice==0)+(PrimaryActivityIndex!=1)+(go_to_primary_work_location==0)+(avail_violation==1)+(IncomeIndex==12)) > 0


BIOGEME_OBJECT.EXCLUDE = exclude
nullLoglikelihood(av,'obsIter')
choiceSet = [1,2,3,4,5,6,7,8,9]
cteLoglikelihood(choiceSet,choice_new,'obsIter')
availabilityStatistics(av,'obsIter')
BIOGEME_OBJECT.PARAMETERS['optimizationAlgorithm'] = "CFSQP"
BIOGEME_OBJECT.PARAMETERS['checkDerivatives'] = "1"
BIOGEME_OBJECT.PARAMETERS['numberOfThreads'] = "4"
