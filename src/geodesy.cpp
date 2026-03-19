#include "geodesy.h"

#include <cmath>
#include <limits>
#include <numbers>

#include "console_calc/expression_error.h"

namespace console_calc {

namespace {

constexpr double k_wgs84_a = 6378137.0;
constexpr double k_wgs84_f = 1.0 / 298.257223563;
constexpr double k_wgs84_b = (1.0 - k_wgs84_f) * k_wgs84_a;
constexpr int k_max_iterations = 200;
constexpr double k_convergence_epsilon = 1e-12;

[[nodiscard]] double degrees_to_radians(double value) {
    return value * std::numbers::pi_v<double> / 180.0;
}

[[nodiscard]] double radians_to_degrees(double value) {
    return value * 180.0 / std::numbers::pi_v<double>;
}

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

[[nodiscard]] double square(double value) { return value * value; }

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
    const double phi1 = degrees_to_radians(start.latitude_deg);
    const double phi2 = degrees_to_radians(end.latitude_deg);
    const double L = degrees_to_radians(
        normalize_longitude_degrees(end.longitude_deg - start.longitude_deg));

    const double U1 = std::atan((1.0 - k_wgs84_f) * std::tan(phi1));
    const double U2 = std::atan((1.0 - k_wgs84_f) * std::tan(phi2));
    const double sinU1 = std::sin(U1);
    const double cosU1 = std::cos(U1);
    const double sinU2 = std::sin(U2);
    const double cosU2 = std::cos(U2);

    double lambda = L;
    double lambda_previous = 0.0;
    double sin_sigma = 0.0;
    double cos_sigma = 1.0;
    double sigma = 0.0;
    double sin_alpha = 0.0;
    double cos_sq_alpha = 1.0;
    double cos2_sigma_m = 0.0;

    for (int iteration = 0; iteration < k_max_iterations; ++iteration) {
        const double sin_lambda = std::sin(lambda);
        const double cos_lambda = std::cos(lambda);
        sin_sigma = std::sqrt(square(cosU2 * sin_lambda) +
                              square(cosU1 * sinU2 - sinU1 * cosU2 * cos_lambda));
        if (sin_sigma == 0.0) {
            return GeodesicInverseResult{};
        }

        cos_sigma = sinU1 * sinU2 + cosU1 * cosU2 * cos_lambda;
        sigma = std::atan2(sin_sigma, cos_sigma);
        sin_alpha = (cosU1 * cosU2 * sin_lambda) / sin_sigma;
        cos_sq_alpha = 1.0 - square(sin_alpha);
        cos2_sigma_m = cos_sq_alpha == 0.0
                           ? 0.0
                           : cos_sigma - (2.0 * sinU1 * sinU2) / cos_sq_alpha;
        const double C = (k_wgs84_f / 16.0) * cos_sq_alpha *
                         (4.0 + k_wgs84_f * (4.0 - 3.0 * cos_sq_alpha));

        lambda_previous = lambda;
        lambda = L + (1.0 - C) * k_wgs84_f * sin_alpha *
                          (sigma + C * sin_sigma *
                                       (cos2_sigma_m +
                                        C * cos_sigma *
                                            (-1.0 + 2.0 * square(cos2_sigma_m))));
        if (std::fabs(lambda - lambda_previous) < k_convergence_epsilon) {
            break;
        }

        if (iteration == k_max_iterations - 1) {
            throw EvaluationError("geodesic calculation did not converge");
        }
    }

    const double u_sq = cos_sq_alpha * (square(k_wgs84_a) - square(k_wgs84_b)) / square(k_wgs84_b);
    const double A = 1.0 + (u_sq / 16384.0) *
                               (4096.0 + u_sq * (-768.0 + u_sq * (320.0 - 175.0 * u_sq)));
    const double B = (u_sq / 1024.0) *
                     (256.0 + u_sq * (-128.0 + u_sq * (74.0 - 47.0 * u_sq)));
    const double delta_sigma =
        B * sin_sigma *
        (cos2_sigma_m +
         (B / 4.0) *
             (cos_sigma * (-1.0 + 2.0 * square(cos2_sigma_m)) -
              (B / 6.0) * cos2_sigma_m * (-3.0 + 4.0 * square(sin_sigma)) *
                  (-3.0 + 4.0 * square(cos2_sigma_m))));

    const double distance = k_wgs84_b * A * (sigma - delta_sigma);
    const double alpha1 =
        std::atan2(cosU2 * std::sin(lambda),
                   cosU1 * sinU2 - sinU1 * cosU2 * std::cos(lambda));

    return GeodesicInverseResult{
        .distance_m = distance,
        .initial_bearing_deg = normalize_bearing_degrees(radians_to_degrees(alpha1)),
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

    const double alpha1 = degrees_to_radians(normalize_bearing_degrees(bearing_deg));
    const double phi1 = degrees_to_radians(start.latitude_deg);
    const double lambda1 = degrees_to_radians(start.longitude_deg);

    const double tanU1 = (1.0 - k_wgs84_f) * std::tan(phi1);
    const double cosU1 = 1.0 / std::sqrt(1.0 + square(tanU1));
    const double sinU1 = tanU1 * cosU1;
    const double sin_alpha1 = std::sin(alpha1);
    const double cos_alpha1 = std::cos(alpha1);
    const double sigma1 = std::atan2(tanU1, cos_alpha1);
    const double sin_alpha = cosU1 * sin_alpha1;
    const double cos_sq_alpha = 1.0 - square(sin_alpha);
    const double u_sq = cos_sq_alpha * (square(k_wgs84_a) - square(k_wgs84_b)) / square(k_wgs84_b);
    const double A = 1.0 + (u_sq / 16384.0) *
                               (4096.0 + u_sq * (-768.0 + u_sq * (320.0 - 175.0 * u_sq)));
    const double B = (u_sq / 1024.0) *
                     (256.0 + u_sq * (-128.0 + u_sq * (74.0 - 47.0 * u_sq)));

    double sigma = distance_m / (k_wgs84_b * A);
    double sigma_previous = 0.0;
    double cos2_sigma_m = 0.0;
    double sin_sigma = 0.0;
    double cos_sigma = 1.0;

    for (int iteration = 0; iteration < k_max_iterations; ++iteration) {
        cos2_sigma_m = std::cos(2.0 * sigma1 + sigma);
        sin_sigma = std::sin(sigma);
        cos_sigma = std::cos(sigma);
        const double delta_sigma =
            B * sin_sigma *
            (cos2_sigma_m +
             (B / 4.0) *
                 (cos_sigma * (-1.0 + 2.0 * square(cos2_sigma_m)) -
                  (B / 6.0) * cos2_sigma_m * (-3.0 + 4.0 * square(sin_sigma)) *
                      (-3.0 + 4.0 * square(cos2_sigma_m))));

        sigma_previous = sigma;
        sigma = distance_m / (k_wgs84_b * A) + delta_sigma;
        if (std::fabs(sigma - sigma_previous) < k_convergence_epsilon) {
            break;
        }

        if (iteration == k_max_iterations - 1) {
            throw EvaluationError("geodesic calculation did not converge");
        }
    }

    const double tmp = sinU1 * sin_sigma - cosU1 * cos_sigma * cos_alpha1;
    const double phi2 = std::atan2(
        sinU1 * cos_sigma + cosU1 * sin_sigma * cos_alpha1,
        (1.0 - k_wgs84_f) * std::sqrt(square(sin_alpha) + square(tmp)));
    const double lambda =
        std::atan2(sin_sigma * sin_alpha1, cosU1 * cos_sigma - sinU1 * sin_sigma * cos_alpha1);
    const double C = (k_wgs84_f / 16.0) * cos_sq_alpha *
                     (4.0 + k_wgs84_f * (4.0 - 3.0 * cos_sq_alpha));
    const double L =
        lambda - (1.0 - C) * k_wgs84_f * sin_alpha *
                     (sigma + C * sin_sigma *
                                  (cos2_sigma_m +
                                   C * cos_sigma * (-1.0 + 2.0 * square(cos2_sigma_m))));

    return normalize_position(radians_to_degrees(phi2), radians_to_degrees(lambda1 + L));
}

}  // namespace console_calc
