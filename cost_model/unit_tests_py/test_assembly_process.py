import design as d

ap = d.Assembly("test_assembly_process", 0.1, None, 1000000, 5, 0.9, 200000, 10,1,2000000,5,0.8,210000, 20,2,0.2,0.3,0.4,0.5,0.987,0.999,0.0003,True)

assert ap.assembly_fully_defined() == True
assert ap.name == "test_assembly_process"
assert ap.materials_cost_per_mm2 == 0.1
assert ap.bb_cost_per_second is None
assert ap.picknplace_machine_cost == 1000000
assert ap.picknplace_machine_lifetime == 5
assert ap.picknplace_machine_uptime == 0.9
assert ap.picknplace_technician_yearly_cost == 200000
assert ap.picknplace_time == 10
assert ap.picknplace_group == 1
assert ap.bonding_machine_cost == 2000000
assert ap.bonding_machine_lifetime == 5
assert ap.bonding_machine_uptime == 0.8
assert ap.bonding_technician_yearly_cost == 210000
assert ap.bonding_time == 20
assert ap.bonding_group == 2
assert ap.die_separation == 0.2
assert ap.edge_exclusion == 0.3
assert ap.max_pad_current_density == 0.4
assert ap.bonding_pitch == 0.5
assert ap.alignment_yield == 0.987
assert ap.bonding_yield == 0.999
assert ap.dielectric_bond_defect_density == 0.0003

assert ap.get_power_per_pad(1.0) == 0.019634954084936207
assert ap.get_power_per_pad(0.8) == 0.015707963267948967
assert ap.get_power_per_pad(1.2) == 0.023561944901923447
assert ap.get_power_per_pad(10.0) == 0.19634954084936207

assert ap.compute_picknplace_time(1) == 10
assert ap.compute_picknplace_time(5) == 50
assert ap.compute_picknplace_time(10) == 100
assert ap.compute_picknplace_time(100) == 1000

assert ap.compute_bonding_time(1) == 20
assert ap.compute_bonding_time(5) == 60
assert ap.compute_bonding_time(10) == 100
assert ap.compute_bonding_time(100) == 1000

assert ap.assembly_time(1) == 30
assert ap.assembly_time(5) == 110
assert ap.assembly_time(10) == 200
assert ap.assembly_time(100) == 2000

assert ap.picknplace_cost_per_second == 0.01141552511415525

assert ap.bonding_cost_per_second == 0.015474378488077117

assert ap.assembly_cost(1,10) == 1.423642820903095
assert ap.assembly_cost(100,10) == 27.88990360223237
assert ap.assembly_cost(1,50) == 5.423642820903095
assert ap.assembly_cost(70,33) == 22.122932521562657

assert ap.assembly_yield(1, 1, 10) == 0.9830638085742773
assert ap.assembly_yield(10, 1, 10) == 0.873848372866451
assert ap.assembly_yield(100, 1, 10) == 0.2691409754969558
assert ap.assembly_yield(1, 1000, 10) == 0.3618298945652455
assert ap.assembly_yield(10, 1000, 10) == 0.3216316803268721
assert ap.assembly_yield(100, 1000, 10) == 0.0990609662748996
assert ap.assembly_yield(1, 1, 9) == 0.983357933579336
assert ap.assembly_yield(10, 1, 15) == 0.8725434723594329
assert ap.assembly_yield(100, 1, 70) == 0.2643960807281554
assert ap.assembly_yield(1, 1000, 100) == 0.3523450332513992
assert ap.assembly_yield(10, 1000, 200) == 0.30433639185646477
assert ap.assembly_yield(100, 1000, 400) == 0.08871263319082526
