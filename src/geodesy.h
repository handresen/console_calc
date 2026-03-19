#pragma once

#include "console_calc/value.h"

namespace console_calc {

struct GeodesicInverseResult {
    double distance_m = 0.0;
    double initial_bearing_deg = 0.0;
};

[[nodiscard]] PositionValue normalize_position(double latitude_deg, double longitude_deg);
[[nodiscard]] GeodesicInverseResult wgs84_inverse(const PositionValue& start,
                                                 const PositionValue& end);
[[nodiscard]] PositionValue wgs84_direct(const PositionValue& start, double bearing_deg,
                                         double distance_m);

}  // namespace console_calc
