#pragma once

#include "console_calc/value.h"

namespace console_calc {

struct GeodesicInverseResult {
    double distance_m = 0.0;
    double initial_bearing_deg = 0.0;
};

// Position normalization assumes geodetic WGS84 latitude/longitude input in degrees,
// with latitude constrained to [-90, 90] and longitude wrapped to [-180, 180).
[[nodiscard]] PositionValue normalize_position(double latitude_deg, double longitude_deg);

// Solve the WGS84 inverse geodesic problem: distance and initial bearing from start to end.
// Implementation uses Vincenty's iterative inverse formula on the WGS84 ellipsoid.
// Assumptions:
// - inputs are geodetic latitude/longitude in degrees
// - altitude is ignored; distances are ellipsoidal surface distances
// - result bearing is the initial forward azimuth in degrees clockwise from north
// Expected accuracy:
// - typically sub-millimeter to millimeter-level when the iteration converges
// - practical accuracy is limited by IEEE double precision and the WGS84 model itself
// - near-antipodal cases may fail to converge and are reported as errors
[[nodiscard]] GeodesicInverseResult wgs84_inverse(const PositionValue& start,
                                                 const PositionValue& end);

// Sum the WGS84 ellipsoidal path length over consecutive points in a position list.
// Lists with fewer than two points have zero path length.
[[nodiscard]] double wgs84_path_distance(const PositionListValue& positions);
[[nodiscard]] PositionListValue densify_wgs84_path(const PositionListValue& positions,
                                                   std::size_t inserted_per_leg);

// Solve the WGS84 direct geodesic problem: destination from start, initial bearing, and range.
// Implementation uses Vincenty's iterative direct formula on the WGS84 ellipsoid.
// Assumptions:
// - start is geodetic latitude/longitude in degrees
// - bearing is degrees clockwise from north
// - range is a non-negative ellipsoidal surface distance in meters
// - altitude is ignored
// Expected accuracy:
// - typically sub-millimeter to millimeter-level when the iteration converges
// - practical accuracy is limited by IEEE double precision and the WGS84 model itself
// - pathological near-antipodal-style cases can fail to converge and are reported as errors
[[nodiscard]] PositionValue wgs84_direct(const PositionValue& start, double bearing_deg,
                                         double distance_m);

}  // namespace console_calc
