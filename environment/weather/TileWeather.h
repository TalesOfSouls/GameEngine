
enum ClimateTerrain {
    CLIMATE_TERRAIN_WATER,
    CLIMATE_TERRAIN_PLAINS,
    CLIMATE_TERRAIN_FOREST,
    CLIMATE_TERRAIN_DESERT,
    CLIMATE_TERRAIN_MOUNTAIN,
    CLIMATE_TERRAIN_SIZE
};

struct TileClimate {
    ClimateTerrain terrain;

    // mean temperature in deg C (before season offset)
    float base_temp;
    // day/night swing, +/- deg C
    float temp_amplitude;
    // relative humidity the tile relaxes to, 0..1
    float base_humidity;
    // chance per hour that a rain/snow system starts
    // centred on this tile (0..1, typically 0.0001..0.002)
    float precip_chance;
    // 0..1, tendency for non-raining cloud banks (low = sunny region)
    float cloudiness;
    // 0..1 multiplier, 0 = hail impossible here
    float hail_chance;
    // 0..1 multiplier, 0 = sandstorms impossible here
    float sandstorm_chance;
    // moisture source per hour (water ~0.05); negative = dry sink that keeps deserts arid
    float evaporation;
    // 0..1 wind drag (mountains / forests high)
    float roughness;
};

struct WeatherGlobals {
    // deg C added to every tile (winter: negative)
    float season_temp_offset;
    // deg C added to the snow line; >0 => more snow
    float snow_bias;
    // added to every tile's base humidity
    float humidity_offset;
    // scales how often rain/snow systems spawn
    float precip_multiplier;
    float hail_multiplier;
    float sandstorm_multiplier;
    float fog_multiplier;
    // m/s, large scale background wind (+x = east)
    float prevailing_wind_u;
    // m/s (+y = down the grid / south)
    float prevailing_wind_v;
    // scales thermally driven (pressure) winds
    float wind_strength;
    // 0..24, advanced by weather_step
    float hour_of_day;
};

/**
 * Wind movement
 *
 * coriolis effect
 * friction (based on surface, forest, water, mountain, plane, other)
 * temperature (high temperature = low pressure = wind moves here)
 */

struct ClimateTile {
    // Every tile has 8 flow vectors for sampling
    // These vectors are determined by their surrounding
    v2_f32 flow_vecs[8];

    // Every tile has a drag coefficient that reduces the flow
    f32 roughness;

    f32 temperature_base;

    // day/night swing
    f32 temperature_amplitude;

    f32 humidity;

    // chance to snow/rain
    f32 precip_change;

    f32 cloudiness;

    f32 hail_chance;
    f32 sandstorm_chance;

    // tornado/cyclone chance
    f32 tornado_chance;

    f32 evaporation;
};

struct ClimateGlobal {
    f32 season_temp_offset;
    f32 snow_bias;
    f32 humidity_offset;
    f32 precip_multiplier;
    f32 hail_multiplier;
    f32 sandstorm_multiplier;
    f32 fog_multiplier;
    v2_f32 wind_uv;
    f32 wind_strength;
    f32 daytime;
};