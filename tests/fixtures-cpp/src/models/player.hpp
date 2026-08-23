// player.hpp is the annotated source the C++ integration tests exercise - the
// C++ counterpart of tests/fixtures-cs/src/models/Player.cs: three models
// annotated in place, the generated codecs compiled against these same
// types, and the model RFC-0002 §9.1's version-skew tests decode.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "fomoxa.h"

namespace models {

/// A field whose network type is another model.
FOMOXA_MODEL
FOMOXA_CODEC("edge")
struct PlayerInfo
{
    FOMOXA_FIELD(u32)
    FOMOXA_CODEC("edge")
    uint32_t Level = 0;
};

/// The model RFC-0002 §9.1 is tested against: three fields, and a version
/// that appends a fourth.
FOMOXA_MODEL
FOMOXA_CODEC("edge", "unity")
struct Player
{
    FOMOXA_FIELD(u32)
    FOMOXA_CODEC("edge", "unity")
    uint32_t Id = 0;

    FOMOXA_FIELD(f32)
    FOMOXA_CODEC("edge")
    float X = 0.0f;

    FOMOXA_FIELD(f32)
    FOMOXA_CODEC("edge")
    float Y = 0.0f;

    /// A network field in no codec: it is written by none of them.
    FOMOXA_FIELD(u32)
    uint32_t Unrouted = 0;

    /// Not a network field at all. Logic and caches stay off the wire.
    std::string Cache;
};

/// Holds composites: an array of primitives, an array of models, and a
/// nested model.
FOMOXA_MODEL
FOMOXA_CODEC("edge")
struct Team
{
    FOMOXA_FIELD(PlayerInfo)
    FOMOXA_CODEC("edge")
    PlayerInfo Captain;

    FOMOXA_FIELD(Array<string>)
    FOMOXA_CODEC("edge")
    std::vector<std::string> Tags;

    FOMOXA_FIELD(Array<u32>)
    FOMOXA_CODEC("edge")
    std::vector<uint32_t> Scores;

    FOMOXA_FIELD(Array<PlayerInfo>)
    FOMOXA_CODEC("edge")
    std::vector<PlayerInfo> Roster;
};

}  // namespace models
