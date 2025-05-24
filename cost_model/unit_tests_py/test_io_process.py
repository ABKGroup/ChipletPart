import design as d

io = d.IO("test_io_process", 1.0, 2.0, 0.5, 3.0, 4, "True", 0.9, 5, True)

assert io.io_fully_defined() == True

assert io.type == "test_io_process"
assert io.rx_area == 1.0
assert io.tx_area == 2.0
assert io.shoreline == 0.5
assert io.bandwidth == 3.0
assert io.wire_count == 4
assert io.bidirectional == True
assert io.energy_per_bit == 0.9
assert io.reach == 5

io = d.IO("test_io_process", 1, 2, 0.5, 3.0, 4, "False", 0.9, 5, True)

assert io.bidirectional == False
