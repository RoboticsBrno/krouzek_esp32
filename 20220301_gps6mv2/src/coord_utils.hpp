#pragma once

#include <math.h>

namespace coords {

// https://www.movable-type.co.uk/scripts/latlong.html

static constexpr const float EARTH_RADIUS_METERS = 6371000;

// Haversine formula, precise but slower
float distanceMeters(const float src_lat, const float src_lon, const float dst_lat, const float dst_lon) {
    const float delta_lat = (dst_lat - src_lat) * M_PI/180;
    const float delta_lon = (dst_lon - src_lon) * M_PI/180;

    const float sin_lat = sinf(delta_lat / 2);
    const float sin_lon = sinf(delta_lon / 2);

    const float a = sin_lat * sin_lat +
        cosf(src_lat * M_PI/180) * cosf(dst_lat * M_PI/180) *
        sin_lon * sin_lon;
    const float c = 2 * atan2f(sqrtf(a), sqrtf(1 - a));
    return c * EARTH_RADIUS_METERS; // to meters
}

// Equirectangular approximation, fast but inaccurate over great distances
float distanceMetersFast(const float src_lat, const float src_lon, const float dst_lat, const float dst_lon) {
    const float x = ((dst_lon - src_lon) * M_PI/180) * cosf(((src_lat + dst_lat)/2) * M_PI/180);
    const float y = (dst_lat - src_lat) * M_PI/180;
    return sqrtf(x*x + y*y) * EARTH_RADIUS_METERS;
}

float bearingInitial(const float src_lat, const float src_lon, const float dst_lat, const float dst_lon) {
 return 0;
}

};
