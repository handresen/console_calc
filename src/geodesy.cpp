#include "geodesy.h"

#include <GeographicLib/Geodesic.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <span>
#include <vector>

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

[[nodiscard]] double normalize_longitude_delta_degrees(double value) {
    double normalized = std::fmod(value + 180.0, 360.0);
    if (normalized < 0.0) {
        normalized += 360.0;
    }
    return normalized - 180.0;
}

[[nodiscard]] double normalize_bearing_delta_degrees(double value) {
    double normalized = std::fmod(value + 180.0, 360.0);
    if (normalized < 0.0) {
        normalized += 360.0;
    }
    return normalized - 180.0;
}

[[nodiscard]] const GeographicLib::Geodesic& wgs84_geodesic() {
    return GeographicLib::Geodesic::WGS84();
}

struct CompressionCandidate {
    std::size_t index = 0;
    double penalty_m = 0.0;
    double neighbor_distance_m = 0.0;
};

[[nodiscard]] std::optional<double> previous_nonzero_heading(
    std::span<const PositionValue> positions, std::size_t index) {
    for (std::size_t segment = index; segment > 0U; --segment) {
        const GeodesicInverseResult inverse =
            wgs84_inverse(positions[segment - 1U], positions[segment]);
        if (inverse.distance_m > 0.0) {
            return inverse.initial_bearing_deg;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<double> next_nonzero_heading(
    std::span<const PositionValue> positions, std::size_t index) {
    for (std::size_t segment = index; segment + 1U < positions.size(); ++segment) {
        const GeodesicInverseResult inverse =
            wgs84_inverse(positions[segment], positions[segment + 1U]);
        if (inverse.distance_m > 0.0) {
            return inverse.initial_bearing_deg;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<double> path_heading_at(std::span<const PositionValue> positions,
                                                    std::size_t index) {
    const auto previous = previous_nonzero_heading(positions, index);
    const auto next = next_nonzero_heading(positions, index);
    if (!previous.has_value() && !next.has_value()) {
        return std::nullopt;
    }
    if (!previous.has_value()) {
        return next;
    }
    if (!next.has_value()) {
        return previous;
    }

    constexpr double k_pi = 3.14159265358979323846;
    const double previous_rad = *previous * k_pi / 180.0;
    const double next_rad = *next * k_pi / 180.0;
    const double x = std::cos(previous_rad) + std::cos(next_rad);
    const double y = std::sin(previous_rad) + std::sin(next_rad);
    if (std::fabs(x) < 1e-12 && std::fabs(y) < 1e-12) {
        return next;
    }
    return normalize_bearing_degrees(std::atan2(y, x) * 180.0 / k_pi);
}

[[nodiscard]] double point_to_segment_distance_m(const PositionValue& point,
                                                 const PositionValue& start,
                                                 const PositionValue& end) {
    constexpr double k_earth_radius_m = 6371008.8;
    constexpr double k_pi = 3.14159265358979323846;
    const GeodesicInverseResult start_to_end = wgs84_inverse(start, end);
    const GeodesicInverseResult start_to_point = wgs84_inverse(start, point);
    if (start_to_end.distance_m == 0.0) {
        return start_to_point.distance_m;
    }

    if (std::fabs(normalize_bearing_delta_degrees(start_to_point.initial_bearing_deg -
                                                  start_to_end.initial_bearing_deg)) > 90.0) {
        return start_to_point.distance_m;
    }

    const GeodesicInverseResult end_to_start = wgs84_inverse(end, start);
    const GeodesicInverseResult end_to_point = wgs84_inverse(end, point);
    if (std::fabs(normalize_bearing_delta_degrees(end_to_point.initial_bearing_deg -
                                                  end_to_start.initial_bearing_deg)) > 90.0) {
        return end_to_point.distance_m;
    }

    const double delta13 = start_to_point.distance_m / k_earth_radius_m;
    const double theta13 = start_to_point.initial_bearing_deg * k_pi / 180.0;
    const double theta12 = start_to_end.initial_bearing_deg * k_pi / 180.0;
    const double cross_track_sine =
        std::clamp(std::sin(delta13) * std::sin(theta13 - theta12), -1.0, 1.0);
    return std::fabs(std::asin(cross_track_sine) * k_earth_radius_m);
}

void mark_simplified_points(std::span<const PositionValue> positions, std::size_t start_index,
                            std::size_t end_index, double tolerance_m,
                            std::vector<bool>& keep) {
    if (end_index <= start_index + 1U) {
        return;
    }

    double max_distance_m = -1.0;
    std::size_t split_index = start_index;
    for (std::size_t index = start_index + 1U; index < end_index; ++index) {
        const double distance_m = point_to_segment_distance_m(
            positions[index], positions[start_index], positions[end_index]);
        if (distance_m > max_distance_m) {
            max_distance_m = distance_m;
            split_index = index;
        }
    }

    if (max_distance_m > tolerance_m) {
        keep[split_index] = true;
        mark_simplified_points(positions, start_index, split_index, tolerance_m, keep);
        mark_simplified_points(positions, split_index, end_index, tolerance_m, keep);
    }
}

[[nodiscard]] double merged_span_penalty_m(std::span<const PositionValue> positions,
                                           std::size_t start_index,
                                           std::size_t end_index) {
    double penalty_m = 0.0;
    for (std::size_t index = start_index + 1U; index < end_index; ++index) {
        penalty_m = std::max(
            penalty_m,
            point_to_segment_distance_m(positions[index], positions[start_index],
                                        positions[end_index]));
    }
    return penalty_m;
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

double wgs84_path_distance(const PositionListValue& positions) {
    if (positions.size() < 2U) {
        return 0.0;
    }

    double total_distance_m = 0.0;
    for (std::size_t index = 1; index < positions.size(); ++index) {
        total_distance_m += wgs84_inverse(positions[index - 1U], positions[index]).distance_m;
    }
    return total_distance_m;
}

PositionListValue densify_wgs84_path(const PositionListValue& positions,
                                     std::size_t inserted_per_leg) {
    if (positions.size() < 2U || inserted_per_leg == 0U) {
        return positions;
    }

    PositionListValue dense_positions;
    dense_positions.reserve(positions.size() + (positions.size() - 1U) * inserted_per_leg);
    dense_positions.push_back(positions.front());

    for (std::size_t index = 1; index < positions.size(); ++index) {
        const PositionValue& start = positions[index - 1U];
        const PositionValue& end = positions[index];
        const GeodesicInverseResult leg = wgs84_inverse(start, end);
        const std::size_t subdivisions = inserted_per_leg + 1U;
        for (std::size_t step = 1; step <= inserted_per_leg; ++step) {
            const double fraction =
                static_cast<double>(step) / static_cast<double>(subdivisions);
            dense_positions.push_back(
                wgs84_direct(start, leg.initial_bearing_deg, leg.distance_m * fraction));
        }
        dense_positions.push_back(end);
    }

    return dense_positions;
}

PositionListValue offset_wgs84_path(const PositionListValue& positions, double offset_x_m,
                                    double offset_y_m) {
    if (!std::isfinite(offset_x_m) || !std::isfinite(offset_y_m)) {
        throw EvaluationError("offset_path() offsets must be finite");
    }
    if (positions.size() < 2U || (offset_x_m == 0.0 && offset_y_m == 0.0)) {
        return positions;
    }

    const std::size_t center_index = positions.size() / 2U;
    const PositionValue center = positions[center_index];
    const auto heading = path_heading_at(positions, center_index);
    if (!heading.has_value()) {
        return positions;
    }

    constexpr double k_pi = 3.14159265358979323846;
    const double translation_distance_m = std::hypot(offset_x_m, offset_y_m);
    const double translation_bearing_deg =
        normalize_bearing_degrees(*heading + std::atan2(offset_x_m, offset_y_m) * 180.0 / k_pi);
    const PositionValue shifted_center =
        wgs84_direct(center, translation_bearing_deg, translation_distance_m);

    PositionListValue shifted_positions;
    shifted_positions.reserve(positions.size());
    for (const auto& position : positions) {
        const GeodesicInverseResult relative = wgs84_inverse(center, position);
        shifted_positions.push_back(
            wgs84_direct(shifted_center, relative.initial_bearing_deg, relative.distance_m));
    }
    return shifted_positions;
}

PositionListValue rotate_wgs84_path(const PositionListValue& positions, std::size_t center_index,
                                    double degrees) {
    if (!std::isfinite(degrees)) {
        throw EvaluationError("rotate_path() degrees must be finite");
    }
    if (center_index >= positions.size()) {
        throw EvaluationError("rotate_path() center index is out of range");
    }
    if (positions.size() < 2U || degrees == 0.0) {
        return positions;
    }

    const PositionValue center = positions[center_index];
    PositionListValue rotated_positions;
    rotated_positions.reserve(positions.size());
    for (const auto& position : positions) {
        const GeodesicInverseResult relative = wgs84_inverse(center, position);
        rotated_positions.push_back(
            wgs84_direct(center, normalize_bearing_degrees(relative.initial_bearing_deg + degrees),
                         relative.distance_m));
    }
    return rotated_positions;
}

PositionListValue simplify_wgs84_path(const PositionListValue& positions, double tolerance_m) {
    if (!std::isfinite(tolerance_m) || tolerance_m < 0.0) {
        throw EvaluationError("simplify_path() tolerance must be a non-negative finite distance");
    }
    if (positions.size() < 3U) {
        return positions;
    }

    std::vector<bool> keep(positions.size(), false);
    keep.front() = true;
    keep.back() = true;
    mark_simplified_points(positions, 0U, positions.size() - 1U, tolerance_m, keep);

    PositionListValue simplified;
    simplified.reserve(positions.size());
    for (std::size_t index = 0; index < positions.size(); ++index) {
        if (keep[index]) {
            simplified.push_back(positions[index]);
        }
    }
    return simplified;
}

PositionListValue compress_wgs84_path(const PositionListValue& positions,
                                      std::size_t target_count, std::size_t max_points) {
    if (positions.size() > max_points) {
        throw EvaluationError("compress_path() supports at most " +
                              std::to_string(max_points) + " positions");
    }
    if (positions.empty()) {
        if (target_count != 0U) {
            throw EvaluationError("compress_path() target count must match empty path");
        }
        return positions;
    }
    if (positions.size() == 1U) {
        if (target_count != 1U) {
            throw EvaluationError("compress_path() target count must be 1 for a single-point path");
        }
        return positions;
    }
    if (target_count < 2U || target_count > positions.size()) {
        throw EvaluationError("compress_path() target count must be in range 2..path length");
    }
    if (target_count == positions.size()) {
        return positions;
    }

    std::vector<bool> active(positions.size(), true);
    std::size_t active_count = positions.size();

    while (active_count > target_count) {
        bool found_candidate = false;
        CompressionCandidate best{};

        std::size_t prev_active = 0U;
        for (std::size_t index = 1U; index + 1U < positions.size(); ++index) {
            if (!active[index]) {
                continue;
            }

            std::size_t next_active = index + 1U;
            while (next_active < positions.size() && !active[next_active]) {
                ++next_active;
            }
            if (next_active >= positions.size()) {
                break;
            }

            const double penalty_m =
                merged_span_penalty_m(positions, prev_active, next_active);
            const double neighbor_distance_m =
                wgs84_inverse(positions[prev_active], positions[index]).distance_m +
                wgs84_inverse(positions[index], positions[next_active]).distance_m;

            const CompressionCandidate candidate{
                .index = index,
                .penalty_m = penalty_m,
                .neighbor_distance_m = neighbor_distance_m,
            };

            if (!found_candidate || candidate.penalty_m < best.penalty_m ||
                (candidate.penalty_m == best.penalty_m &&
                 candidate.neighbor_distance_m < best.neighbor_distance_m) ||
                (candidate.penalty_m == best.penalty_m &&
                 candidate.neighbor_distance_m == best.neighbor_distance_m &&
                 candidate.index < best.index)) {
                best = candidate;
                found_candidate = true;
            }

            prev_active = index;
        }

        if (!found_candidate) {
            throw EvaluationError("compress_path() could not reach requested target count");
        }

        active[best.index] = false;
        --active_count;
    }

    PositionListValue compressed;
    compressed.reserve(target_count);
    for (std::size_t index = 0; index < positions.size(); ++index) {
        if (active[index]) {
            compressed.push_back(positions[index]);
        }
    }
    return compressed;
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
