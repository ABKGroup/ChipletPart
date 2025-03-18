#include <iostream>
#include <cassert>
#include <cmath>
#include "design/Layer.hpp"
#include "design/WaferProcess.hpp"

using namespace design;

// Helper function to check if a value is close to an expected value and report sign if it's not
bool check_close(double actual, double expected, double epsilon = 0.0001) {
    double diff = actual - expected;
    if (std::abs(diff) >= epsilon) {
        if (diff < 0) {
            std::cout << "FAIL: " << actual << " is less than expected " << expected << " by " << -diff << std::endl;
        } else {
            std::cout << "FAIL: " << actual << " is greater than expected " << expected << " by " << diff << std::endl;
        }
        return false;
    }
    return true;
}

int main() {
    // Create a Layer with the same parameters as in the Python test
    Layer lp("test_layer_process", "True", 0.1234, 0.0321, 0.00543, 0.5, 2, 0.3, 100000, 0.98, true);
    
    // Test basic properties
    assert(lp.LayerFullyDefined() == true);
    assert(lp.GetName() == "test_layer_process");
    assert(lp.GetActive() == true);
    assert(check_close(lp.GetCostPerMm2(), 0.1234));
    assert(check_close(lp.GetTransistorDensity(), 0.0321));
    assert(check_close(lp.GetDefectDensity(), 0.00543));
    assert(check_close(lp.GetCriticalAreaRatio(), 0.5));
    assert(check_close(lp.GetClusteringFactor(), 2));
    assert(check_close(lp.GetLithoPercent(), 0.3));
    assert(check_close(lp.GetMaskCost(), 100000));
    assert(check_close(lp.GetStitchingYield(), 0.98));
    
    // Create another Layer with active set to false
    Layer lp2("test_layer_process", "False", 0.1234, 0.0321, 0.00543, 0.5, 2, 0.3, 100000, 0.98, true);
    
    // Check that active is false
    assert(lp2.GetActive() == false);
    
    // Function Checks
    
    // ReticleUtilization tests
    assert(check_close(lp.ReticleUtilization(1.0, 10.0, 10.0), 1.0));
    assert(check_close(lp.ReticleUtilization(2.0, 10.0, 10.0), 1.0));
    assert(check_close(lp.ReticleUtilization(1.0, 30.0, 10.0), 1.0));
    assert(check_close(lp.ReticleUtilization(1.0, 10.0, 20.0), 1.0));
    assert(check_close(lp.ReticleUtilization(3.0, 10.0, 10.0), 0.99));
    assert(check_close(lp.ReticleUtilization(45.0, 10.0, 10.0), 0.9));
    assert(check_close(lp.ReticleUtilization(51.0, 10.0, 10.0), 0.51));
    
    // ComputeDiesPerWafer tests
    assert(lp.ComputeDiesPerWafer(10, 10, 300, 1, true) == 540);
    assert(lp.ComputeDiesPerWafer(1, 1, 300, 1, true) == 17470);
    assert(lp.ComputeDiesPerWafer(100, 100, 300, 1, true) == 4);
    assert(lp.ComputeDiesPerWafer(10, 10, 100, 1, true) == 52);
    assert(lp.ComputeDiesPerWafer(10, 10, 300, 0.1, true) == 639);
    assert(lp.ComputeDiesPerWafer(10, 10, 300, 2, true) == 452);
    assert(lp.ComputeDiesPerWafer(10, 10, 300, 1, false) == 550);
    assert(lp.ComputeDiesPerWafer(1, 1, 300, 1, false) == 17590);
    assert(lp.ComputeDiesPerWafer(100, 100, 300, 1, false) == 4);
    assert(lp.ComputeDiesPerWafer(10, 10, 100, 1, false) == 52);
    assert(lp.ComputeDiesPerWafer(10, 10, 300, 0.1, false) == 651);
    assert(lp.ComputeDiesPerWafer(10, 10, 300, 2, false) == 462);
    
    // LayerYield tests
    assert(check_close(lp.LayerYield(1.0), 0.9972905184292643));
    assert(check_close(lp.LayerYield(10.0), 0.9733930025109545));
    assert(check_close(lp.LayerYield(100.0), 0.7752370299332099));
    assert(check_close(lp.LayerYield(1000.0), 0.17992710703076417));
    
    // WaferProcess-dependent tests
    WaferProcess wp("test_wafer_process", 234, 1.2, 0.98, 0.87, 32, 23, "False", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6);
    
    // LayerCost tests
    assert(check_close(lp.LayerCost(1.0, 1.0, wp), 0.4431977042289835));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp), 0.4563853896145381));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp), 16.18236160385767));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp), 170.36347017097916));
    
    // ComputeCostPerMm2 tests
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp), 0.20967401463602725));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp), 0.15804071921253907));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp), 0.2136412765876751));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp), 0.1597967272037895));
    
    // Test with a different WaferProcess
    WaferProcess wp2("test_wafer_process", 123, 2.4, 0.49, 0.45, 16, 25, "False", 0.2, 0.3, 0.4, 0.5, 0.6, 0.7);
    
    // LayerCost tests with wp2
    assert(check_close(lp.LayerCost(1.0, 1.0, wp2), 0.28499017291594464));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp2), 0.2920283687816242));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp2), 17.049702786657388));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp2), 183.28430495656693));
    
    // ComputeCostPerMm2 tests with wp2
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp2), 0.1828272368644059));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp2), 0.170299005766845));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp2), 0.18420533161464012));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp2), 0.16556847782887707));
    
    // Additional LayerYield tests with different Layer configurations
    
    // Layer with active=False
    lp = Layer("test_layer_process", "False", 0.1234, 0.0321, 0.00543, 0.5, 2, 0.3, 100000, 0.98, true);
    assert(check_close(lp.LayerYield(1.0), 0.9972905184292643));
    assert(check_close(lp.LayerYield(10.0), 0.9733930025109545));
    assert(check_close(lp.LayerYield(100.0), 0.7752370299332099));
    assert(check_close(lp.LayerYield(1000.0), 0.17992710703076417));
    
    // Test LayerCost and ComputeCostPerMm2 with wp and wp2
    assert(check_close(lp.LayerCost(1.0, 1.0, wp), 0.4431977042289835));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp), 0.4563853896145381));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp), 16.18236160385767));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp), 170.36347017097916));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp), 0.20967401463602725));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp), 0.15804071921253907));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp), 0.2136412765876751));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp), 0.1597967272037895));
    
    assert(check_close(lp.LayerCost(1.0, 1.0, wp2), 0.28499017291594464));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp2), 0.2920283687816242));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp2), 17.049702786657388));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp2), 183.28430495656693));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp2), 0.1828272368644059));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp2), 0.170299005766845));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp2), 0.18420533161464012));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp2), 0.16556847782887707));
    
    // Layer with cost_per_mm2=0.2468
    lp = Layer("test_layer_process", "True", 0.2468, 0.0321, 0.00543, 0.5, 2, 0.3, 100000, 0.98, true);
    assert(check_close(lp.LayerYield(1.0), 0.9972905184292643));
    assert(check_close(lp.LayerYield(10.0), 0.9733930025109545));
    assert(check_close(lp.LayerYield(100.0), 0.7752370299332099));
    assert(check_close(lp.LayerYield(1000.0), 0.17992710703076417));
    
    // Test LayerCost and ComputeCostPerMm2 with wp and wp2
    assert(check_close(lp.LayerCost(1.0, 1.0, wp), 0.886395408457967));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp), 0.9127707792290762));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp), 32.36472320771534));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp), 340.7269403419583));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp), 0.4193480292720545));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp), 0.31608143842507813));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp), 0.4272825531753502));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp), 0.319593454407579));
    
    assert(check_close(lp.LayerCost(1.0, 1.0, wp2), 0.5699803458318893));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp2), 0.5840567375632484));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp2), 34.099405573314776));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp2), 366.56860991313386));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp2), 0.3656544737288118));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp2), 0.34059801153369));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp2), 0.36841066322928023));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp2), 0.33113695565775414));
    
    // Layer with transistor_density=0.0642
    lp = Layer("test_layer_process", "True", 0.1234, 0.0642, 0.00543, 0.5, 2, 0.3, 100000, 0.98, true);
    assert(check_close(lp.LayerYield(1.0), 0.9972905184292643));
    assert(check_close(lp.LayerYield(10.0), 0.9733930025109545));
    assert(check_close(lp.LayerYield(100.0), 0.7752370299332099));
    assert(check_close(lp.LayerYield(1000.0), 0.17992710703076417));
    
    // Test LayerCost and ComputeCostPerMm2 with wp and wp2
    assert(check_close(lp.LayerCost(1.0, 1.0, wp), 0.4431977042289835));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp), 0.4563853896145381));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp), 16.18236160385767));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp), 170.36347017097916));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp), 0.20967401463602725));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp), 0.15804071921253907));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp), 0.2136412765876751));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp), 0.1597967272037895));
    
    assert(check_close(lp.LayerCost(1.0, 1.0, wp2), 0.28499017291594464));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp2), 0.2920283687816242));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp2), 17.049702786657388));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp2), 183.28430495656693));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp2), 0.1828272368644059));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp2), 0.170299005766845));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp2), 0.18420533161464012));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp2), 0.16556847782887707));
    
    // Layer with defect_density=0.01086
    lp = Layer("test_layer_process", "True", 0.1234, 0.0321, 0.01086, 0.5, 2, 0.3, 100000, 0.98, true);
    assert(check_close(lp.LayerYield(1.0), 0.994592033894289));
    assert(check_close(lp.LayerYield(10.0), 0.9478339469519471));
    assert(check_close(lp.LayerYield(100.0), 0.6185392607744513));
    assert(check_close(lp.LayerYield(1000.0), 0.07245733621472007));
    
    // Test LayerCost and ComputeCostPerMm2 with wp and wp2
    assert(check_close(lp.LayerCost(1.0, 1.0, wp), 0.4431977042289835));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp), 0.4563853896145381));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp), 16.18236160385767));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp), 170.36347017097916));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp), 0.20967401463602725));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp), 0.15804071921253907));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp), 0.2136412765876751));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp), 0.1597967272037895));
    
    assert(check_close(lp.LayerCost(1.0, 1.0, wp2), 0.28499017291594464));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp2), 0.2920283687816242));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp2), 17.049702786657388));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp2), 183.28430495656693));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp2), 0.1828272368644059));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp2), 0.170299005766845));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp2), 0.18420533161464012));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp2), 0.16556847782887707));
    
    // Layer with critical_area_ratio=1.0
    lp = Layer("test_layer_process", "True", 0.1234, 0.0321, 0.00543, 1.0, 2, 0.3, 100000, 0.98, true);
    assert(check_close(lp.LayerYield(1.0), 0.994592033894289));
    assert(check_close(lp.LayerYield(10.0), 0.9478339469519471));
    assert(check_close(lp.LayerYield(100.0), 0.6185392607744513));
    assert(check_close(lp.LayerYield(1000.0), 0.07245733621472007));
    
    // Test LayerCost and ComputeCostPerMm2 with wp and wp2
    assert(check_close(lp.LayerCost(1.0, 1.0, wp), 0.4431977042289835));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp), 0.4563853896145381));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp), 16.18236160385767));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp), 170.36347017097916));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp), 0.20967401463602725));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp), 0.15804071921253907));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp), 0.2136412765876751));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp), 0.1597967272037895));
    
    assert(check_close(lp.LayerCost(1.0, 1.0, wp2), 0.28499017291594464));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp2), 0.2920283687816242));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp2), 17.049702786657388));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp2), 183.28430495656693));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp2), 0.1828272368644059));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp2), 0.170299005766845));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp2), 0.18420533161464012));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp2), 0.16556847782887707));
    
    // Layer with litho_percent=0.6
    lp = Layer("test_layer_process", "True", 0.1234, 0.0321, 0.00543, 0.5, 2, 0.6, 100000, 0.98, true);
    assert(check_close(lp.LayerYield(1.0), 0.9972905184292643));
    assert(check_close(lp.LayerYield(10.0), 0.9733930025109545));
    assert(check_close(lp.LayerYield(100.0), 0.7752370299332099));
    assert(check_close(lp.LayerYield(1000.0), 0.17992710703076417));
    
    // Test LayerCost and ComputeCostPerMm2 with wp
    assert(check_close(lp.LayerCost(1.0, 1.0, wp), 0.4431977042289836));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp), 0.4563853896145381));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp), 16.428238791986065));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp), 204.65388109996218));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp), 0.20967401463602725));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp), 0.15804071921253907));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp), 0.2136412765876751));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp), 0.1597967272037895));
    
    // Test LayerCost and ComputeCostPerMm2 with wp2
    assert(check_close(lp.LayerCost(1.0, 1.0, wp2), 0.2849901729159447));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp2), 0.29202836878162425));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp2), 17.049702786657388));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp2), 183.28430495656693));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp2), 0.1828272368644059));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp2), 0.170299005766845));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp2), 0.18420533161464012));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp2), 0.16556847782887707));
    
    // Layer with mask_cost=200000
    lp = Layer("test_layer_process", "True", 0.1234, 0.0321, 0.00543, 0.5, 2, 0.3, 200000, 0.98, true);
    assert(check_close(lp.LayerYield(1.0), 0.9972905184292643));
    assert(check_close(lp.LayerYield(10.0), 0.9733930025109545));
    assert(check_close(lp.LayerYield(100.0), 0.7752370299332099));
    assert(check_close(lp.LayerYield(1000.0), 0.17992710703076417));
    
    // Test LayerCost and ComputeCostPerMm2 with wp and wp2
    assert(check_close(lp.LayerCost(1.0, 1.0, wp), 0.4431977042289835));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp), 0.4563853896145381));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp), 16.18236160385767));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp), 170.36347017097916));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp), 0.20967401463602725));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp), 0.15804071921253907));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp), 0.2136412765876751));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp), 0.1597967272037895));
    
    assert(check_close(lp.LayerCost(1.0, 1.0, wp2), 0.28499017291594464));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp2), 0.2920283687816242));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp2), 17.049702786657388));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp2), 183.28430495656693));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp2), 0.1828272368644059));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp2), 0.170299005766845));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp2), 0.18420533161464012));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp2), 0.16556847782887707));
    
    // Layer with stitching_yield=0.49
    lp = Layer("test_layer_process", "True", 0.1234, 0.0321, 0.00543, 0.5, 2, 0.3, 100000, 0.49, true);
    assert(check_close(lp.LayerYield(1.0), 0.9972905184292643));
    assert(check_close(lp.LayerYield(10.0), 0.9733930025109545));
    assert(check_close(lp.LayerYield(100.0), 0.7752370299332099));
    assert(check_close(lp.LayerYield(1000.0), 0.17992710703076417));
    
    // Test LayerCost and ComputeCostPerMm2 with wp and wp2
    assert(check_close(lp.LayerCost(1.0, 1.0, wp), 0.4431977042289835));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp), 0.4563853896145381));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp), 16.18236160385767));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp), 170.36347017097916));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp), 0.20967401463602725));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp), 0.15804071921253907));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp), 0.2136412765876751));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp), 0.1597967272037895));
    
    assert(check_close(lp.LayerCost(1.0, 1.0, wp2), 0.28499017291594464));
    assert(check_close(lp.LayerCost(1.0, 2.0, wp2), 0.2920283687816242));
    assert(check_close(lp.LayerCost(100.0, 1.0, wp2), 17.049702786657388));
    assert(check_close(lp.LayerCost(800.0, 1.0, wp2), 183.28430495656693));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 1.0, wp2), 0.1828272368644059));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 1.0, wp2), 0.170299005766845));
    assert(check_close(lp.ComputeCostPerMm2(10.0, 2.0, wp2), 0.18420533161464012));
    assert(check_close(lp.ComputeCostPerMm2(123.0, 2.0, wp2), 0.16556847782887707));
    
    std::cout << "All Layer tests passed successfully!" << std::endl;
    return 0;
} 