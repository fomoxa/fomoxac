#pragma once

/* A C fixture model, in the exact style the brief's own DeviceState example
 * uses: a plain tagged `struct Name { ... };`, no `typedef`. Every generated
 * reference to one of these types is spelled `struct Name` for that reason -
 * see generator::c::struct_type's doc comment. */

#include "fomoxa.h"

/* FomoxaBytes / FomoxaArray_* are generated types (arrays.h, runtime.h),
 * not standard ones - unlike C++'s std::string/std::vector, a C model that
 * uses `bytes` or `Array<T>` fields has a real build-order dependency on a
 * previous `fomoxac generate` run having produced them. This fixture's
 * generated tree is committed, so that dependency is already satisfied. */
#include "../generated/arrays.h"
#include "../generated/runtime.h"

#include <stdint.h>

FOMOXA_MODEL
FOMOXA_CODEC("edge")
struct PlayerInfo {
    FOMOXA_FIELD(u32)
    FOMOXA_CODEC("edge")
    uint32_t Level;
};

FOMOXA_MODEL
FOMOXA_CODEC("edge", "unity")
struct Player {
    FOMOXA_FIELD(u32)
    FOMOXA_CODEC("edge", "unity")
    uint32_t Id;

    FOMOXA_FIELD(f32)
    FOMOXA_CODEC("edge")
    float X;

    FOMOXA_FIELD(f32)
    FOMOXA_CODEC("edge")
    float Y;

    FOMOXA_FIELD(string)
    FOMOXA_CODEC("edge", "unity")
    const char *Name;

    FOMOXA_FIELD(bytes)
    FOMOXA_CODEC("edge")
    FomoxaBytes Payload;

    /* Not on the wire at all. */
    int Cache;
};

FOMOXA_MODEL
FOMOXA_CODEC("edge")
struct Team {
    FOMOXA_FIELD(PlayerInfo)
    FOMOXA_CODEC("edge")
    struct PlayerInfo Captain;

    FOMOXA_FIELD(Array<string>)
    FOMOXA_CODEC("edge")
    FomoxaArray_string Tags;

    FOMOXA_FIELD(Array<u32>)
    FOMOXA_CODEC("edge")
    FomoxaArray_u32 Scores;

    FOMOXA_FIELD(Array<PlayerInfo>)
    FOMOXA_CODEC("edge")
    FomoxaArray_PlayerInfo Roster;
};
