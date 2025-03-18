import design as d

lp = d.Layer("test_layer_process", "True", 0.1234, 0.0321, 0.00543, 0.5, 2, 0.3, 100000, 0.98, True)

assert lp.layer_fully_defined() == True
assert lp.name == "test_layer_process"
assert lp.active == True
assert lp.cost_per_mm2 == 0.1234
assert lp.transistor_density == 0.0321
assert lp.defect_density == 0.00543
assert lp.critical_area_ratio == 0.5
assert lp.clustering_factor == 2
assert lp.litho_percent == 0.3
assert lp.mask_cost == 100000
assert lp.stitching_yield == 0.98

lp = d.Layer("test_layer_process", "False", 0.1234, 0.0321, 0.00543, 0.5, 2, 0.3, 100000, 0.98, True)

assert lp.active == False


# Function Checks

# Reticle utilization is only dependent on passed parameters.
assert lp.reticle_utilization(1.0,10.0,10.0) == 1.0
assert lp.reticle_utilization(2.0,10.0,10.0) == 1.0
assert lp.reticle_utilization(1.0,30.0,10.0) == 1.0
assert lp.reticle_utilization(1.0,10.0,20.0) == 1.0
assert lp.reticle_utilization(3.0,10.0,10.0) == 0.99
assert lp.reticle_utilization(45.0,10.0,10.0) == 0.9
assert lp.reticle_utilization(51.0,10.0,10.0) == 0.51

# Compute dies per wafer is only dependent on passed parameters
assert lp.compute_dies_per_wafer(10,10,300,1,True) == 540
assert lp.compute_dies_per_wafer(1,1,300,1,True) == 17470
assert lp.compute_dies_per_wafer(100,100,300,1,True) == 4
assert lp.compute_dies_per_wafer(10,10,100,1,True) == 52
assert lp.compute_dies_per_wafer(10,10,300,0.1,True) == 639
assert lp.compute_dies_per_wafer(10,10,300,2,True) == 452
assert lp.compute_dies_per_wafer(10,10,300,1,False) == 550
assert lp.compute_dies_per_wafer(1,1,300,1,False) == 17590
assert lp.compute_dies_per_wafer(100,100,300,1,False) == 4
assert lp.compute_dies_per_wafer(10,10,100,1,False) == 52
assert lp.compute_dies_per_wafer(10,10,300,0.1,False) == 651
assert lp.compute_dies_per_wafer(10,10,300,2,False) == 462

# First layer process definition
lp = d.Layer("test_layer_process", "True", 0.1234, 0.0321, 0.00543, 0.5, 2, 0.3, 100000, 0.98, True)
# Layer yield is dependent on layer object properties.
assert lp.layer_yield(1.0) == 0.9972905184292643
assert lp.layer_yield(10.0) == 0.9733930025109545
assert lp.layer_yield(100.0) == 0.7752370299332099
assert lp.layer_yield(1000.0) == 0.17992710703076417

# The following also depend on wafer process.
wp = d.WaferProcess("test_wafer_process", 234, 1.2, 0.98, 0.87, 32, 23, "False", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.4431977042289835
assert lp.layer_cost(1.0,2.0,wp) == 0.4563853896145381
assert lp.layer_cost(100.0,1.0,wp) == 16.18236160385767
assert lp.layer_cost(800.0,1.0,wp) == 170.36347017097916
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.20967401463602725
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.15804071921253907
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.2136412765876751
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.1597967272037895
wp = d.WaferProcess("test_wafer_process", 123, 2.4, 0.49, 0.45, 16, 25, "False", 0.2, 0.3, 0.4, 0.5, 0.6, 0.7)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.28499017291594464
assert lp.layer_cost(1.0,2.0,wp) == 0.2920283687816242
assert lp.layer_cost(100.0,1.0,wp) == 17.049702786657388
assert lp.layer_cost(800.0,1.0,wp) == 183.28430495656693
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.1828272368644059
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.170299005766845
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.18420533161464012
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.16556847782887707

lp = d.Layer("test_layer_process", "False", 0.1234, 0.0321, 0.00543, 0.5, 2, 0.3, 100000, 0.98, True)
# Layer yield is dependent on layer object properties.
assert lp.layer_yield(1.0) == 0.9972905184292643
assert lp.layer_yield(10.0) == 0.9733930025109545
assert lp.layer_yield(100.0) == 0.7752370299332099
assert lp.layer_yield(1000.0) == 0.17992710703076417

# The following also depend on wafer process.
wp = d.WaferProcess("test_wafer_process", 234, 1.2, 0.98, 0.87, 32, 23, "False", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.4431977042289835
assert lp.layer_cost(1.0,2.0,wp) == 0.4563853896145381
assert lp.layer_cost(100.0,1.0,wp) == 16.18236160385767
assert lp.layer_cost(800.0,1.0,wp) == 170.36347017097916
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.20967401463602725
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.15804071921253907
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.2136412765876751
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.1597967272037895
wp = d.WaferProcess("test_wafer_process", 123, 2.4, 0.49, 0.45, 16, 25, "False", 0.2, 0.3, 0.4, 0.5, 0.6, 0.7)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.28499017291594464
assert lp.layer_cost(1.0,2.0,wp) == 0.2920283687816242
assert lp.layer_cost(100.0,1.0,wp) == 17.049702786657388
assert lp.layer_cost(800.0,1.0,wp) == 183.28430495656693
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.1828272368644059
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.170299005766845
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.18420533161464012
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.16556847782887707

lp = d.Layer("test_layer_process", "True", 0.2468, 0.0321, 0.00543, 0.5, 2, 0.3, 100000, 0.98, True)
# Layer yield is dependent on layer object properties.
assert lp.layer_yield(1.0) == 0.9972905184292643
assert lp.layer_yield(10.0) == 0.9733930025109545
assert lp.layer_yield(100.0) == 0.7752370299332099
assert lp.layer_yield(1000.0) == 0.17992710703076417

# The following also depend on wafer process.
wp = d.WaferProcess("test_wafer_process", 234, 1.2, 0.98, 0.87, 32, 23, "False", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.886395408457967
assert lp.layer_cost(1.0,2.0,wp) == 0.9127707792290762
assert lp.layer_cost(100.0,1.0,wp) == 32.36472320771534
assert lp.layer_cost(800.0,1.0,wp) == 340.7269403419583
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.4193480292720545
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.31608143842507813
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.4272825531753502
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.319593454407579
wp = d.WaferProcess("test_wafer_process", 123, 2.4, 0.49, 0.45, 16, 25, "False", 0.2, 0.3, 0.4, 0.5, 0.6, 0.7)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.5699803458318893
assert lp.layer_cost(1.0,2.0,wp) == 0.5840567375632484
assert lp.layer_cost(100.0,1.0,wp) == 34.099405573314776
assert lp.layer_cost(800.0,1.0,wp) == 366.56860991313386
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.3656544737288118
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.34059801153369
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.36841066322928023
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.33113695565775414

lp = d.Layer("test_layer_process", "True", 0.1234, 0.0642, 0.00543, 0.5, 2, 0.3, 100000, 0.98, True)
# Layer yield is dependent on layer object properties.
assert lp.layer_yield(1.0) == 0.9972905184292643
assert lp.layer_yield(10.0) == 0.9733930025109545
assert lp.layer_yield(100.0) == 0.7752370299332099
assert lp.layer_yield(1000.0) == 0.17992710703076417

# The following also depend on wafer process.
wp = d.WaferProcess("test_wafer_process", 234, 1.2, 0.98, 0.87, 32, 23, "False", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.4431977042289835
assert lp.layer_cost(1.0,2.0,wp) == 0.4563853896145381
assert lp.layer_cost(100.0,1.0,wp) == 16.18236160385767
assert lp.layer_cost(800.0,1.0,wp) == 170.36347017097916
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.20967401463602725
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.15804071921253907
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.2136412765876751
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.1597967272037895
wp = d.WaferProcess("test_wafer_process", 123, 2.4, 0.49, 0.45, 16, 25, "False", 0.2, 0.3, 0.4, 0.5, 0.6, 0.7)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.28499017291594464
assert lp.layer_cost(1.0,2.0,wp) == 0.2920283687816242
assert lp.layer_cost(100.0,1.0,wp) == 17.049702786657388
assert lp.layer_cost(800.0,1.0,wp) == 183.28430495656693
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.1828272368644059
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.170299005766845
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.18420533161464012
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.16556847782887707

lp = d.Layer("test_layer_process", "True", 0.1234, 0.0321, 0.01086, 0.5, 2, 0.3, 100000, 0.98, True)
# Layer yield is dependent on layer object properties.
assert lp.layer_yield(1.0) == 0.994592033894289
assert lp.layer_yield(10.0) == 0.9478339469519471
assert lp.layer_yield(100.0) == 0.6185392607744513
assert lp.layer_yield(1000.0) == 0.07245733621472007

# The following also depend on wafer process.
wp = d.WaferProcess("test_wafer_process", 234, 1.2, 0.98, 0.87, 32, 23, "False", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.4431977042289835
assert lp.layer_cost(1.0,2.0,wp) == 0.4563853896145381
assert lp.layer_cost(100.0,1.0,wp) == 16.18236160385767
assert lp.layer_cost(800.0,1.0,wp) == 170.36347017097916
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.20967401463602725
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.15804071921253907
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.2136412765876751
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.1597967272037895
wp = d.WaferProcess("test_wafer_process", 123, 2.4, 0.49, 0.45, 16, 25, "False", 0.2, 0.3, 0.4, 0.5, 0.6, 0.7)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.28499017291594464
assert lp.layer_cost(1.0,2.0,wp) == 0.2920283687816242
assert lp.layer_cost(100.0,1.0,wp) == 17.049702786657388
assert lp.layer_cost(800.0,1.0,wp) == 183.28430495656693
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.1828272368644059
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.170299005766845
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.18420533161464012
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.16556847782887707

lp = d.Layer("test_layer_process", "True", 0.1234, 0.0321, 0.00543, 1.0, 2, 0.3, 100000, 0.98, True)
# Layer yield is dependent on layer object properties.
assert lp.layer_yield(1.0) == 0.994592033894289
assert lp.layer_yield(10.0) == 0.9478339469519471
assert lp.layer_yield(100.0) == 0.6185392607744513
assert lp.layer_yield(1000.0) == 0.07245733621472007

# The following also depend on wafer process.
wp = d.WaferProcess("test_wafer_process", 234, 1.2, 0.98, 0.87, 32, 23, "False", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.4431977042289835
assert lp.layer_cost(1.0,2.0,wp) == 0.4563853896145381
assert lp.layer_cost(100.0,1.0,wp) == 16.18236160385767
assert lp.layer_cost(800.0,1.0,wp) == 170.36347017097916
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.20967401463602725
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.15804071921253907
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.2136412765876751
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.1597967272037895
wp = d.WaferProcess("test_wafer_process", 123, 2.4, 0.49, 0.45, 16, 25, "False", 0.2, 0.3, 0.4, 0.5, 0.6, 0.7)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.28499017291594464
assert lp.layer_cost(1.0,2.0,wp) == 0.2920283687816242
assert lp.layer_cost(100.0,1.0,wp) == 17.049702786657388
assert lp.layer_cost(800.0,1.0,wp) == 183.28430495656693
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.1828272368644059
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.170299005766845
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.18420533161464012
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.16556847782887707

lp = d.Layer("test_layer_process", "True", 0.1234, 0.0321, 0.00543, 0.5, 2, 0.6, 100000, 0.98, True)
# Layer yield is dependent on layer object properties.
assert lp.layer_yield(1.0) == 0.9972905184292643
assert lp.layer_yield(10.0) == 0.9733930025109545
assert lp.layer_yield(100.0) == 0.7752370299332099
assert lp.layer_yield(1000.0) == 0.17992710703076417

# The following also depend on wafer process.
wp = d.WaferProcess("test_wafer_process", 234, 1.2, 0.98, 0.87, 32, 23, "False", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.4431977042289836
assert lp.layer_cost(1.0,2.0,wp) == 0.4563853896145381
assert lp.layer_cost(100.0,1.0,wp) == 16.428238791986065
assert lp.layer_cost(800.0,1.0,wp) == 204.65388109996218
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.20967401463602725
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.15804071921253907
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.2136412765876751
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.1597967272037895
wp = d.WaferProcess("test_wafer_process", 123, 2.4, 0.49, 0.45, 16, 25, "False", 0.2, 0.3, 0.4, 0.5, 0.6, 0.7)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.2849901729159447
assert lp.layer_cost(1.0,2.0,wp) == 0.29202836878162425
assert lp.layer_cost(100.0,1.0,wp) == 17.049702786657388
assert lp.layer_cost(800.0,1.0,wp) == 183.28430495656693
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.1828272368644059
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.170299005766845
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.18420533161464012
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.16556847782887707

lp = d.Layer("test_layer_process", "True", 0.1234, 0.0321, 0.00543, 0.5, 2, 0.3, 200000, 0.98, True)
# Layer yield is dependent on layer object properties.
assert lp.layer_yield(1.0) == 0.9972905184292643
assert lp.layer_yield(10.0) == 0.9733930025109545
assert lp.layer_yield(100.0) == 0.7752370299332099
assert lp.layer_yield(1000.0) == 0.17992710703076417

# The following also depend on wafer process.
wp = d.WaferProcess("test_wafer_process", 234, 1.2, 0.98, 0.87, 32, 23, "False", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.4431977042289835
assert lp.layer_cost(1.0,2.0,wp) == 0.4563853896145381
assert lp.layer_cost(100.0,1.0,wp) == 16.18236160385767
assert lp.layer_cost(800.0,1.0,wp) == 170.36347017097916
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.20967401463602725
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.15804071921253907
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.2136412765876751
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.1597967272037895
wp = d.WaferProcess("test_wafer_process", 123, 2.4, 0.49, 0.45, 16, 25, "False", 0.2, 0.3, 0.4, 0.5, 0.6, 0.7)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.28499017291594464
assert lp.layer_cost(1.0,2.0,wp) == 0.2920283687816242
assert lp.layer_cost(100.0,1.0,wp) == 17.049702786657388
assert lp.layer_cost(800.0,1.0,wp) == 183.28430495656693
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.1828272368644059
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.170299005766845
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.18420533161464012
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.16556847782887707

lp = d.Layer("test_layer_process", "True", 0.1234, 0.0321, 0.00543, 0.5, 2, 0.3, 100000, 0.49, True)
# Layer yield is dependent on layer object properties.
assert lp.layer_yield(1.0) == 0.9972905184292643
assert lp.layer_yield(10.0) == 0.9733930025109545
assert lp.layer_yield(100.0) == 0.7752370299332099
assert lp.layer_yield(1000.0) == 0.17992710703076417

# The following also depend on wafer process.
wp = d.WaferProcess("test_wafer_process", 234, 1.2, 0.98, 0.87, 32, 23, "False", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.4431977042289835
assert lp.layer_cost(1.0,2.0,wp) == 0.4563853896145381
assert lp.layer_cost(100.0,1.0,wp) == 16.18236160385767
assert lp.layer_cost(800.0,1.0,wp) == 170.36347017097916
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.20967401463602725
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.15804071921253907
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.2136412765876751
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.1597967272037895
wp = d.WaferProcess("test_wafer_process", 123, 2.4, 0.49, 0.45, 16, 25, "False", 0.2, 0.3, 0.4, 0.5, 0.6, 0.7)
# Layer cost is dependent on the layer object properties as well as the wafer process properties.
assert lp.layer_cost(1.0,1.0,wp) == 0.28499017291594464
assert lp.layer_cost(1.0,2.0,wp) == 0.2920283687816242
assert lp.layer_cost(100.0,1.0,wp) == 17.049702786657388
assert lp.layer_cost(800.0,1.0,wp) == 183.28430495656693
# Compute cost per mm2 is dependent on the layer object properties as well as the wafer process properties.
assert lp.compute_cost_per_mm2(10.0, 1.0, wp) == 0.1828272368644059
assert lp.compute_cost_per_mm2(123.0, 1.0, wp) == 0.170299005766845
assert lp.compute_cost_per_mm2(10.0, 2.0, wp) == 0.18420533161464012
assert lp.compute_cost_per_mm2(123.0, 2.0, wp) == 0.16556847782887707
