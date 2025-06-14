#ifndef COMS_ENVIRONMENT_UNIVERSE_H
#define COMS_ENVIRONMENT_UNIVERSE_H

#include "../stdlib/Types.h"

enum StarType {
    STAR_TYPE_STAR,
    STAR_TYPE_RED_GIANT,
    STAR_TYPE_WHITE_DWARF,
    STAR_TYPE_NEUTRON_STAR,
    STAR_TYPE_SUPERGIANT,
    STAR_TYPE_HYPERGIANT,
    STAR_TYPE_PULSAR,
    STAR_TYPE_VARIABLE_STAR,
    STAR_TYPE_QUARK_STAR,
    STAR_TYPE_BOSON_STAR,
    STAR_TYPE_STRANGE_STAR,
};

struct StarDistribution {
    int16 count_min; // per galaxy
    int16 count_max; // per galaxy
    int64 age_min;
    int64 age_max;
    f32 mass_min; // in relation to sun
    f32 mass_max; // in relation to sun
    int32 radius_min; // maybe use age instead, since that basically defines the state of the star
    int32 radius_max; // maybe use age instead, since that basically defines the state of the star
    f32 rotation_min;
    f32 rotation_max;
    byte multiple_max;
};

static const StarDistribution STAR_TYPE_DISTRIBUTION[] = {
    { // STAR_TYPE_STAR
        .count_min = 800,
        .count_max = 1000,
        .age_min = 0,
        .age_max = 0,
        .mass_min = 0,
        .mass_max = 0,
        .radius_min = 0,
        .radius_max = 0,
        .rotation_min = 0,
        .rotation_max = 0,
        .multiple_max = 5 // higher = less likely (1 = no multiple allowed)
    }
};

enum BlackHoleType {
    BLACK_HOLE_TYPE_STELLAR,
    BLACK_HOLE_TYPE_SUPERMASSIVE,
    BLACK_HOLE_TYPE_INTERMEDIATE,
    BLACK_HOLE_TYPE_PRIMORDIAL,
    BLACK_HOLE_TYPE_MICRO,
};

enum PlanetType {
    PLANET_TYPE_TERRESTRIAL_PLANET,
    PLANET_TYPE_GAS_GIANT,
    PLANET_TYPE_ICE_GIANTS,
    PLANET_TYPE_DWARF_PLANET,
    PLANET_TYPE_ROUGE,
};

enum AsteroidType {
    ASTEROID_TYPE_BELT_ASTEROID,
    ASTEROID_TYPE_TROJAN,
    ASTEROID_TYPE_WANDERING,
};

enum GalaxyType {
    GALAXY_TYPE_SPIRAL,
    GALAXY_TYPE_ELLIPTICAL,
    GALAXY_TYPE_IRREGULAR,
};

enum NebulaeType {
    NEBULA_TYPE_EMISSION_NEBULAE,
    NEBULA_TYPE_REFLECTION_NEBULAE,
    NEBULA_TYPE_DARK_NEBULAE,
    NEBULA_TYPE_PLANETARY_NEBULAE,
    NEBULA_TYPE_SUPERNOVA_NEBULAE,
};

struct Star {
    StarType type;
    bool is_pulsar;
    bool is_magnetar;
};

struct WormHole {

};

struct MegaStructure {

};

struct BlackHole {
    BlackHoleType type;
    bool is_quasar;
    f32 rotation;
    f32 charge;
};

struct CelestialBody {

};

struct Galaxy {
    CelestialBody* celestial_bodies;
    int32 celestial_body_count;


};

struct Universe {
    Galaxy* galaxies;
    int32 galaxy_count;
};

#define GALAXY_COUNT 100

#define GALAXY_STAR_COUNT_MIN 800
#define GALAXY_STAR_COUNT_MIN 1000

#define STAR_PLANET_COUNT_MIN 800
#define STAR_PLANET_COUNT_MIN 1000

void galaxy_generate(Galaxy* galaxy) {

    for (int32 i = 0; i )
}

void universe_generate(Universe* universe) {
    for (int32 i = 0; i < GALAXY_COUNT; ++i) {
        galaxy_generate(&universe->galaxies[i]);
    }
}

#endif