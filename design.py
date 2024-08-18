# Define Design Class and Corresponding Subclasses
# This is meant to avoid passing the same set of parameters to every function in the partitioning code.

import numpy as np
import math
import sys
import random
#import fiducciaMattheyses.run_params as params
import xml.etree.ElementTree as ET


# =========================================
# Wafer Process Class
# =========================================
# The class has the following attributes:
#   name: The name of the wafer process.
#   wafer_diameter: The diameter of the wafer in mm.
#   edge_exclusion: The edge exclusion of the wafer in mm.
#   wafer_process_yield: The yield of the wafer process. Value should be between 0 and 1.
#   reticle_x: Reticle dimension in the x dimension in mm.
#   reticle_y: Reticle dimension in the y dimension in mm.
#   wafer_fill_grid: Whether the wafer is filled in a grid pattern or in a line pattern
#       that ignores vertical alignment in dicing.
#   nre_front_end_cost_per_mm2_memory: The NRE design cost per mm^2 of the front end of
#       the wafer process for memory. (Front end refers to higher level design steps.)
#   nre_back_end_cost_per_mm2_memory: The NRE design cost per mm^2 of the back end of
#       the wafer process for memory. (Back end refers to lower level design steps.)
#   nre_front_end_cost_per_mm2_logic: The NRE design cost per mm^2 of the front end of
#       the wafer process for logic. (Front end refers to higher level design steps.)
#   nre_back_end_cost_per_mm2_logic: The NRE design cost per mm^2 of the back end of
#       the wafer process for logic. (Back end refers to lower level design steps.)
#   nre_front_end_cost_per_mm2_analog: The NRE design cost per mm^2 of the front end of
#       the wafer process for analog. (Front end refers to higher level design steps.)
#   nre_back_end_cost_per_mm2_analog: The NRE design cost per mm^2 of the back end of
#       the wafer process for analog. (Back end refers to lower level design steps.)
#   static: A boolean set true when the process is defined to prevent further changes.
# =========================================
# The class has the following methods.
# == Get/Set ==
#   get_name()
#   set_name(string)
#   get_wafer_diameter()
#   set_wafer_diameter(float)
#   get_edge_exclusion()
#   set_edge_exclusion(float)
#   get_wafer_process_yield()
#   set_wafer_process_yield(float)
#   get_reticle_x()
#   set_reticle_x(float)
#   get_reticle_y()
#   set_reticle_y(float)
#   get_wafer_fill_grid()
#   set_wafer_fill_grid(bool)
#   get_nre_front_end_cost_per_mm2_memory()
#   set_nre_front_end_cost_per_mm2_memory(float)
#   get_nre_back_end_cost_per_mm2_memory()
#   set_nre_back_end_cost_per_mm2_memory(float)
#   get_nre_front_end_cost_per_mm2_logic()
#   set_nre_front_end_cost_per_mm2_logic(float)
#   get_nre_back_end_cost_per_mm2_logic()
#   set_nre_back_end_cost_per_mm2_logic(float)
#   get_nre_front_end_cost_per_mm2_analog()
#   set_nre_front_end_cost_per_mm2_analog(float)
#   get_nre_back_end_cost_per_mm2_analog()
#   set_nre_back_end_cost_per_mm2_analog(float)
#   get_static()
#   set_static()
# == Print ==
#   print_description(): Dumps values of all the parameters for inspection.
# =========================================

class WaferProcess:
    def __init__(self, name = None, wafer_diameter = None, edge_exclusion = None, wafer_process_yield = None, dicing_distance = None, reticle_x = None, reticle_y = None, wafer_fill_grid = None, nre_front_end_cost_per_mm2_memory = None, nre_back_end_cost_per_mm2_memory = None, nre_front_end_cost_per_mm2_logic = None, nre_back_end_cost_per_mm2_logic = None, nre_front_end_cost_per_mm2_analog = None, nre_back_end_cost_per_mm2_analog = None, static = True) -> None:
        self.name = name
        self.wafer_diameter = wafer_diameter
        self.edge_exclusion = edge_exclusion
        self.wafer_process_yield = wafer_process_yield
        self.dicing_distance = dicing_distance
        self.reticle_x = reticle_x
        self.reticle_y = reticle_y
        if wafer_fill_grid == "True" or wafer_fill_grid == "true":
            self.wafer_fill_grid = True
        else:
            self.wafer_fill_grid = False
        self.nre_front_end_cost_per_mm2_memory = nre_front_end_cost_per_mm2_memory
        self.nre_back_end_cost_per_mm2_memory = nre_back_end_cost_per_mm2_memory
        self.nre_front_end_cost_per_mm2_logic = nre_front_end_cost_per_mm2_logic
        self.nre_back_end_cost_per_mm2_logic = nre_back_end_cost_per_mm2_logic
        self.nre_front_end_cost_per_mm2_analog = nre_front_end_cost_per_mm2_analog
        self.nre_back_end_cost_per_mm2_analog = nre_back_end_cost_per_mm2_analog
        self.static = static
        if self.name is None or self.wafer_diameter is None or self.edge_exclusion is None or self.wafer_process_yield is None or self.reticle_x is None or self.reticle_y is None or self.wafer_fill_grid is None:
            print("Warning: Wafer Process not fully defined, setting to non-static.")
            self.static = False
            print("Parameters given for Wafer Process name " + str(self.name) + " are wafer_diameter = " + str(self.wafer_diameter) + ", edge_exclusion = " + str(self.edge_exclusion) + ", wafer_process_yield = " + str(self.wafer_process_yield) + ", reticle_x = " + str(self.reticle_x) + ", reticle_y = " + str(self.reticle_y) + ", wafer_fill_grid = " + str(self.wafer_fill_grid) + ".")
        return

    # ===== Get/Set Functions =====

    def get_name(self) -> str:
        return self.name

    def set_name(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static wafer process.")
            return 1
        else:
            self.name = value
            return 0

    def get_wafer_diameter(self) -> float:
        return self.wafer_diameter

    def set_wafer_diameter(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static wafer process.")
            return 1
        else:
            self.wafer_diameter = value
            return 0

    def get_edge_exclusion(self) -> float:
        return self.edge_exclusion

    def set_edge_exclusion(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static wafer process.")
            return 1
        else:
            self.edge_exclusion = value
            return 0
    
    def get_wafer_process_yield(self) -> float:
        return self.wafer_process_yield

    def set_wafer_process_yield(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static wafer process.")
            return 1
        else:
            if value < 0.0 or value > 1.0:
                print("Error: Wafer process yield must be between 0 and 1.")
                return 1
            self.wafer_process_yield = value
            return 0

    def get_dicing_distance(self) -> float:
        return self.dicing_distance
    
    def set_dicing_distance(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static wafer process.")
            return 1
        else:
            self.dicing_distance = value
            return 0
    
    def get_reticle_x(self) -> int:
        return self.reticle_x

    def set_reticle_x(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static wafer process.")
            return 1
        else:
            self.reticle_x = value
            return 0

    def get_reticle_y(self) -> bool:
        return self.reticle_y
    
    def set_reticle_y(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static wafer process.")
            return 1
        else:
            self.reticle_y = value
            return 0

    def get_wafer_fill_grid(self) -> bool:
        return self.wafer_fill_grid
    
    def set_wafer_fill_grid(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static wafer process.")
            return 1
        else:
            if value == "True" or value == "true":
                self.wafer_fill_grid = True
            else:
                self.wafer_fill_grid = False
            return 0

    def get_nre_front_end_cost_per_mm2_memory(self) -> float:
        return self.nre_front_end_cost_per_mm2_memory
    
    def set_nre_front_end_cost_per_mm2_memory(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static wafer process.")
            return 1
        else:
            self.nre_front_end_cost_per_mm2_memory = value
            return 0
    
    def get_nre_back_end_cost_per_mm2_memory(self) -> float:
        return self.nre_back_end_cost_per_mm2_memory
    
    def set_nre_back_end_cost_per_mm2_memory(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static wafer process.")
            return 1
        else:
            self.nre_back_end_cost_per_mm2_memory = value
            return 0
        
    def get_nre_front_end_cost_per_mm2_logic(self) -> float:
        return self.nre_front_end_cost_per_mm2_logic
    
    def set_nre_front_end_cost_per_mm2_logic(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static wafer process.")
            return 1
        else:
            self.nre_front_end_cost_per_mm2_logic = value
            return 0
        
    def get_nre_back_end_cost_per_mm2_logic(self) -> float:
        return self.nre_back_end_cost_per_mm2_logic
    
    def set_nre_back_end_cost_per_mm2_logic(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static wafer process.")
            return 1
        else:
            self.nre_back_end_cost_per_mm2_logic = value
            return 0

    def get_nre_front_end_cost_per_mm2_analog(self) -> float:
        return self.nre_front_end_cost_per_mm2_analog

    def set_nre_front_end_cost_per_mm2_analog(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static wafer process.")
            return 1
        else:
            self.nre_front_end_cost_per_mm2_analog = value
            return 0    
        
    def get_nre_back_end_cost_per_mm2_analog(self) -> float:
        return self.nre_back_end_cost_per_mm2_analog
    
    def set_nre_back_end_cost_per_mm2_analog(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static wafer process.")
            return 1
        else:
            self.nre_back_end_cost_per_mm2_analog = value
            return 0

    def get_static(self) -> bool:
        return self.static

    def set_static(self) -> int:
        if self.name is None or self.wafer_diameter is None or self.edge_exclusion is None or self.wafer_process_yield is None or self.dicing_distance is None or self.reticle_x is None or self.reticle_y is None or self.wafer_fill_grid is None or self.nre_front_end_cost_per_mm2_memory is None or self.nre_back_end_cost_per_mm2_memory is None or self.nre_front_end_cost_per_mm2_logic is None or self.nre_back_end_cost_per_mm2_logic is None or self.nre_front_end_cost_per_mm2_analog is None or self.nre_back_end_cost_per_mm2_analog is None:
            print("Error: Attempt to set wafer process static without defining all parameters. Exiting...")
            sys.exit(1)
        self.static = True
        return 0

    # ===== End Get/Set Functions =====

    # ===== Print Functions =====

    def print_description(self) -> None:
        print("Wafer Process Name: " + self.get_name()
                + "\n\r\tWafer Diameter: " + str(self.get_wafer_diameter())
                + "\n\r\tEdge Exclusion: " + str(self.get_edge_exclusion())
                + "\n\r\tWafer Process Yield: " + str(self.get_wafer_process_yield())
                + "\n\r\tReticle X: " + str(self.get_reticle_x())
                + "\n\r\tReticle Y: " + str(self.get_reticle_y())
                + "\n\r\tWafer Fill Grid: " + str(self.get_wafer_fill_grid())
                + "\n\r\tNRE Front End Cost Per mm^2 Memory: " + str(self.get_nre_front_end_cost_per_mm2_memory())
                + "\n\r\tNRE Back End Cost Per mm^2 Memory: " + str(self.get_nre_back_end_cost_per_mm2_memory())
                + "\n\r\tNRE Front End Cost Per mm^2 Logic: " + str(self.get_nre_front_end_cost_per_mm2_logic())
                + "\n\r\tNRE Back End Cost Per mm^2 Logic: " + str(self.get_nre_back_end_cost_per_mm2_logic())
                + "\n\r\tNRE Front End Cost Per mm^2 Analog: " + str(self.get_nre_front_end_cost_per_mm2_analog())
                + "\n\r\tNRE Back End Cost Per mm^2 Analog: " + str(self.get_nre_back_end_cost_per_mm2_analog())
                + "\n\r\tStatic: " + str(self.get_static()))
        return

    # ===== End Print Functions =====


# =========================================
# IO Class
# =========================================
# The class has the following attributes:
#   type: The type of IO for this adjacency matrix. (Select from list of IO definitions.)
#   rx_area: The area of RX IOs in mm^2.
#   tx_area: The area of TX IOs in mm^2
#   shoreline: The shoreline of the IO in mm.
#   bandwidth: The bandwidth of the IO in Gbps.
#   wire_count: The number of wires in the IO.
#   bidirectional: Whether the IO is bidirectional or not.
#   energy_per_bit: The energy per bit of the IO in pJ/bit.
#   reach: The reach of the IO in mm.
#   static: A boolean set true when the IO is defined to prevent further changes.
# =========================================
# The class has the following methods.
# == Get/Set ==
#   get_type()
#   set_type(string)
#   get_rx_area()
#   set_rx_area(float)
#   get_tx_area()
#   set_tx_area(float)
#   get_shoreline()
#   set_shoreline(float)
#   get_bandwidth()
#   set_bandwidth(float)
#   get_wire_count()
#   set_wire_count(int)
#   get_bidirectional()
#   set_bidirectional(bool)
#   get_energy_per_bit()
#   set_energy_per_bit(float)
#   get_reach()
#   set_reach(float)
#   get_static()
#   set_static()
# == Print ==
#   print_description(): Dumps values of all the parameters for inspection.
# =========================================

class IO:
    def __init__(self, type = None, rx_area = None, tx_area = None, shoreline = None, bandwidth = None, wire_count = None, bidirectional = None, energy_per_bit = None, reach = None, static = True) -> None:
        self.type = type
        self.rx_area = rx_area
        self.tx_area = tx_area
        self.shoreline = shoreline
        self.bandwidth = bandwidth
        self.wire_count = wire_count
        if bidirectional == "True" or bidirectional == "true":
            self.bidirectional = True
        else:
            self.bidirectional = False
        self.bidirectional = bidirectional
        self.energy_per_bit = energy_per_bit
        self.reach = reach
        self.static = static
        if self.type is None or self.rx_area is None or self.tx_area is None or self.shoreline is None or self.bandwidth is None or self.wire_count is None or self.bidirectional is None or self.energy_per_bit is None or self.reach is None:
            print("Warning: IO not fully defined, setting to non-static.")
            self.static = False
            print("Parameters given for IO type " + str(self.type) + " are area = " + str(self.area) + ", shoreline = " + str(self.shoreline) + ", bandwidth = " + str(self.bandwidth) + ", wire_count = " + str(self.wire_count) + ".")
        return

    # ===== Get/Set Functions =====

    def get_type(self) -> str:
        return self.type

    def set_type(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static IO.")
            return 1
        else:
            self.type = value
            return 0

    def get_rx_area(self) -> float:
        return self.rx_area

    def set_rx_area(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static IO.")
            return 1
        else:
            self.rx_area = value
            return 0

    def get_tx_area(self) -> float:
        return self.tx_area

    def set_tx_area(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static IO.")
            return 1
        else:
            self.tx_area = value
            return 0

    def get_shoreline(self) -> float:
        return self.shoreline

    def set_shoreline(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static IO.")
            return 1
        else:
            self.shoreline = value
            return 0    
    
    def get_bandwidth(self) -> float:
        return self.bandwidth

    def set_bandwidth(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static IO.")
            return 1
        else:
            self.bandwidth = value
            return 0
    
    def get_wire_count(self) -> int:
        return self.wire_count

    def set_wire_count(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static IO.")
            return 1
        else:
            self.wire_count = value
            return 0

    def get_bidirectional(self) -> bool:
        return self.bidirectional
    
    def set_bidirectional(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static IO.")
            return 1
        else:
            if value == "True" or value == "true":
                self.bidirectional = True
            else:
                self.bidirectional = False
            return 0

    def get_energy_per_bit(self) -> float:
        return self.energy_per_bit
    
    def set_energy_per_bit(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static IO.")
            return 1
        else:
            self.energy_per_bit = value
            return 0

    def get_reach(self) -> float:
        return self.reach
    
    def set_reach(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static IO.")
            return 1
        else:
            self.reach = value
            return 0

    def get_static(self) -> bool:
        return self.static

    def set_static(self) -> int:
        if (self.type is None or self.rx_area is None or self.tx_area is None or self.shoreline is None
                 or self.bandwidth is None or self.wire_count is None or self.bidirectional is None or
                 self.energy_per_bit is None or self.reach is None):
            print("Error: Attempt to set IO static without defining all parameters. Exiting...")
        self.static = True
        return 0

    # ===== End Get/Set Functions =====

    # ===== Print Functions =====

    def print_description(self) -> None:
        print("IO Type: " + str(self.get_type())
                + "\n\n\tRX Area: " + str(self.get_rx_area())
                + "\n\n\tTX Area: " + str(self.get_tx_area())
                + "\n\n\tShoreline: " + str(self.get_shoreline())
                + "\n\r\tBandwidth: " + str(self.get_bandwidth())
                + "\n\r\tWire Count: " + str(self.get_wire_count())
                + "\n\r\tBidirectional: " + str(self.get_bidirectional())
                + "\n\r\tEnergy Per Bit: " + str(self.get_energy_per_bit())
                + "\n\n\tStatic: " + str(self.static))
        return

    # ===== End Print Functions =====


# =========================================
# Layer Class
# =========================================
# The class consists of the following attributes:
#   name: The name of the layer.
#   active: Whether the layer is active or not.
#   cost_per_mm2: The cost per mm^2 of the layer.
#   defect_density: The defect density of the layer.
#   critical_area_ratio: The critical area ratio of the layer.
#   clustering_factor: The clustering factor of the layer.
#   litho_percent: The litho percent of the layer.
#   mask_cost: The mask cost of the layer.
#   stitching_yield: The stitching yield of the layer.
#   static: A boolean set true when the layer is defined to prevent further changes.
# =========================================
# The class has the following methods.
# == Get/Set ==
#   get_name()
#   set_name(string)
#   get_active()
#   set_active(bool)
#   get_cost_per_mm2()
#   set_cost_per_mm2(float)
#   get_defect_density()
#   set_defect_density(float)
#   get_critical_area_ratio()
#   set_critical_area_ratio(float)
#   get_clustering_factor()
#   set_clustering_factor(float)
#   get_litho_percent()
#   set_litho_percent(float)
#   get_mask_cost()
#   set_mask_cost(float)
#   get_stitching_yield()
#   set_stitching_yield(float)
#   get_static()
#   set_static()
# == Print ==
#   print_description(): Dumps values of all the parameters for inspection.
# == Computation ==
#   layer_yield(float): Computes the yield of the layer given the area of the layer.
#   reticle_utilization(float,float,float): Computes the reticle utilization given the area of the layer, the reticle x dimension, and the reticle y dimension.
#   layer_cost(float,float,float): Computes the cost of the layer given the area of the layer, the reticle x dimension, and the reticle y dimension.
# =========================================

class Layer:
    def __init__(self, name = None, active = None, cost_per_mm2 = None, defect_density = None, critical_area_ratio = None,
                 clustering_factor = None, litho_percent = None, mask_cost = None, stitching_yield = None, static = True) -> None:
        self.name = name
        self.active = active
        self.cost_per_mm2 = cost_per_mm2
        self.defect_density = defect_density
        self.critical_area_ratio = critical_area_ratio
        self.clustering_factor = clustering_factor
        self.litho_percent = litho_percent
        self.mask_cost = mask_cost
        self.stitching_yield = stitching_yield
        self.static = static
        if (self.name is None or self.active is None or self.cost_per_mm2 is None or self.defect_density is None or
                     self.critical_area_ratio is None or self.clustering_factor is None or self.litho_percent is None or
                     self.mask_cost is None or self.stitching_yield is None):
            print("Warning: Layer not fully defined. Setting non-static.")
            self.static = False
            print("IO " + self.name + " has parameters active = " + str(self.active) + ", cost_per_mm2 = " + str(self.cost_per_mm2) +
                  ", defect_density = " + str(self.defect_density) + ", critical_area_ratio = " + str(self.critical_area_ratio) +
                  ", litho_percent = " + str(self.litho_percent) + ", mask_cost = " + str(self.mask_cost) + ", stitching_yield = " +
                  str(self.stitching_yield) + ".")
        return

    # =========== Get/Set Functions ===========

    def get_name(self) -> str:
        return self.name

    def set_name(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static layer.")
            return 1
        else:
            self.name = value
            return 0

    def get_active(self) -> bool:
        return self.active

    def set_active(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static layer.")
            return 1
        else:
            self.active = value
            return 0

    def get_cost_per_mm2(self) -> float:
        return self.cost_per_mm2

    def set_cost_per_mm2(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static layer.")
            return 1
        else:
            self.cost_per_mm2 = value
            return 0

    def get_defect_density(self) -> float:
        return self.defect_density

    def set_defect_density(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static layer.")
            return 1
        else:
            self.defect_density = value
            return 0

    def get_critical_area_ratio(self) -> float:
        return self.critical_area_ratio

    def set_critical_area_ratio(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static layer.")
            return 1
        else:
            self.critical_area_ratio = value
            return 0

    def get_clustering_factor(self) -> float:
        return self.clustering_factor

    def set_clustering_factor(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static layer.")
            return 1
        else:
            self.clustering_factor = value
            return 0

    def get_litho_percent(self) -> float:
        return self.litho_percent
    
    def set_litho_percent(self, value) -> int: 
        if (self.static):
            print("Error: Cannot change static layer.")
            return 1
        else:
            self.litho_percent = value
            return 0

    def get_mask_cost(self) -> float:
        return self.mask_cost

    def set_mask_cost(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static layer.")
            return 1
        else:
            self.mask_cost = value
            return 0
    
    def get_stitching_yield(self) -> float:
        return self.stitching_yield

    def set_stitching_yield(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static layer.")
            return 1
        else:
            self.stitching_yield = value
            return 0

    def get_static(self) -> bool:
        return self.static 

    def set_static(self) -> int: # The static variable should act somewhat like a latch, when it is set, it should not get unset.
        if (self.name is None or self.active is None or self.cost_per_mm2 is None or self.defect_density is None or
                     self.critical_area_ratio is None or self.clustering_factor is None or self.litho_percent is None or
                     self.mask_cost is None or self.stitching_yield is None):
            print("Error: Attempt to set layer static without defining all parameters. Exiting...")
        self.static = True
        return 0

    # ======== End of Get/Set Functions ========

    # =========== Print Functions ==============

    def print_description(self) -> None:
        print("Layer " + self.name + " has parameters \n\r\t active = " + str(self.active) 
                + "\n\r\t cost_per_mm2 = " + str(self.cost_per_mm2)
                + "\n\r\t defect_density = " + str(self.defect_density)
                + "\n\r\t critical_area_ratio = " + str(self.critical_area_ratio)
                + "\n\r\t clustering_factor = " + str(self.clustering_factor)
                + "\n\r\t mask_cost = " + str(self.mask_cost)
                + "\n\r\t stitching_yield = " + str(self.stitching_yield)
                + "\n\r\t static = " + str(self.static) + ".")
        return  

    # ======== End of Print Functions ==========

    # ========== Computation Functions =========

    def layer_yield(self,area) -> float:
        num_stitches = 0
        defect_yield = (1+(self.defect_density*area*self.critical_area_ratio)/self.clustering_factor)**(-1*self.clustering_factor)
        stitching_yield = self.stitching_yield**num_stitches
        final_layer_yield = stitching_yield*defect_yield
        return final_layer_yield

    def reticle_utilization(self,area,reticle_x,reticle_y) -> float:
        reticle_area = reticle_x*reticle_y
        # If the area is larger than the reticle area, this requires stitching. To get the reticle utilization,
        #  increase the reticle area to the lowest multiple of the reticle area that will fit the stitched chip.
        while reticle_area < area:
            reticle_area += reticle_x*reticle_y
        number_chips_in_reticle = reticle_area//area
        unutilized_reticle = (reticle_area) - number_chips_in_reticle*area
        reticle_utilization = (reticle_area - unutilized_reticle)/(reticle_area)
        return reticle_utilization
        
    # Compute the cost of the layer given area and chip dimensions.
    def layer_cost(self,area,wafer_process) -> float:
        # If area is 0, the cost is 0.
        if area == 0:
            layer_cost = 0
            
        # For valid nonzero area, compute the cost.
        elif area > 0:
            # Compute the cost of the layer before considering scaling of lithography costs with reticle fit.
            layer_cost = area*self.compute_cost_per_mm2(area,wafer_process)

            # Get utilization based on reticle fit.
            # Edge case to avoid division by zero.
            if (self.litho_percent == 0.0):
                reticle_utilization = 1.0
            elif (self.litho_percent > 0.0):
                reticle_utilization = self.reticle_utilization(area,wafer_process.get_reticle_x(),wafer_process.get_reticle_y())
            # A negative percent does not make sense and should crash the program.
            else:
                print("Error: Negative litho percent in Layer.layer_cost(). Exiting...")
                sys.exit(1)

            # Scale the lithography component of cost by the reticle utilization.
            layer_cost = layer_cost*(1-self.litho_percent) + (layer_cost*self.litho_percent)/reticle_utilization

        # Negative area does not make sense and should crash the program.
        else:
            print("Error: Negative area in Layer.layer_cost(). Exiting...")
            sys.exit(1)

        return layer_cost

    def compute_cost_per_mm2(self, square_area, wafer_process) -> float:
        # Access parameters that will be used multiple times.
        wafer_diameter = wafer_process.get_wafer_diameter()
        grid_fill = wafer_process.get_wafer_fill_grid()

        # Find effective wafer diameter that is valid for placing dies.
        usable_wafer_diameter = wafer_diameter - 2*wafer_process.get_edge_exclusion()
        # Get the side length of the die assuming it is a square.
        square_side = math.sqrt(square_area) + wafer_process.get_dicing_distance()

        if (square_side * math.sqrt(2) > usable_wafer_diameter / 2): 
            return 1e09

        # Two different methods are supported for filling the wafer with dies:
        #  1. Grid fill: The dies are placed in a grid pattern where both horizontal and vertical edges must line up.
        #  2. Line fill: The dies are placed one row at a time without requiring vertical edges to line up.

        # The number of dies that fit is estimated by taking the best of two cases:
        #  1. The dies of the first placed row are centered on the diameter line of the circle.
        #  2. The dies of the first placed rows are above and below the diameter line of the circle.

        # Case 1: Dies are centered on the diameter line of the circle.
        num_squares_case_1 = 0
        # Compute the length of a chord that intersects the circle half the square's side length away from the center of the circle.
        row_chord_height = square_side/2
        chord_length = math.sqrt((usable_wafer_diameter/2)**2 - (row_chord_height)**2)*2
        # Compute the number of squares that fit along the chord length.
        num_squares_case_1 += math.floor(chord_length/square_side)
        # Update the row chord height for the next row and start iterating.
        row_chord_height += square_side
        while row_chord_height < usable_wafer_diameter/2:
            # Compute the length of a chord that intersects the circle half the square's side length away from the center of the circle.
            chord_length = math.sqrt((usable_wafer_diameter/2)**2 - (row_chord_height)**2)*2

            starting_distance_from_left = None

            if not grid_fill:
                # For the Line Fill case, compute the maximum number of squares that can fit along the chord length.
                num_squares_case_1 += 2*math.floor(chord_length/square_side)
            else:
                # For the Grid Fill case, the dies need to fit on top of the dies of the previous row.
                if starting_distance_from_left is None:
                    # TODO: Doublecheck the starting_distance_from_left and starting_location calculations.
                    starting_distance_from_left = (usable_wafer_diameter - chord_length)/2
                    num_squares_case_1 += 2*math.floor(chord_length/square_side)
                else:
                    # Compute how many squares over from the first square it is possible to fit an other square on top.
                    location_of_first_fit_candidate = (usable_wafer_diameter - chord_length)/2
                    starting_location = math.ceil((location_of_first_fit_candidate - starting_distance_from_left)/square_side)*square_side + starting_distance_from_left
                    effective_cord_length = chord_length - (starting_location - location_of_first_fit_candidate)
                    num_squares_case_1 += 2*math.floor(effective_cord_length/square_side)
            
            row_chord_height += square_side

        # Case 2: Dies are above and below the diameter line of the circle.
        num_squares_case_2 = 0
        # Compute the length of a chord that intersects the circle the square's side length away from the center of the circle.
        row_chord_height = square_side
        chord_length = math.sqrt((usable_wafer_diameter/2)**2 - (row_chord_height)**2)*2
        num_squares_case_2 += 2*math.floor(chord_length/square_side)
        row_chord_height += square_side
        if grid_fill:
            starting_distance_from_left = None
        while row_chord_height < usable_wafer_diameter/2:
            # Compute the length of a chord that intersects the circle half the square's side length away from the center of the circle.
            chord_length = math.sqrt((usable_wafer_diameter/2)**2 - (row_chord_height)**2)*2
            if not grid_fill:
                num_squares_case_2 += 2*math.floor(chord_length/square_side)
            else:
                if starting_distance_from_left is None:
                    starting_distance_from_left = (usable_wafer_diameter - chord_length)/2
                    num_squares_case_2 += 2*math.floor(chord_length/square_side)
                else:
                    # Compute how many squares over from the first square it is possible to fit an other square on top.
                    location_of_first_fit_candidate = (usable_wafer_diameter - chord_length)/2
                    starting_location = math.ceil((location_of_first_fit_candidate - starting_distance_from_left)/square_side)*square_side + starting_distance_from_left
                    effective_cord_length = chord_length - (starting_location - location_of_first_fit_candidate)
                    num_squares_case_2 += 2*math.floor(effective_cord_length/square_side)

            row_chord_height += square_side
        
        # Find the maximum of the two cases.
        num_squares = max(num_squares_case_1, num_squares_case_2)

        if (square_area == 0):
            print("Square area is zero. Exiting...")
            sys.exit(1)

        if (num_squares == 0):
            print("Number squares is zero. Exiting...")
            sys.exit(1)

        # Compute the cost per mm^2.
        used_area = num_squares*square_area
        circle_area = math.pi*(wafer_diameter/2)**2
        cost_per_mm2 = self.cost_per_mm2*circle_area/used_area
        return cost_per_mm2

    # ===== End of Computation Functions =======


# =========================================
# Assembly Definition Class
# =========================================
# The class has attributes:
#   name: The name of the assembly process.
#   machine_cost_list: The cost of the machine for each year of its lifetime.
#   machine_lifetime_list: The lifetime of the machine in years.
#   machine_uptime_list: The uptime of the machine in hours per year.
#   technician_yearly_cost_list: The cost of the technician for each year of the machine's lifetime.
#   materials_cost_per_mm2: The cost of the materials per mm^2 of the assembly.
#   picknplace_time: The time it takes to pick and place a die in seconds.
#   picknplace_group: The number of dies that can be picked and placed at once.
#   bonding_time: The time it takes to bond a die in seconds.
#   bonding_group: The number of dies that can be bonded at once.
#   die_separation: The distance between the dies in mm.
#   edge_exclusion: The distance from the edge of the wafer to the first die in mm.
#   max_pad_current_density: The maximum current density of the pads in mA/mm^2.
#   bonding_pitch: The pitch of the bonding pads in mm.
#   alignment_yield: The yield of the alignment process.
#   bonding_yield: The yield of the bonding process.
#   dielectric_bond_defect_density: The defect density of the dielectric bond.
#   static: A boolean set true when the assembly process is defined to prevent further changes.
# =========================================
# The class has the following methods.
# == Get/Set ==
#   get_name()
#   set_name(string)
##   get_machine_cost_list_len()
##   get_machine_cost_list()
##   set_machine_cost_list(list)
##   get_machine_cost(int)
##   set_machine_cost(int,float)
##   get_machine_lifetime_list_len()
##   get_machine_lifetime_list()
##   set_machine_lifetime_list(list)
##   get_machine_lifetime(int)
##   set_machine_lifetime(int,float)
##   get_machine_uptime_list_len()
##   get_machine_uptime_list()
##   set_machine_uptime_list(list)
##   get_machine_uptime(int)
##   set_machine_uptime(int,float)
##   get_technician_yearly_cost_list_len()
##   get_technician_yearly_cost_list()
##   set_technician_yearly_cost_list(list)
##   get_technician_yearly_cost(int)
##   set_technician_yearly_cost(int,float)
#   get_materials_cost_per_mm2()
#   set_materials_cost_per_mm2(float)
#   get_picknplace_machine_cost()
#   set_picknplace_machine_cost(float)
#   get_picknplace_machine_lifetime()
#   set_picknplace_machine_lifetime(float)
#   get_picknplace_machine_uptime()
#   set_picknplace_machine_uptime(float)
#   get_picknplace_technician_yearly_cost()
#   set_picknplace_technician_yearly_cost(float)
#   get_picknplace_time()
#   set_picknplace_time(float)
#   get_picknplace_group()
#   set_picknplace_group(int)
#   get_bonding_machine_cost()
#   set_bonding_machine_cost(float)
#   get_bonding_machine_lifetime()
#   set_bonding_machine_lifetime(float)
#   get_bonding_machine_uptime()
#   set_bonding_machine_uptime(float)
#   get_bonding_technician_yearly_cost()
#   set_bonding_technician_yearly_cost(float)
#   get_bonding_time()
#   set_bonding_time(float)
#   get_bonding_group()
#   set_bonding_group(int)
#   get_dieSeparation()
#   set_dieSeparation(float)
#   get_edgeExclusion()
#   set_edgeExclusion(float)
#   get_max_pad_current_density()
#   set_max_pad_current_density(float)
#   get_bonding_pitch()
#   set_bonding_pitch(float)
#   get_alignment_yield()
#   set_alignment_yield(float)
#   get_bonding_yield()
#   set_bonding_yield(float)
#   get_dielectric_bond_defect_density()
#   set_dielectric_bond_defect_density(float)
#   get_static()
#   set_static()
# == Print ==
#   print_description(): Dumps values of all the parameters for inspection.
# == Other ==
#   compute_picknplace_time(int): Computes the time it takes to pick and place a given number of dies.
#   compute_bonding_time(int): Computes the time it takes to bond a given number of dies.
#   assembly_time(float): Computes the assembly time given the area of the die in mm^2.
#   compute_assembly_cost_per_second(): Computes the cost per second of the assembly process.
#   assembly_cost(float): Computes the cost of the assembly process given the area of the die in mm^2.
#   assembly_yield(float): Computes the yield of the assembly process given the area of the die in mm^2.
# =========================================

class Assembly:
    def __init__(self, name = "", materials_cost_per_mm2 = None, picknplace_machine_cost = None,
                 picknplace_machine_lifetime = None, picknplace_machine_uptime = None, picknplace_technician_yearly_cost = None,
                 picknplace_time = None, picknplace_group = None, bonding_machine_cost = None, bonding_machine_lifetime = None,
                 bonding_machine_uptime = None, bonding_machine_technician_yearly_cost = None, bonding_time = None,
                 bonding_group = None, die_separation = None, edge_exclusion = None, max_pad_current_density = None,
                 bonding_pitch = None, alignment_yield = None, bonding_yield = None, dielectric_bond_defect_density = None,
                 static = True) -> None:
#    def __init__(self, name = "", machine_cost_list = [], machine_lifetime_list = [], machine_uptime_list = [], technician_yearly_cost_list = [], materials_cost_per_mm2 = None, picknplace_time = None, picknplace_group = None, bonding_time = None, bonding_group = None, die_separation = None, edge_exclusion = None, max_pad_current_density = None, bonding_pitch = None, alignment_yield = None, bonding_yield = None, dielectric_bond_defect_density = None, static = True) -> None:
        self.name = name
#        self.machine_cost_list = machine_cost_list
#        self.machine_lifetime_list = machine_lifetime_list
#        self.machine_uptime_list = machine_uptime_list
#        self.technician_yearly_cost_list = technician_yearly_cost_list
        self.materials_cost_per_mm2 = materials_cost_per_mm2
        self.picknplace_machine_cost = picknplace_machine_cost
        self.picknplace_machine_lifetime = picknplace_machine_lifetime
        self.picknplace_machine_uptime = picknplace_machine_uptime
        self.picknplace_technician_yearly_cost = picknplace_technician_yearly_cost
        self.picknplace_time = picknplace_time
        self.picknplace_group = picknplace_group
        self.bonding_machine_cost = bonding_machine_cost
        self.bonding_machine_lifetime = bonding_machine_lifetime
        self.bonding_machine_uptime = bonding_machine_uptime
        self.bonding_machine_technician_yearly_cost = bonding_machine_technician_yearly_cost
        self.bonding_time = bonding_time
        self.bonding_group = bonding_group
        self.die_separation = die_separation                # Given Parameter
        self.edge_exclusion = edge_exclusion                # Given Parameter
        self.max_pad_current_density = max_pad_current_density  # Given Parameter
        self.bonding_pitch = bonding_pitch
        self.picknplace_cost_per_second = None
        self.bonding_cost_per_second = None
        self.bonding_yield = bonding_yield
        self.alignment_yield = alignment_yield
        self.dielectric_bond_defect_density = dielectric_bond_defect_density
        self.static = static
        if self.name == "" or self.machine_cost_list == [] or self.machine_lifetime_list == [] or self.machine_uptime_list == [] or self.technician_yearly_cost_list == [] or self.materials_cost_per_mm2 is None or self.picknplace_time is None or self.picknplace_group is None or self.bonding_time is None or self.bonding_group is None:
            # print("Warning: Assembly not fully defined. Setting non-static.")
            self.static = False
            # print("Assembly process " + self.name + " has parameters machine_cost_list = " + str(self.machine_cost_list) + ", machine_lifetime_list = " + str(self.machine_lifetime_list) + ", machine_uptime_list = " + str(self.machine_uptime_list) + ", technician_yearly_cost_list = " + str(self.technician_yearly_cost_list) + ", materials_cost_per_mm2 = " + str(self.materials_cost_per_mm2) + ", picknplace_time = " + str(self.picknplace_time) + ", picknplace_group = " + str(self.picknplace_group) + ", bonding_time = " + str(self.bonding_time) + ", bonding_group = " + str(self.bonding_group) + ".")
        else:
            self.compute_picknplace_cost_per_second()
            self.compute_bonding_cost_per_second()
            self.static = False
        
        return

    # ====== Get/Set Functions ======

    def get_name(self) -> str:
        return self.name

    def set_name(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static assembly process.")
            return 1
        else:
            self.name = value
            return 0
    
#    def get_machine_cost_list_len(self) -> int:
#        return len(self.machine_cost_list)
#
#    def get_machine_cost_list(self) -> list:
#        return self.machine_cost_list
#
#    def set_machine_cost_list(self, value) -> int:
#        if (self.static):
#            print("Error: Cannot change static assembly process.")
#            return 1
#        else:
#            self.machine_cost_list = value
#            return 0
#    
#    def get_machine_cost(self, index) -> float:
#        return self.machine_cost_list[index]
#
#    def set_machine_cost(self, index, value) -> int:
#        if (self.static):
#            print("Error: Cannot change static assembly process.")
#            return 1
#        else:
#            self.machine_cost_list[index] = value
#            return 0
#
#    def set_machine_lifetime_list(self, value) -> int:
#        if (self.static):
#            print("Error: Cannot change static assembly process.")
#            return 1
#        else:
#            self.machine_lifetime_list = value
#            return 0
#    
#    def get_machine_lifetime(self, index) -> float:
#        return self.machine_lifetime_list[index]
#
#    def set_machine_lifetime(self, index, value) -> int:
#        if (self.static):
#            print("Error: Cannot change static assembly process.")
#            return 1
#        else:
#            self.machine_lifetime_list[index] = value
#            return 0
#    
#    def get_machine_uptime_list_len(self) -> int:
#        return len(self.machine_uptime_list)
#
#    def get_machine_uptime_list(self) -> list:
#        return self.machine_uptime_list
#
#    def set_machine_uptime_list(self, value) -> int:
#        if (self.static):
#            print("Error: Cannot change static assembly process.")
#            return 1
#        else:
#            self.machine_uptime_list = value
#            return 0
#    
#    def get_machine_uptime(self, index) -> float:
#        return self.machine_uptime_list[index]
#
#    def set_machine_uptime(self, index, value) -> int:
#        if (self.static):
#            print("Error: Cannot change static assembly process.")
#            return 1
#        else:
#            self.machine_uptime_list[index] = value
#            return 0
#
#    def get_technician_yearly_cost_list_len(self) -> int:
#        return len(self.technician_yearly_cost_list)
#
#    def get_technician_yearly_cost_list(self) -> list:
#        return self.technician_yearly_cost_list
#
#    def set_technician_yearly_cost_list(self, value) -> int:
#        if (self.static):
#            print("Error: Cannot change static assembly process.")
#            return 1
#        else:
#            self.technician_yearly_cost_list = value
#            return 0
#    
#    def get_technician_yearly_cost(self, index) -> float:
#        return self.technician_yearly_cost_list[index]
#
#    def set_technician_yearly_cost(self, index, value) -> int:
#        if (self.static):
#            print("Error: Cannot change static assembly process.")
#            return 1
#        else:
#            self.technician_yearly_cost_list[index] = value
#            return 0
    
    def get_materials_cost_per_mm2(self) -> float:
        return self.materials_cost_per_mm2

    def set_materials_cost_per_mm2(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static assembly process.")
            return 1
        else:
            self.materials_cost_per_mm2 = value
            return 0
    
    def get_picknplace_machine_cost(self) -> float:
        return self.picknplace_machine_cost
    
    def set_picknplace_machine_cost(self, value) -> float:
        if (self.static):
            print("Error: Cannot change static assembly process.")
            return 1
        else:
            self.picknplace_machine_cost = value
            return 0

    def get_picknplace_machine_lifetime(self) -> float:
        return self.picknplace_machine_lifetime
    
    def set_picknplace_machine_lifetime(self, value) -> float:
        if (self.static):
            print("Error: Cannot change static assembly process.")
            return 1
        else:
            self.picknplace_machine_lifetime = value
            return 0
        
    def get_picknplace_machine_uptime(self) -> float:
        return self.picknplace_machine_uptime
    
    def set_picknplace_machine_uptime(self, value) -> float:
        if (self.static):
            print("Error: Cannot change static assembly process.")
            return 1
        else:
            self.picknplace_machine_uptime = value
            return 0 

    def get_picknplace_technician_yearly_cost(self) -> float:
        return self.picknplace_technician_yearly_cost
    
    def set_picknplace_technician_yearly_cost(self, value) -> float:
        if (self.static):
            print("Error: Cannot change static assembly process.")
            return 1
        else:
            self.picknplace_technician_yearly_cost = value
            return 0
    
    def get_picknplace_time(self) -> float:
        return self.picknplace_time

    def set_picknplace_time(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static assembly process.")
            return 1
        else:
            self.picknplace_time = value
            return 0

    def get_picknplace_group(self) -> str:
        return self.picknplace_group

    def set_picknplace_group(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static assembly process.")
            return 1
        else:
            self.picknplace_group = value
            return 0

    def get_bonding_machine_cost(self) -> float:
        return self.bonding_machine_cost
    
    def set_bonding_machine_cost(self, value) -> float:
        if (self.static):
            print("Error: Cannot change static assembly process.")
            return 1
        else:
            self.bonding_machine_cost = value
            return 0
        
    def get_bonding_machine_lifetime(self) -> float:
        return self.bonding_machine_lifetime
    
    def set_bonding_machine_lifetime(self, value) -> float:
        if (self.static):
            print("Error: Cannot change static assembly process.")
            return 1
        else:
            self.bonding_machine_lifetime = value
            return 0

    def get_bonding_machine_uptime(self) -> float:
        return self.bonding_machine_uptime

    def set_bonding_machine_uptime(self, value) -> float:
        if (self.static):
            print("Error: Cannot change static assembly process.")
            return 1
        else:
            self.bonding_machine_uptime = value
            return 0
 
    def get_bonding_technician_yearly_cost(self) -> float:
        return self.bonding_machine_technician_yearly_cost
    
    def set_bonding_technician_yearly_cost(self, value) -> float:
        if (self.static):
            print("Error: Cannot change static assembly process.")
            return 1
        else:
            self.bonding_machine_technician_yearly_cost = value
            return 0

    def get_bonding_time(self) -> float:
        return self.bonding_time

    def set_bonding_time(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static assembly process.")
            return 1
        else:
            self.bonding_time = value
            return 0

    def get_bonding_group(self) -> str:
        return self.bonding_group

    def set_bonding_group(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static assembly process.")
            return 1
        else:
            self.bonding_group = value
            return 0

    def get_die_separation(self) -> float:
        return self.die_separation
    
    def set_die_separation(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.die_separation = value
            return 0
        
    def get_edge_exclusion(self) -> float:
        return self.edge_exclusion
    
    def set_edge_exclusion(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.edge_exclusion = value
            return 0

    def get_bonding_pitch(self) -> float:
        return self.bonding_pitch
    
    def set_bonding_pitch(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.bonding_pitch = value
            return 0

    def get_max_pad_current_density(self) -> float:
        return self.max_pad_current_density
    
    def set_max_pad_current_density(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.max_pad_current_density = value
            return 0

    def get_power_per_pad(self,core_voltage) -> float:
        pad_area = math.pi*(self.bonding_pitch/4)**2
        current_per_pad = self.max_pad_current_density*pad_area
        power_per_pad = current_per_pad*core_voltage
        return power_per_pad

    def get_alignment_yield(self) -> float:
        return self.alignment_yield
    
    def set_alignment_yield(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.alignment_yield = value
            return 0
        
    def get_bonding_yield(self) -> float:
        return self.bonding_yield
    
    def set_bonding_yield(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.bonding_yield = value
            return 0

    def get_dielectric_bond_defect_density(self) -> float:
        return self.dielectric_bond_defect_density
    
    def set_dielectric_bond_defect_density(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.dielectric_bond_defect_density = value
            return 0

    def get_picknplace_cost_per_second(self) -> float:
        if self.picknplace_cost_per_second is None:
            self.compute_picknplace_cost_per_second()
        return self.picknplace_cost_per_second
    
    def set_picknplace_cost_per_second(self) -> int:
        self.picknplace_cost_per_second = self.compute_picknplace_cost_per_second()
        return 0

    def get_bonding_cost_per_second(self) -> float:
        if self.bonding_cost_per_second is None:
            self.compute_bonding_cost_per_second()
        return self.bonding_cost_per_second
    
    def set_bonding_cost_per_second(self) -> int:
        self.bonding_cost_per_second = self.compute_bonding_cost_per_second()
        return 0

    def get_static(self) -> bool:
        return self.static    

    def set_static(self) -> int:
        self.static = True
        return 0 

    # ===== End of Get/Set Functions =====

    # ===== Print Functions =====

    def print_description(self):
        print("Assembly Process Description:")
        print("\tPick and Place Group: " + str(self.get_picknplace_group()))
        print("\tPick and Place Time: " + str(self.get_picknplace_time()))
        print("\tBonding Group: " + str(self.get_bonding_group()))
        print("\tBonding Time: " + str(self.get_bonding_time()))
        print("\tMaterials Cost per mm2: " + str(self.get_materials_cost_per_mm2()))
        print("\tMachine Cost is " + str(self.get_picknplace_machine_cost()) + " for the picknplace machine and " + str(self.get_bonding_machine_cost()) + " for the bonding machine.")
        print("\tTechnician Yearly Cost is " + str(self.get_picknplace_technician_yearly_cost()) + " for picknplace and " + str(self.get_bonding_technician_yearly_cost()) + " for bonding.")
        print("\tMachine Uptime is " + str(self.get_picknplace_machine_uptime()) + " for the picknplace machine and " + str(self.get_bonding_machine_uptime()) + " for the bonding machine.")
        print("\tMachine Lifetime is " + str(self.get_picknplace_machine_lifetime()) + " for the picknplace machine and " + str(self.get_bonding_machine_lifetime()) + " for the bonding machine.")
        return

    # ===== End of Print Functions =====

    # ===== Other Functions =====

    def compute_picknplace_time(self, n_chips):
        picknplace_steps = math.ceil(n_chips/self.picknplace_group)
        time = self.picknplace_time*picknplace_steps
        return time
    
    def compute_bonding_time(self, n_chips):
        bonding_steps = math.ceil(n_chips/self.bonding_group)
        time = self.bonding_time*bonding_steps
        return time
    
    def assembly_time(self, n_chips):
        time = self.compute_picknplace_time() + self.compute_bonding_time()
        return time

    def compute_picknplace_cost_per_second(self):
        machine_cost_per_year = self.get_picknplace_machine_cost()/self.get_picknplace_machine_lifetime()
        technician_cost_per_year = self.get_picknplace_technician_yearly_cost()
        picknplace_cost_per_year = machine_cost_per_year + technician_cost_per_year
        picknplace_cost_per_second = picknplace_cost_per_year/(365*24*60*60)*self.get_picknplace_machine_uptime()
        self.picknplace_cost_per_second = picknplace_cost_per_second
        return picknplace_cost_per_second
    
    def compute_bonding_cost_per_second(self):
        machine_cost_per_year = self.get_bonding_machine_cost()/self.get_bonding_machine_lifetime()
        technician_cost_per_year = self.get_bonding_technician_yearly_cost()
        bonding_cost_per_year = machine_cost_per_year + technician_cost_per_year
        bonding_cost_per_second = bonding_cost_per_year/(365*24*60*60)*self.get_bonding_machine_uptime()
        self.bonding_cost_per_second = bonding_cost_per_second
        return bonding_cost_per_second

    # Assembly cost includes cost of machine time and materials cost.
    def assembly_cost(self, n_chips, area):
        # TODO: Remove critical bonds from the argument list here since yield is calculated elsewhere.
        assembly_cost = self.get_picknplace_cost_per_second()*self.compute_picknplace_time(n_chips) + self.get_bonding_cost_per_second()*self.compute_bonding_time(n_chips)
        assembly_cost += self.get_materials_cost_per_mm2()*area
        return assembly_cost

    def assembly_yield(self, n_chips, n_bonds, area):
        assem_yield = 1.0
        assem_yield *= self.alignment_yield**n_chips
        assem_yield *= self.bonding_yield**n_bonds

        # If hybrid bonding, there is some yield impact of the dielectric bond.
        # This uses a defect density and area number which approximates the negative binomial yield model with no clustering.
        dielectric_bond_area = area
        dielectric_bond_yield = 1 - self.get_dielectric_bond_defect_density()*dielectric_bond_area
        assem_yield *= dielectric_bond_yield

        return assem_yield

    # ===== End of Other Functions =====

# Test Definition Class
# The class has attributes:
#   name: string
#   static: boolean
# The class has methods:
#   get_name: returns name
#   set_name: sets name
#   get_static: returns static
#   set_static: sets static to True
class Test:
    def __init__(self, name = None, test_self = None, test_assembly = None, self_defect_coverage = None,
                 assembly_defect_coverage = None, self_test_cost_per_mm2 = None, assembly_test_cost_per_mm2 = None,
                 self_pattern_count = None, assembly_pattern_count = None, self_test_failure_dist=None, assembly_test_failure_dist=None, static = True) -> None:
        self.name = name
        self.static = static
        self.test_self = test_self
        self.test_assembly = test_assembly
        self.self_defect_coverage = self_defect_coverage
        self.assembly_defect_coverage = assembly_defect_coverage
        self.self_test_cost_per_mm2 = self_test_cost_per_mm2
        self.assembly_test_cost_per_mm2 = assembly_test_cost_per_mm2
        self.self_pattern_count = self_pattern_count
        self.assembly_pattern_count = assembly_pattern_count
        self.self_test_failure_dist = self_test_failure_dist
        self.assembly_test_failure_dist = assembly_test_failure_dist
        if self.name is None:
            print("Warning: Test not fully defined. Setting non-static.")
            self.static = False
            print("Test has name " + self.name + ".")
        return

    # ===== Get/Set Functions =====

    def get_name(self) -> str:
        return self.name
    
    def set_name(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static testing.")
            return 1
        else:
            self.name = value
            return 0

    def get_static(self) -> bool:
        return self.static
        
    def set_static(self) -> int:
        self.static = True
        return 0
    
    def get_test_self(self) -> bool:
        return self.test_self

    def set_test_self(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static testing.")
            return 1
        else:
            self.test_self = value
            return 0

    def get_test_assembly(self) -> bool:
        return self.test_assembly

    def set_test_assembly(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static testing.")
            return 1
        else:
            self.test_assembly = value
            return 0
    
    def get_self_defect_coverage(self) -> float:
        return self.self_defect_coverage

    def set_self_defect_coverage(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static testing.")
            return 1
        else:
            self.self_defect_coverage = value
            return 0

    def get_assembly_defect_coverage(self) -> float:
        return self.assembly_defect_coverage

    def set_assembly_defect_coverage(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static testing.")
            return 1
        else:
            self.assembly_defect_coverage = value
            return 0
        
    def get_self_test_cost_per_mm2(self) -> float:
        return self.self_test_cost_per_mm2

    def set_self_test_cost_per_mm2(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static testing.")
            return 1
        else:
            self.self_test_cost_per_mm2 = value
            return 0

    def get_assembly_test_cost_per_mm2(self) -> float:
        return self.assembly_test_cost_per_mm2

    def set_assembly_test_cost_per_mm2(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static testing.")
            return 1
        else:
            self.assembly_test_cost_per_mm2 = value
            return 0

    def get_self_pattern_count(self) -> int:
        return self.self_pattern_count
    
    def set_self_pattern_count(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static testing.")
            return 1
        else:
            self.self_pattern_count = value
            return 0
        
    def get_assembly_pattern_count(self) -> int:
        return self.assembly_pattern_count
    
    def set_assembly_pattern_count(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static testing.")
            return 1
        else:
            self.assembly_pattern_count = value
            return 0

    def get_self_test_failure_dist(self) -> int:
        return self.self_test_failure_dist
    
    def set_self_test_failure_dist(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static testing.")
            return 1
        else:
            self.self_test_failure_dist = value
            return 0
        
    def get_assembly_test_failure_dist(self) -> int:
        return self.assembly_test_failure_dist
    
    def set_assembly_test_failure_dist(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static testing.")
            return 1
        else:
            self.assembly_test_failure_dist = value
            return 0

    # ===== End of Get/Set Functions =====

    # ===== Print Functions =====

    def print_description(self) -> None:
        print("Test: " + self.name)
        print("Test_self: " + self.test_self)
        print("Test_assembly: " + self.test_assembly)
        print("Defect_coverage: " + self.defect_coverage)
        print("Test_cost: " + self.test_cost)
        print("Static: " + str(self.static))
        return

    # ===== End of Print Functions =====

    # ===== Other Functions =====
    # This is the yield based on number of chips that pass test.
    def compute_self_test_yield(self, chip) -> float:
        if self.get_test_self() == True:
            true_yield = chip.get_self_true_yield()
            test_yield = 1-(1-true_yield)*float(self.get_self_defect_coverage())
        else:
            test_yield = 1.0
        return test_yield

    def compute_self_quality(self, chip) -> float:
        test_yield = chip.get_self_test_yield()
        true_yield = chip.get_self_true_yield()

        quality = true_yield/test_yield

        return quality

    def compute_assembly_test_yield(self, chip) -> float:
        assembly_true_yield = chip.get_chip_true_yield()

        if self.get_test_assembly() == True:
            assembly_test_yield = 1.0-(1.0-assembly_true_yield)*self.assembly_defect_coverage
        else:
            assembly_test_yield = 1.0

        return assembly_test_yield

    def compute_assembly_quality(self, chip) -> float:
        assembly_true_yield = chip.get_chip_true_yield()
        
        assembly_test_yield = chip.get_chip_test_yield()

        assembly_quality = assembly_true_yield/assembly_test_yield

        return assembly_quality

    def compute_self_test_cost(self, chip) -> float:
        if (self.get_test_self() == False):
            test_cost = 0.0
        else:
            test_cost = chip.get_core_area()*self.get_self_test_cost_per_mm2()*self.get_self_pattern_count() #Will need to add pattern count in test def.xml, and chain length parameters in sip.xml
            if(self.get_self_test_failure_dist() == 1): #Normal Distribution
                derating_factor = random.gauss(chip.get_chip_true_yield(),chip.get_chip_true_yield()*0.05)
            elif(self.get_self_test_failure_dist() == 2): #Uniform Distribution
                derating_factor = 1.0 #This means no deration
            elif(self.get_self_test_failure_dist() == 3): #Exponential Distribution
                derating_factor = random.expovariate(1/chip.get_chip_true_yield())
            elif(self.get_self_test_failure_dist() == 4):
                derating_factor = random.lognormvariate(math.log(chip.get_chip_true_yield()),0.05)
            test_cost = derating_factor*test_cost
        return test_cost

    def compute_assembly_test_cost(self, chip) -> float:
        if (self.get_test_assembly() == False):
            test_cost = 0.0
        else:
            area = chip.get_core_area()
            chips = chip.get_chips()
            for c in chips:
                area += c.get_core_area()
            
            test_cost = area*self.get_assembly_test_cost_per_mm2()*self.get_assembly_pattern_count() #Will need to add pattern count in test def.xml, and chain length parameters in sip.xml
            if(self.assembly_test_failure_dist == 1): #Normal Distribution
                derating_factor = random.gauss(chip.get_chip_true_yield(),chip.get_chip_true_yield()*0.05)
            elif(self.assembly_test_failure_dist == 1): #Uniform Distribution
                derating_factor = 1.0 #This means no deration
            elif(self.assembly_test_failure_dist == 2): #Exponential Distribution
                derating_factor = random.expovariate(1/chip.get_chip_true_yield())
            elif(self.assembly_test_failure_dist == 3):
                derating_factor = random.lognormvariate(math.log(chip.get_chip_true_yield()),0.05)
            test_cost = derating_factor*test_cost 
        return test_cost


    # ===== End of Other Functions =====

# =========================================
# Chip Class
# =========================================
# The class has attributes:
#   name: The name of the chip.
#   coreArea: The area of the core in mm^2.
#   cost: The cost of the chip in dollars.
#   chip_true_yield: The yield of the chip.
#   quality: The quality of the chip.
#   assembly_process: The assembly process used to assemble the chip.
#   stackup: The stackup of the chip.
#   chips: The list of chips that are stacked in this chip.
#   adjacencyMatrixList: The list of adjacency matrices for the chip.
#   power: The power of the chip in Watts.
#   static: A boolean set true when the chip is defined to prevent further changes.
# =========================================
# The class has the following methods.
# == Get/Set ==
#   get_name()
#   set_name(string)
#   get_core_area()
#   set_core_area(float)
#   get_cost()
#   set_cost(float)
#   get_chip_true_yield()
#   set_chip_true_yield(float)
#   get_quality()
#   set_quality(float)
#   get_assembly_process()
#   set_assembly_process(Assembly)
#   get_stackup()
#   set_stackup(list)
#   get_chips()
#   set_chips(list)
#   get_adjacencyMatrixList()
#   set_adjacencyMatrixList(list)
#   get_power()
#   set_power(float)
#   get_static()
#   set_static()
# == Print ==
#   print_description(): Dumps values of all the parameters for inspection.
# == Other ==
#   compute_area(): Computes the area of the chip in mm^2.
#   compute_cost(): Computes the cost of the chip in dollars.
#   compute_chip_yield(): Computes the yield of the chip.
# =========================================

class Chip:

    # ===== Initialization Functions =====
    # "etree" is an element tree built from the system definition xml file. This can also be built without reading from the xml file and passed to the init function without defining the filename argument.
    def __init__(self, filename = None, etree = None, parent_chip = None, wafer_process_list = None, assembly_process_list = None, test_process_list = None, layers = None, ios = None, adjacency_matrix_definitions = None, average_bandwidth_utilization = None, block_names = None, static = False) -> None:
        # If the critical parameters are not defined, throw an error and exit.
        if wafer_process_list is None:
            print("wafer_process_list is None")
            sys.exit(1)
        elif assembly_process_list is None:
            print("assembly_process_list is None")
            sys.exit(1)
        elif test_process_list is None:
            print("test_process_list is None")
            sys.exit(1)
        elif layers is None:
            print("layers is None")
            sys.exit(1)
        elif ios is None:
            print("ios is None")
            sys.exit(1)
        elif adjacency_matrix_definitions is None:
            print("adjacency_matrix_definitions is None")
            sys.exit(1)
        elif average_bandwidth_utilization is None:
            print("average_bandwidth_utilization is None")
            sys.exit(1)
        elif block_names is None:
            print("block_names is None")
            sys.exit(1)

        root = {}
        self.static = False
        # If the filename is given and the etree is not, read the file and build the etree.
        if filename is not None and filename != "" and etree is None:
            tree = ET.parse(filename)
            root = tree.getroot()
        # If the etree is given and the filename is not, use the etree.
        elif filename == None and etree is not None and etree != {}:
            root = etree
        elif (filename == None or filename == "") and (etree == None or etree == {}):
            print("Error: Invalid chip definition. The filename and etree are both None or empty. Exiting...")
            sys.exit(1) 
        # If neither is given, there is an error. If both are given, this is ambiguous, so it should also throw an error.
        else:
            print("Error: Invalid Chip definition. Filename and etree are both defined leading to possible ambiguity. Exiting...")
            sys.exit(1)

        self.parent_chip = parent_chip
        attributes = root.attrib

        # The following are the class parameter objects. The find_* functions match the correct object with the name given in the chip definition.
        self.wafer_process = self.find_wafer_process(attributes["wafer_process"], wafer_process_list)
        self.assembly_process = self.find_assembly_process(attributes["assembly_process"], assembly_process_list)
        self.test_process = self.find_test_process(attributes["test_process"], test_process_list)
        self.stackup = self.build_stackup(attributes["stackup"], layers)

        # Recursively handle the chips that are stacked on this chip.
        self.chips = []
        for chip_def in root:
            if "chip" in chip_def.tag:
                self.chips.append(Chip(filename = None, etree = chip_def, parent_chip = self, wafer_process_list = wafer_process_list, assembly_process_list = assembly_process_list, test_process_list = test_process_list, layers = layers, ios = ios, adjacency_matrix_definitions = adjacency_matrix_definitions, average_bandwidth_utilization = average_bandwidth_utilization, block_names = block_names, static = static))

        # Set Black-Box Parameters
        bb_area = attributes["bb_area"]
        if bb_area == "":
            self.bb_area = None
        else:
            self.bb_area = float(bb_area)
        bb_cost = attributes["bb_cost"]
        if bb_cost == "":
            self.bb_cost = None
        else:
            self.bb_cost = float(bb_cost)
        bb_quality = attributes["bb_quality"]
        if bb_quality == "":
            self.bb_quality = None
        else:
            self.bb_quality = float(bb_quality)
        bb_power = attributes["bb_power"]
        if bb_power == "":
            self.bb_power = None
        else:
            self.bb_power = float(bb_power)
        aspect_ratio = attributes["aspect_ratio"]
        if aspect_ratio == "":
            self.set_aspect_ratio(1.0)
        else:
            self.set_aspect_ratio(float(aspect_ratio))

        if attributes["x_location"] == "":
            self.set_x_location(None)
        else:
            self.set_x_location(float(attributes["x_location"]))

        if attributes["y_location"] == "":
            self.set_y_location(None)
        else:
            self.set_y_location(float(attributes["y_location"]))

        # Chip name should match the name in the netlist file.
        self.name = attributes["name"]

        # If core area is not given, this is an interposer and only has interconnect, so size is determined from the size of the stacked chiplets.
        # If core area is given, it is possible that the area will be determined by the size of the stacked chiplets or of the IO pads.
        self.core_area = float(attributes["core_area"])

        # NRE Design Cost Depends on the Type of Chip.
        # The following parameters allow defining a chip as a mix of memory, logic, and analog.
        self.fraction_memory = float(attributes["fraction_memory"])
        self.fraction_logic = float(attributes["fraction_logic"])
        self.fraction_analog = float(attributes["fraction_analog"])

        # In the case of a shared reticle, NRE mask costs can be shared. This allows definition of the percentage of reticle costs which are incurred by this chip.
        self.reticle_share = float(attributes["reticle_share"])

        # All NRE costs scale with the quantity of chips produced.
        self.quantity = int(attributes["quantity"])

        # If a chip is defined as buried (such as a bridge die in EMIB), it is not counted in the total area of stacked chips.
        if attributes["buried"] == "True":
            self.buried = True
        elif attributes["buried"] == "False":
            self.buried = False
        else:
            print("Error: Invalid buried parameter. Exiting...")
            sys.exit(1)

        # Store the adjacency matrix, bandwidth utilization, block names, and IO list.
        self.global_adjacency_matrix = adjacency_matrix_definitions
        self.average_bandwidth_utilization = average_bandwidth_utilization
        self.block_names = block_names
        self.io_list = ios

        # Power and Core Voltage are important for determining the number of power pads required.
        self.set_power(float(attributes["power"]))
        self.core_voltage = float(attributes["core_voltage"])

        # If the chip is not fully defined, throw an error and exit.
        if self.name == "":
            print("Error: Chip name is \"\". Exiting...")
            sys.exit(1)
        elif self.wafer_process is None:
            print("wafer_process is None")
            sys.exit(1)
        elif self.assembly_process is None:
            print("assembly_process is None")
            sys.exit(1)
        elif self.test_process is None:
            print("test_process is None")
            sys.exit(1)
        elif self.stackup is None:
            print("stackup is None")
            sys.exit(1)

        # Compute power for all chips stacked on top of the current chip.
        self.set_stack_power(self.compute_stack_power())

        # Compute the io power for the chip.
        self.set_io_power(self.get_signal_power(self.get_chip_list()))

        # Compute the total power used for the chip.
        if self.bb_power is None:
            self.set_total_power(self.get_power() + self.get_io_power() + self.get_stack_power())
        else:
            # bb_power overrides all power specific to the chip. So this is the io power and the self power, but stack power is part of other chip objects, so is still added.
            self.set_total_power(self.bb_power + self.get_stack_power())

        self.set_nre_design_cost()

        # print("Chip name is " + self.name + ".")
        self.set_area()

        self.set_self_true_yield(self.compute_layer_aware_yield())
        self.set_self_test_yield(self.get_test_process().compute_self_test_yield(self))
        if self.bb_quality is None:
            self.set_self_quality(self.get_test_process().compute_self_quality(self))
        else:
            self.set_self_quality(self.bb_quality)
        


        self.set_chip_true_yield(self.compute_chip_yield())
        self.set_chip_test_yield(self.get_test_process().compute_assembly_test_yield(self))
        self.set_quality(self.get_test_process().compute_assembly_quality(self))
        self.set_self_cost(self.compute_self_cost())
        self.set_cost(self.compute_cost())

#        self.set_chip_true_yield(self.compute_chip_yield())
#        self.set_quality(self.get_test_process().compute_assembly_quality(self))
#        self.set_chip_test_yield(self.get_test_process().compute_self_test_yield(self))
#        self.set_cost(self.compute_cost())

        # If the chip is defined as static, it should not be changed.
        self.static = static 

        return

    def compute_stack_power(self) -> float:
        stack_power = 0.0
        for chip in self.chips:
            stack_power += chip.get_total_power()
        return stack_power

#    def build_adjacency_matrices(self, adjacency_matrix_definitions, ios):
#        
#        # TODO: Implement a dynamic adjacency matrix that is oriented toward stacks rather than chiplets.
#                   The idea for this would be that instead of having chiplets connect with the adjacency matrix,
#                   the adjacency matrix would show the connection between stacks on the current chip.
#                   This would allow storage of the relevant parts of the adjacency matrix in the chip object
#                   instead of as a netlist object passed to every chip object in the assembly.
#
#        adjacencyMatrixList = {}
#        return adjacencyMatrixList

    # Find process function for use in searching through lists of processes.
    #       Note that there is a requirement that the processes support the
    #       get_name() function.
    def find_process(self, process_name, process_list):
        process = None
        for p in process_list:
            if p.get_name() == process_name:
                process = p
                break
        if process is None:
            print("Error: Process not found.")
        return process

    def find_wafer_process(self, wafer_process_name, wafer_process_list):
        wafer_process = self.find_process(wafer_process_name, wafer_process_list)
        if wafer_process is None:
            print("Error: Wafer Process " + wafer_process_name + " not found.")
            print("Exiting")
            sys.exit(1)
        return wafer_process

    def find_assembly_process(self, assembly_process_name, assembly_process_list):
        assembly_process = self.find_process(assembly_process_name, assembly_process_list)
        if assembly_process is None:
            print("Error: Assembly Process " + assembly_process_name + " not found.")
            print("Exiting")
            sys.exit(1)
        return assembly_process

    def find_test_process(self, test_process_name, test_process_list):
        test_process = self.find_process(test_process_name, test_process_list)
        if test_process is None:
            print("Error: Test Process " + test_process_name + " not found.")
            print("Exiting")
            sys.exit(1)
        return test_process

    def build_stackup(self, stackup_string, layers):
        stackup = []
        # Split the stackup string at the commas.
        stackup_string = stackup_string.split(",")
        stackup_names = []
        for layer in stackup_string:
            layer_specification = layer.split(":")
            if int(layer_specification[0]) >= 0:
                for i in range(int(layer_specification[0])):
                    stackup_names.append(layer_specification[1])
            else:
                print("Error: Number of layers " + layer_specification[0] + " not valid for " + layer_specification[1] + ".")
                sys.exit(1)
        n_layers = len(stackup_names)
        for layer in stackup_names:
            l = self.find_process(layer, layers)
            if l is not None:
                stackup.append(l)
            else:
                print("Error: Layer " + layer + " not found.")
                print("Exiting")
                sys.exit(1)
        if len(stackup) != n_layers:
            print("Error: Stackup number of layers does not match definition, make sure all selected layers are included in the layer definition.")
            sys.exit(1)
        return stackup

    # ===== End of Initialization Functions =====

    # ===== Get/Set Functions =====

    def get_name(self) -> str:
        return self.name

    def set_name(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.name = value
            return 0

    def get_parent_chip(self):
        return self.parent_chip

    def get_aspect_ratio(self):
        return self.aspect_ratio

    def set_aspect_ratio(self, value):
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.aspect_ratio = value
            return 0

    def get_x_location(self):
        return self.x_location
    
    def set_x_location(self, value):
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.x_location = value
            return 0
        
    def get_y_location(self):
        return self.y_location
    
    def set_y_location(self, value):
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.y_location = value
            return 0

    def get_core_area(self) -> float:
        return self.core_area
    
    def set_core_area(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.core_area = value
            return 0

    def get_fraction_memory(self) -> float:
        return self.fraction_memory
    
    def set_fraction_memory(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.fraction_memory = value
            return 0
        
    def get_fraction_logic(self) -> float:
        return self.fraction_logic
    
    def set_fraction_logic(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.fraction_logic = value
            return 0

    def get_fraction_analog(self) -> float:
        return self.fraction_analog
    
    def set_fraction_analog(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.fraction_analog = value
            return 0

    def get_reticle_share(self) -> float:
        return self.reticle_share

    def set_reticle_share(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.reticle_share = value
            return 0

    def get_buried(self) -> bool:
        return self.buried

    def set_buried(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.buried = value
            return 0

    def get_self_cost(self) -> float:
        return self.self_cost
    
    def set_self_cost(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.self_cost = value
            return 0

    def get_cost(self) -> float:
        return self.cost

    def set_cost(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.cost = value
            return 0

    def get_self_true_yield(self) -> float:
        return self.self_true_yield
    
    def set_self_true_yield(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.self_true_yield = value
            return 0

    def get_chip_true_yield(self) -> float:
        return self.chip_true_yield

    def set_chip_true_yield(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.chip_true_yield = value
            return 0

    def get_self_test_yield(self) -> float:
        return self.self_test_yield
    
    def set_self_test_yield(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.self_test_yield = value
            return 0

    def get_chip_test_yield(self) -> float:
        return self.chip_test_yield

    def set_chip_test_yield(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.chip_test_yield = value
            return 0
    
    def get_self_quality(self) -> float:
        return self.self_quality
    
    def set_self_quality(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.self_quality = value
            return 0

    def get_quality(self) -> float:
        return self.quality

    def set_quality(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.quality = value
            return 0
    
    def get_assembly_process(self):
        return self.assembly_process

    def set_assembly_process(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.assembly_process = value
            return 0
    
    def get_stackup(self) -> list:
        return self.stackup

    def set_stackup(self,layer_list) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.stackup = layer_list
            return 0

    def get_test_process(self):
        return self.test_process
    
    def set_test_process(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.test_process = value
            return 0
    
    def get_chips_len(self) -> int:
        return len(self.chips)

    def get_chips(self) -> list:
        return self.chips
    
    def set_chip_definitions(self,chip_definitions) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.chips = self.buildChips(chip_definitions)
            return 0
    
    def get_wafer_diameter(self) -> float:
        return self.wafer_process.get_wafer_diameter()
    
    def get_edge_exclusion(self) -> float:
        return self.wafer_process.get_edge_exclusion()
    
    def get_wafer_process_yield(self) -> float:
        return self.wafer_process.get_wafer_process_yield()
    
    def get_reticle_x(self) -> float:
        return self.wafer_process.get_reticle_x()
 
    def get_reticle_y(self) -> float:
        return self.wafer_process.get_reticle_y()
     
    def get_power(self) -> list:
        return self.power

    def set_power(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.power = value
            return 0
    
    def get_core_voltage(self) -> float:
        return self.core_voltage
    
    def set_core_voltage(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.core_voltage = value
            return 0

    def get_stack_power(self) -> float:
        return self.stack_power
    
    def set_stack_power(self, value) -> int:
        self.stack_power = self.compute_stack_power()
        return 0

    def get_io_power(self) -> float:
        return self.io_power

    def set_io_power(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.io_power = value
            return 0

    def get_total_power(self) -> float:
        return self.total_power

    def set_total_power(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.total_power = value
            return 0

    def get_area(self) -> float:
        return self.area
    
    def set_area(self) -> int:
        self.area = self.compute_area()
        return 0

    # TODO: Add rectangle packing now that the aspect ratio is defined.
    def get_stacked_die_area(self) -> float:
        stacked_die_area = 0.0
        for chip in self.chips:
            if not chip.get_buried():
                stacked_die_area += self.expandedArea(chip.get_area(),self.assembly_process.get_die_separation()/2,chip.get_aspect_ratio())
        # Note default aspect ratio is assumed here since this is not the final area for the chip object.
        stacked_die_area = self.expandedArea(stacked_die_area, self.assembly_process.get_edge_exclusion())
        return stacked_die_area

    def get_quantity(self) -> int:
        return self.quantity
    
    def set_quantity(self, value) -> int:
        if (self.static):
            print("Error: Cannot change static chip.")
            return 1
        else:
            self.quantity = value
            return 0

    def compute_nre_front_end_cost(self) -> float:
        front_end_cost = self.get_core_area()*(self.wafer_process.get_nre_front_end_cost_per_mm2_memory()*self.get_fraction_memory() +
                                               self.wafer_process.get_nre_front_end_cost_per_mm2_logic()*self.get_fraction_logic() +
                                               self.wafer_process.get_nre_front_end_cost_per_mm2_analog()*self.get_fraction_analog())
        return front_end_cost
    
    def compute_nre_back_end_cost(self) -> float:
        back_end_cost = self.get_core_area()*(self.wafer_process.get_nre_back_end_cost_per_mm2_memory()*self.get_fraction_memory() +
                                              self.wafer_process.get_nre_back_end_cost_per_mm2_logic()*self.get_fraction_logic() +
                                              self.wafer_process.get_nre_back_end_cost_per_mm2_analog()*self.get_fraction_analog())
        return back_end_cost

    def compute_nre_design_cost(self) -> float:
        nre_design_cost = self.compute_nre_front_end_cost() + self.compute_nre_back_end_cost()
        return nre_design_cost

    def get_nre_design_cost(self) -> float:
        return self.nre_design_cost

    def set_nre_design_cost(self) -> int:
        self.nre_design_cost = self.compute_nre_design_cost()
        return 0

    def get_static(self) -> bool:
        return self.static

    def set_static(self) -> int:
        self.static = True
        return 0

    # ===== End of Get/Set Functions =====

    # ===== Print Functions =====

    def print_description(self):
        print("Chip Name: " + self.name)
        print()
        print("Black-Box Parameters: area = " + str(self.bb_area) + ", cost = " + str(self.bb_cost) + ", quality = " +
              str(self.bb_quality) + ", power = " + str(self.bb_power) +
              ". (If any of these parameters are not empty, the value will override the computed value.)")

        print()
        print("Chip Wafer Process: " + self.wafer_process.get_name())
        print("Chip Assembly Process: " + self.assembly_process.get_name())
        print("Chip Test Process: " + self.test_process.get_name())
        print("Chip Stackup: " + str([(l.get_name() + ",") for l in self.stackup]))

        print()
        print("Chip Core Area: " + str(self.core_area))
        print("Chip Buried: " + str(self.get_buried()))
        print("Chip Core Voltage: " + str(self.get_core_voltage()))

        print()
        print("Chip Area Breakdown: memory = " + str(self.fraction_memory) + ", logic = " + str(self.fraction_logic) +
              ", analog = " + str(self.fraction_analog) + ".")
        if self.get_reticle_share() != 1.0:
            print("Chip takes up " + str(self.get_reticle_share()*100) + " \% of a shared reticle")
        else:
            print("Reticle is not shared.")
        print("NRE Cost: " + str(self.compute_nre_cost()))
        print("Quantity: " + str(self.get_quantity()))

        print()

        print("Chip Power: " + str(self.get_power()))
        print("Stack Power: " + str(self.get_stack_power()))
        print("Total Power: " + str(self.get_total_power()))

        print()
        print("Number of Chip Power Pads: " + str(self.get_power_pads()))
        print("Number of Signal Pads: " + str(self.get_signal_count(self.get_chip_list())[0]))
        print("Total number of pads: " + str(self.get_power_pads() + self.get_signal_count(self.get_chip_list())[0]))
        
        print()
        print("Area of IO Cells: " + str(self.get_io_area()))
        print("Area required by pads: " + str(self.get_pad_area()))
        print("Chip Calculated Area: " + str(self.area))

        print()
        print("Chip Self True Yield: " + str(self.get_self_true_yield()))
        print("Chip Self Test Yield: " + str(self.get_self_test_yield()))
        print("Chip Self Quality: " + str(self.get_self_quality()))
        print("Chip Self Cost: " + str(self.get_self_cost()))
        print("Chip True Yield: " + str(self.get_chip_true_yield()))
        print("Chip Tested Yield: " + str(self.get_chip_test_yield()))
        print("Chip Quality: " + str(self.get_quality()))
        print("Chip Cost: " + str(self.get_cost()))

        # print("Chip Cost: " + str(self.get_cost()))
        # print("Self Layer Cost: " + str(self.get_layer_aware_cost()))
        # print("Chip Cost Including Yield: " + str(self.get_cost()/self.get_chip_true_yield()))
        # print("Chip Quality: " + str(self.quality))
        # print("Chip Assembly Yield: " + str(self.assembly_process.assembly_yield(self.get_chips_len(),self.get_chips_signal_count(),self.get_stacked_die_area())))
        # print("Chip Assembly Process: " + self.assembly_process.get_name())
        # print("Chip Stackup: " + str([l.get_name() for l in self.stackup]))
        # print("Chip Test Process: " + self.test_process.get_name())
        # print("Chip Wafer Diameter: " + str(self.get_wafer_diameter()) + "mm")
        # print("Reticle Dimensions: (" + str(self.get_reticle_x()) + "," + str(self.get_reticle_y()) + ")mm")
        # print("Chip Adjacency Matrix List: " + str(self.adjacencyMatrixList))
        # print("Chip List: " + str([c.get_name() for c in self.chips]))
        #core_area_sum = 0.0
        #io_area_sum = 0.0
        #total_area = 0.0
        #for chip in self.chips:
        #    print("Cost = " + str(chip.get_cost()))
        #    print("Yield = " + str(chip.get_chip_true_yield()))
        for chip in self.chips:
           print(">>")
           chip.print_description()
        #   core_area_sum += chip.coreArea
        #   total_area += chip.area
        #   io_area_sum += chip.get_io_area()
        #   if chip.chips != []:
        #       for subchip in chip.chips:
        #            core_area_sum += subchip.coreArea
        #            total_area += subchip.area
        #            io_area_sum += chip.get_io_area()
           print("<<")
        #core_area_sum += self.core_area
        #io_area_sum += self.get_io_area()
        #total_area += self.area
        #print("core_area_sum = " + str(core_area_sum))
        #print("io_area_sum = " + str(io_area_sum))
        #print("total_area = " + str(total_area))
        return


    # ===== End of Print Functions =====

    # ===== Other Functions =====
 
    # Find the total area if a given area is assumed to be square and increased in size by a certain amount in every direction.
    def expandedArea(self,area,border,aspect_ratio=1.0):
        x = math.sqrt(area*aspect_ratio)
        y = math.sqrt(area/aspect_ratio)
        new_area = (x+2*border)*(y+2*border)
        return new_area

    # Get the area of the IOs on the chip.
    def get_io_area(self):
        # TODO: Ultimately, this needs to look at the adjacency matrices and count the number of connections. For now, filler.
        io_area = 0.0
        block_index = None
        for i in range(len(self.block_names)):
            if self.block_names[i] == self.get_name():
                block_index = i
                break
        if block_index is None:
            return 0
        for io_type in self.global_adjacency_matrix:
            # TODO: Fix this when the adjacency matrix is properly implemented.
            for io in self.io_list:
                if io.get_type() == io_type:
                    break
            # Add all the entries in the row and column of the global adjacency matrix with the index correesponding to the name of the chip and weight with the wire_count of the IO type.
            io_area += sum(self.global_adjacency_matrix[io_type][block_index][:]) * io.get_tx_area() + sum(self.global_adjacency_matrix[io_type][:][block_index]) * io.get_rx_area()

        return io_area

    # Get number of Power Pads
    def get_power_pads(self):
        power = self.get_total_power()
        power_pads = math.ceil(power / self.assembly_process.get_power_per_pad(self.get_core_voltage()))
        power_pads = power_pads*2 # Multiply by 2 for ground and power.
        return power_pads

    # Get the area taken up by the grid of pads on the bottom of the chip at the given pitch.
    def get_pad_area(self):
        num_power_pads = self.get_power_pads()
        signal_pads, signal_with_reach_count = self.get_signal_count(self.get_chip_list())
        num_pads = signal_pads + num_power_pads
        # print("num pads = " + str(num_pads))

        parent_chip = self.get_parent_chip()
        if parent_chip is not None:
            bonding_pitch = parent_chip.assembly_process.get_bonding_pitch()
        else:
            bonding_pitch = self.assembly_process.get_bonding_pitch()
        area_per_pad = bonding_pitch**2

        # Create a list of reaches by taking the keys from the signal_with_reach_count dictionary and converting to floats.
        reaches = [float(key) for key in signal_with_reach_count.keys()]
        # Sort the reaches from smallest to largest.
        reaches.sort()
        #current_side = 0.0
        current_x = 0.0
        current_y = 0.0
        current_count = 0
        for reach in reaches:
            # Note that half of the reach with separation value is the valid placement band.
            if parent_chip is not None:
                reach_with_separation = reach - parent_chip.assembly_process.get_die_separation()
            else:
                reach_with_separation = reach - self.assembly_process.get_die_separation()
            if reach_with_separation < 0:
                print("Error: Reach is smaller than chip separation. Exiting...")
                sys.exit(1)
            current_count += signal_with_reach_count[str(reach)]
            # Find the minimum boundary that would contain all the pads with the current reach.
            required_area = current_count*area_per_pad
            if reach_with_separation < current_x and reach_with_separation < current_y: 
                #usable_area = 2*reach_with_separation*current_side - reach_with_separation**2
                # x*(reach_with_separation/2) is the placement band along a single edge.
                usable_area = reach_with_separation*(current_x+current_y) - reach_with_separation**2
            else:
                #usable_area = current_side**2
                usable_area = current_x*current_y
            if usable_area <= required_area:
                # Note that required_x and required_y are minimum. The real values are likely larger.
                required_x = math.sqrt(required_area*self.get_aspect_ratio())
                required_y = math.sqrt(required_area/self.get_aspect_ratio())
                if required_x > reach_with_separation and required_y > reach_with_separation:
                    # Work for computing the formulas below:
                    # required_area = 2*(new_req_x - (reach_with_separation/2)) * (reach_with_separation/2) + 2*(new_req_y - (reach_with_separation/2)) * (reach_with_separation/2)
                    # required_area = (2*new_req_x - reach_with_separation) * (reach_with_separation/2) + (2*new_req_y - reach_with_separation) * (reach_with_separation/2)
                    # required_area = (2*new_req_x + 2*new_req_y - 2*reach_with_separation) * (reach_with_separation/2)
                    # new_req_x = aspect_ratio*new_req_y
                    # 2*aspect_ratio*new_req_y + 2*new_req_y = (2*required_area/reach_with_separation) + 2*reach_with_separation
                    # new_req_y*(2*aspect_ratio + 2) = (2*required_area/reach_with_separation) + 2*reach_with_separation
                    # new_req_y = ((2*required_area/reach_with_separation) + 2*reach_with_separation)/(2*aspect_ratio + 2)
                    new_req_y = ((2*required_area/reach_with_separation) + 2*reach_with_separation)/(2*self.get_aspect_ratio() + 2)
                    new_req_x = self.get_aspect_ratio()*new_req_y
                else:
                    new_req_x = required_x
                    new_req_y = required_y
                # Round up to the nearest multiple of bonding pitch.
                new_req_x = math.ceil(new_req_x/bonding_pitch)*bonding_pitch
                new_req_y = math.ceil(new_req_y/bonding_pitch)*bonding_pitch
                if new_req_x > current_x:
                    current_x = new_req_x
                if new_req_y > current_y:
                    current_y = new_req_y

        # TODO: This is not strictly accurate. The aspect ratio requirement may break down when the chip becomes pad limited.
        #       Consider updating this if the connected placement tool does not account for pad area.
        required_area = area_per_pad * num_pads #current_x * current_y #current_side**2
        if required_area <= current_x*current_y:
            grid_x = math.ceil(current_x / bonding_pitch)
            grid_y = math.ceil(current_y / bonding_pitch)
        else:
            # Expand shorter side until sides are the same length, then expand both.
            if current_x < current_y:
                # Y is larger
                if current_y**2 <= required_area:
                    grid_y = math.ceil(current_y / bonding_pitch)
                    grid_x = math.ceil((required_area/current_y) / bonding_pitch)
                else:
                    required_side = math.sqrt(required_area)
                    grid_x = math.ceil(required_side / bonding_pitch)
                    grid_y = grid_x
            elif current_y < current_x:
                # X is larger
                if current_x**2 <= required_area:
                    grid_x = math.ceil(current_x / bonding_pitch)
                    grid_y = math.ceil((required_area/current_x) / bonding_pitch)
                else:
                    required_side = math.sqrt(required_area)
                    grid_x = math.ceil(required_side / bonding_pitch)
                    grid_y = grid_x
            else:
                # Both are the same size
                required_side = math.sqrt(required_area)
                grid_x = math.ceil(required_side / bonding_pitch)
                grid_y = grid_x

        pad_area = grid_x * grid_y * area_per_pad
        # print("Pad area is " + str(pad_area) + ".")

        return pad_area

    # Get the area of the interposer based on areas of the consituent chiplets.
    # Note that this is an approximation that assumes square chiplets that pack perfectly so it is an optimistic solution that actually gives a lower bound on area.
    # TODO: Implement proper packing and aspect ratio shaping so this is a legitimate solution instead of a strange L shaped interposer for example.
    def compute_area(self):
        if self.bb_area is not None:
            return self.bb_area

        # print("Computing area for chip " + self.get_name() + "...")
        chip_io_area = self.get_core_area() + self.get_io_area()

        pad_required_area = self.get_pad_area()

        stacked_die_bound_area = self.get_stacked_die_area()
        #for chip in self.get_chips():
        #    chip_contribution = self.expandedArea(chip.get_core_area() + chip.get_io_area(), self.assembly_process.get_die_separation()/2)
        #    # print("\tAdding " + str(chip_contribution) + " from chip " + chip.get_name() + " to stacked_die_bound_area.")
        #    stacked_die_bound_area += chip_contribution
        ## print("\tStacked die bound area is " + str(stacked_die_bound_area) + ".")

        # print("Selecting the maximum from (stacked_die_bound_area,pad_required_area,chip_io_area): " + str(stacked_die_bound_area) + ", " + str(pad_required_area) + ", and " + str(chip_io_area) + ".")
        # chip_area is the maximum value of the chip_io_area, stacked_die_bound_area, and pad_required_area.
        chip_area = max(stacked_die_bound_area, pad_required_area, chip_io_area)

        return chip_area
 
    def compute_number_reticles(self, area) -> int:
        # TODO: Ground this by actually packing rectangles to calculate a more accurate number of reticles.
        reticle_area = self.get_reticle_x()*self.get_reticle_y()
        num_reticles = math.ceil(area/reticle_area)
        largest_square_side = math.floor(math.sqrt(num_reticles))
        largest_square_num_reticles = largest_square_side**2
        num_stitches = largest_square_side*(largest_square_side-1)*2+2*(num_reticles-largest_square_num_reticles)-math.ceil((num_reticles-largest_square_num_reticles)/largest_square_side)
        return num_reticles, num_stitches
        
    def compute_layer_aware_yield(self) -> float:
        layer_yield = 1.0
        for layer in self.stackup:
            layer_yield *= layer.layer_yield(self.get_core_area() + self.get_io_area())

        return layer_yield

    # Get probability that all component tested chips are good.
    def quality_yield(self) -> float:
        quality_yield = 1.0
        for chip in self.chips:
            quality_yield *= chip.get_quality()
        return quality_yield

    def get_signal_count(self,internal_block_list):
        # print("Getting signal count")
        signal_count = 0
        # This is a dictionary where the key is the reach and the value is the number of signals with that reach.
        signal_with_reach_count = {}

        block_index = None
        internal_block_list_indices = []
        for i in range(len(self.block_names)):
            if self.block_names[i] == self.get_name():
                block_index = i
            if self.block_names[i] in internal_block_list:
                internal_block_list_indices.append(i)
        if block_index is None:
            #print("Warning: Chip " + self.get_name() + " not found in block list netlist: " + str(self.block_names) + ". This can be ignored if the chip is a pass-through chip.")
            return 0, {}
        for io_type in self.global_adjacency_matrix:
            # TODO: Fix this when the adjacency matrix is properly implemented.
            for io in self.io_list:
                if io.get_type() == io_type:
                    break
            if io.get_bidirectional():
                bidirectional_factor = 0.5
            else:
                bidirectional_factor = 1.0
            # Add all the entries in the row and column of the global adjacency matrix with the index correesponding to the name of the chip and weight with the wire_count of the IO type.
            for j in range(len(self.global_adjacency_matrix[io_type][block_index][:])):
                # print("Internal block list indices = " + str(internal_block_list_indices) + ".")
                # print("Adjacency matrix:" + str(self.global_adjacency_matrix[io_type][:][:]))
                if j not in internal_block_list_indices:
                    # print("Adding to signal count for " + self.block_names[j] + ".")
                    # print("io signal width = " + str(io.get_wire_count()) + ".")
                    # print("Signal count before = " + str(signal_count) + ".")
                    signal_count += (self.global_adjacency_matrix[io_type][block_index][j] + self.global_adjacency_matrix[io_type][j][block_index]) * io.get_wire_count() * bidirectional_factor
                    # print("Signal count after = " + str(signal_count) + ".")
                    if str(io.get_reach()) in signal_with_reach_count:
                        signal_with_reach_count[str(io.get_reach())] += (self.global_adjacency_matrix[io_type][block_index][j] + self.global_adjacency_matrix[io_type][j][block_index]) * io.get_wire_count() * bidirectional_factor
                    else:
                        signal_with_reach_count[str(io.get_reach())] = (self.global_adjacency_matrix[io_type][block_index][j] + self.global_adjacency_matrix[io_type][j][block_index]) * io.get_wire_count() * bidirectional_factor
            #signal_count += (sum(self.global_adjacency_matrix[io_type][block_index][:]) + sum(self.global_adjacency_matrix[io_type][:][block_index])) * io.get_wire_count()
        
        # print("Signal count = " + str(signal_count) + ".")
        # print("Signal with reach count = " + str(signal_with_reach_count) + ".")

        # print()

        return signal_count, signal_with_reach_count

    def get_signal_power(self,internal_block_list) -> float:
        signal_power = 0.0
        block_index = None
        internal_block_list_indices = []
        for i in range(len(self.block_names)):
            if self.block_names[i] == self.get_name():
                block_index = i
            if self.block_names[i] in internal_block_list:
                internal_block_list_indices.append(i)
        # if block_index is None: #This is a chip with only pass-through connections such as an interposer.
            # return 0
        for io_type in self.global_adjacency_matrix:
            # TODO: Fix this when the local adjacency matrix is properly implemented.
            for io in self.io_list:
                if io.get_type() == io_type:
                    break
            if io.get_bidirectional():
                bidirectional_factor = 0.5
            else:
                bidirectional_factor = 1.0
            link_weighted_sum = 0.0

            for index in internal_block_list_indices:
                # Sum of row and columns of the global adjacency matrix weighted element-wise by the average bandwidth utilization.
                value = (sum(self.global_adjacency_matrix[io_type][:][index]*self.average_bandwidth_utilization[io_type][:][index]) + sum(self.global_adjacency_matrix[io_type][index][:]*self.average_bandwidth_utilization[io_type][index][:]))
                link_weighted_sum += value
            signal_power += link_weighted_sum * io.get_bandwidth() * io.get_energy_per_bit() * bidirectional_factor

        return signal_power

    def get_chip_list(self):
        chip_list = []
        for chip in self.chips:
            chip_list.append(chip.get_chip_list())
        chip_list.append(self.get_name())
        return chip_list

    def get_chips_signal_count(self) -> int:
        signal_count = 0

        internal_chip_list = self.get_chip_list()

        for chip in self.chips:
            signal_count += chip.get_signal_count(internal_chip_list)[0]
        return signal_count

    # This computes the true yield of a chip assembly.
    def compute_chip_yield(self) -> float:
        # Account for the quality of the chip after self-test.
        chip_true_yield = self.get_self_quality()
        # Account for quality of component chiplets.
        quality_yield = self.quality_yield()
        # Account for assembly yield.
        assembly_yield = self.assembly_process.assembly_yield(self.get_chips_len(),self.get_chips_signal_count(),self.get_stacked_die_area())
        # Multiply the yields.
        chip_true_yield *= quality_yield*assembly_yield*self.get_wafer_process_yield()
        return chip_true_yield

    def wafer_area_eff(self) -> float:
        # TODO: Need to add a function or closed form solution to find the Gauss' circle problem for the number of chips that can fit on a wafer.
        usable_wafer_radius = (self.get_wafer_diameter()/2)-self.get_edge_exclusion()
        usable_wafer_area = math.pi*(usable_wafer_radius)**2
        return usable_wafer_area

    def get_layer_aware_cost(self):
        cost = 0
        for layer in self.stackup:
            cost += layer.layer_cost(self.get_area(), self.wafer_process)
        return cost

    def get_mask_cost(self):
        cost = 0
        for layer in self.stackup:
            cost += layer.get_mask_cost()
        cost *= self.get_reticle_share()
        return cost

    def compute_nre_cost(self) -> float:

        nre_cost = (self.get_nre_design_cost() + self.get_mask_cost())/self.get_quantity()

        return nre_cost

    def compute_self_cost(self) -> float:
        cost = 0.0

        # The bb_cost parameter will override the self cost computation.
        if self.bb_cost is not None:
            cost = self.bb_cost
        else:
            # Add cost of this chip
            self_layer_cost = self.get_layer_aware_cost()
            cost += self_layer_cost
            # NRE Cost
            nre_cost = self.compute_nre_cost()
            cost += nre_cost
            # Add test cost
            self_test_cost = self.get_test_process().compute_self_test_cost(self)
            cost += self_test_cost

            cost = cost/self.get_self_test_yield()

        return cost

    def compute_cost(self) -> float:
        cost = self.get_self_cost()

        stack_cost = 0.0
        # Add cost of stacked chips
        for i in range(len(self.chips)):
            stack_cost += self.chips[i].get_cost()
        cost += stack_cost

        # Add assembly cost 
        assembly_cost = self.assembly_process.assembly_cost(self.get_chips_len(),self.get_stacked_die_area())
        cost += assembly_cost

        # Add assembly test cost
        assembly_test_cost = self.get_test_process().compute_assembly_test_cost(self)
        cost += assembly_test_cost

        cost = cost/self.get_chip_test_yield()

        return cost

    # ===== End of Other Functions =====








# Interconnect Adjacency Matrix Class
# The class has the following attributes (These should be treated as private.):
#   type: The type of IO for this adjacency matrix. (Select from list of IO definitions.)
#   block_names: The names of the blocks in the adjacency matrix.
#   adjacency_matrix: The adjacency matrix itself.
#   static: A boolean value indicating whether the adjacency matrix is static or not. (This is meant to act like a latch.)
# The class has the following methods:
#   get_type(): Returns the type of the adjacency matrix.
#   set_type(): Sets the type of the adjacency matrix.
#   get_block_names_length(): Returns the length of the block names list.
#   get_block_names_entry(index): Returns the block name at the given index.
#   set_block_names_entry(index, value): Sets the block name at the given index to the given value.
#   get_block_names(): Returns the block names of the adjacency matrix.
#   set_block_names(): Sets the block names of the adjacency matrix.
#   get_adjacency_matrix_shape(): Returns the shape of the adjacency matrix.
#   get_adjacency_matrix_entry(row, column): Returns the entry at the given row and column.
#   set_adjacency_matrix_entry(row, column, value): Sets the entry at the given row and column to the given value.
#   get_adjacency_matrix(): Returns the adjacency matrix.
#   set_adjacency_matrix(): Sets the adjacency matrix.
#   get_static(): Returns the static value of the adjacency matrix.
#   set_static(): Sets the static value of the adjacency matrix.

#class InterconnectAdjacencyMatrix:
#    def __init__(self, type=None, IO_list=None, block_names=None, adjacency_matrix=[], static=False) -> None:
#        self.type = type
#        self.block_names = block_names
#        self.adjacency_matrix = adjacency_matrix
#        self.static = static
#        if self.type is None:
#            print("Error: Type not defined for Interconnect Adjacency Matrix. Exiting...")
#            sys.exit(1) 
#        # If the adjacency matrix is empty, exit.
#        if self.adjacency_matrix == []:
#            print("Error: Adjacency matrix not defined for Interconnect Adjacency Matrix. Exiting...")
#            sys.exit(1)
#        # If the adjacency matrix is not square, exit.
#        if len(self.adjacency_matrix) != len(self.adjacency_matrix[0]) or len(self.adjacency_matrix.shape) != 2:
#            print("Error: Adjacency matrix is not square. Exiting...")
#            sys.exit(1)
#        if self.block_names is None:
#            print("Warning: Block names not defined for Interconnect Adjacency Matrix. Generating default block names.")
#            block_names = []
#            for i in range(len(self.adjacency_matrix)):
#                block_names.append("Block " + str(i))
#        # If block_names is not the same length as the side of adjacency matrix exit.
#        if len(self.block_names) != len(self.adjacency_matrix):
#            print("Error: Block names is not the same length as the side of the adjacency matrix. Exiting...")
#            sys.exit(1)
#        if IO_list is None:
#            print("Error: IO list not defined for Interconnect Adjacency Matrix. Exiting...")
#            sys.exit(1)
#        self.IO = None
#        for IO in IO_list:
#            if IO.get_type() == type:
#                self.IO = IO
#                break
#        if self.IO is None:
#            print("Error: No IO of type " + type + " found.")
#            print("Exiting...")
#            sys.exit(1)
#        return
#
#    # ===== Get/Set Functions =====
#
#    def get_type(self) -> str:
#        return self.type
#
#    def get_block_names_len(self) -> int:
#        return len(self.block_names)
#
#    def get_block_names_entry(self, index) -> str:
#        return self.block_names[index]
#
#    def set_block_names_entry(self, index, value) -> int:
#        if (self.static):
#            print("Error: Cannot change static IO.")
#            return 1
#        else:
#            self.block_names[index] = value
#            return 0    
#
#    def get_block_names(self) -> list:
#        return self.block_names
#
#    def set_block_names(self, value) -> int:
#        if (self.static):
#            print("Error: Cannot change static IO.")
#            return 1
#        else:
#            self.block_names = value
#            return 0
#
#    def get_adjacency_matrix_shape(self) -> tuple:
#        return self.adjacency_matrix.shape
#
#    def get_adjacency_matrix_entry(self, TX, RX) -> int:
#        return self.adjacency_matrix[TX][RX]
#    
#    def set_adjacency_matrix_entry(self, TX, RX, value) -> int:
#        if (self.static):
#            print("Error: Cannot change static IO.")
#            return 1
#        else:
#            self.adjacency_matrix[TX][RX] = value
#            return 0
#
#    def get_adjacency_matrix(self) -> np.ndarray:
#        return self.adjacency_matrix
#
#    def set_adjacency_matrix(self, value) -> int:
#        if (self.static):
#            print("Error: Cannot change static IO.")
#            return 1
#        else:
#            self.adjacency_matrix = value
#            return 0
#
#    def get_static(self) -> bool:
#        return self.static
#    
#    def set_static(self) -> int:
#        self.static = True
#        return 0
#
#    # ===== End of Get/Set Functions =====
#
#    # ===== Print Functions =====
#
#    def print_description(self) -> None:
#        print("Type: " + self.type)
#        print("Static: " + str(self.static))
#
#        print("\t\t",end="")   
#        for i in range(len(self.block_names)):
#            print(self.block_names[i],end="\t")
#        print()
#        for i in range(len(self.block_names)):
#            print(self.block_names[i],end="\t")
#            for j in range(len(self.block_names)):
#                print(self.adjacency_matrix[i][j],end="\t")
#        return
#
#    # ===== End of Print Functions =====
#
#    # ===== Other Functions =====
#
#    def combine_blocks(self, block_1, block_2) -> int:
#        if (self.static):
#            print("Error: Cannot change static IO.")
#            return 1
#        else:
#            if block_1 == block_2:
#                print("Error: Cannot combine a block with itself.")
#                return 1
#            self.block_names[block_1] = self.block_names[block_1] + "," + self.block_names[block_2]
#            self.block_names.pop(block_2)
#            shape_am = self.adjacency_matrix.shape
#            for i in range(shape_am[0]):
#                if i != block_1 and i != block_2:
#                    self.adjacency_matrix[i][block_1] = self.adjacency_matrix[i][block_1] + self.adjacency_matrix[i][block_2]
#                    self.adjacency_matrix[i][block_2] = 0
#            for i in range(shape_am[1]):
#                if i != block_1 and i != block_2:
#                    self.adjacency_matrix[block_1][i] = self.adjacency_matrix[block_1][i] + self.adjacency_matrix[block_2][i]
#                    self.adjacency_matrix[block_2][i] = 0
#            self.adjacency_matrix = np.delete(self.adjacency_matrix, block_2, 0)
#            self.adjacency_matrix = np.delete(self.adjacency_matrix, block_2, 1)
#            return 0  
#
#    # ===== End of Other Functions =====
