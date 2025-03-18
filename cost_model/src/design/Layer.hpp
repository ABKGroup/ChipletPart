#define OPTIMIZE_WAFER_CALCULATION

// Include OpenMP headers if ENABLE_OPENMP is defined
#if defined(ENABLE_OPENMP) && !defined(DISABLE_OPENMP)
  #include <omp.h>
#endif

// Optimized DieLocation struct
struct DieLocation {
    FloatType x;
    FloatType y;
    
    DieLocation(FloatType x_loc, FloatType y_loc) : x(x_loc), y(y_loc) {}
    
    // Optimize comparison operators
    bool operator==(const DieLocation& other) const {
        return (x == other.x && y == other.y);
    }
    
    bool operator!=(const DieLocation& other) const {
        return !(*this == other);
    }
}; 