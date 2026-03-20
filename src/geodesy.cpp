#include "geodesy.h"

#include <GeographicLib/Geodesic.hpp>

#include <cmath>

#include "console_calc/expression_error.h"

namespace console_calc {

namespace {

// Geo builtins use GeographicLib's Geodesic implementation on the WGS84 ellipsoid:
// - wgs84_inverse(): inverse problem (distance + initial bearing)
// - wgs84_direct(): direct problem (destination from start/bearing/range)
//
// Compared with the previous Vincenty implementation, GeographicLib is more
// robust for nearly antipodal cases while keeping excellent geodetic accuracy.
// In normal use the results are effectively at round-off level for double
// precision and are usually more accurate than the input coordinates warrant.

[[nodiscard]] double normalize_longitude_degrees(double value) {
    double normalized = std::fmod(value + 180.0, 360.0);
    if (normalized < 0.0) {
        normalized += 360.0;
    }
    return normalized - 180.0;
}

[[nodiscard]] double normalize_bearing_degrees(double value) {
    double normalized = std::fmod(value, 360.0);
    if (normalized < 0.0) {
        normalized += 360.0;
    }
    return normalized;
}

[[nodiscard]] const GeographicLib::Geodesic& wgs84_geodesic() {
    return GeographicLib::Geodesic::WGS84();
}

}  // namespace

PositionValue normalize_position(double latitude_deg, double longitude_deg) {
    if (!std::isfinite(latitude_deg) || latitude_deg < -90.0 || latitude_deg > 90.0) {
        throw EvaluationError("pos() latitude must be within [-90, 90] degrees");
    }
    if (!std::isfinite(longitude_deg)) {
        throw EvaluationError("pos() longitude must be finite");
    }

    return PositionValue{
        .latitude_deg = latitude_deg,
        .longitude_deg = normalize_longitude_degrees(longitude_deg),
    };
}

GeodesicInverseResult wgs84_inverse(const PositionValue& start, const PositionValue& end) {
    // GeographicLib solves the inverse geodesic problem on WGS84 directly from
    // geodetic latitude/longitude in degrees, ignoring altitude and terrain.
    double distance = 0.0;
    double initial_bearing = 0.0;
    double final_bearing = 0.0;
    try {
        wgs84_geodesic().Inverse(start.latitude_deg, start.longitude_deg, end.latitude_deg,
                                 end.longitude_deg, distance, initial_bearing, final_bearing);
    } catch (const GeographicLib::GeographicErr& error) {
        throw EvaluationError(error.what());
    }

    return GeodesicInverseResult{
        .distance_m = distance,
        .initial_bearing_deg = normalize_bearing_degrees(initial_bearing),
    };
}

PositionValue wgs84_direct(const PositionValue& start, double bearing_deg, double distance_m) {
    if (!std::isfinite(bearing_deg)) {
        throw EvaluationError("bearing must be finite");
    }
    if (!std::isfinite(distance_m) || distance_m < 0.0) {
        throw EvaluationError("range must be a non-negative finite distance");
    }
    if (distance_m == 0.0) {
        return start;
    }

    // GeographicLib solves the direct geodesic problem on WGS84 directly from
    // geodetic latitude/longitude in degrees, initial bearing in degrees
    // clockwise from north, and ellipsoidal surface distance in meters.
    double latitude_deg = 0.0;
    double longitude_deg = 0.0;
    try {
        wgs84_geodesic().Direct(start.latitude_deg, start.longitude_deg,
                                normalize_bearing_degrees(bearing_deg), distance_m,
                                latitude_deg, longitude_deg);
    } catch (const GeographicLib::GeographicErr& error) {
        throw EvaluationError(error.what());
    }

    return normalize_position(latitude_deg, longitude_deg);
}

}  // namespace console_calc
