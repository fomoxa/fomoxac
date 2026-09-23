#pragma once

#include "fomoxa.h"

#include "../generated/arrays.h"
#include "../generated/runtime.h"

#include <stdbool.h>
#include <stdint.h>

FOMOXA_MODEL
FOMOXA_CODEC("edge", "unity")
struct DeviceState {
    FOMOXA_FIELD(u32)
    FOMOXA_CODEC("edge", "unity")
    uint32_t Id;

    FOMOXA_FIELD(f32)
    FOMOXA_CODEC("edge")
    float Temperature;

    FOMOXA_FIELD(string)
    FOMOXA_CODEC("unity")
    const char *DisplayName;

    FOMOXA_FIELD(u32)
    uint32_t Unrouted;

    int Cache;
};

FOMOXA_MODEL
FOMOXA_CODEC("edge")
struct EveryPrimitive {
    FOMOXA_FIELD(bool)
    FOMOXA_CODEC("edge")
    bool Flag;

    FOMOXA_FIELD(i8)
    FOMOXA_CODEC("edge")
    int8_t Tiny;

    FOMOXA_FIELD(u8)
    FOMOXA_CODEC("edge")
    uint8_t Byte;

    FOMOXA_FIELD(i16)
    FOMOXA_CODEC("edge")
    int16_t Small;

    FOMOXA_FIELD(u16)
    FOMOXA_CODEC("edge")
    uint16_t Port;

    FOMOXA_FIELD(i32)
    FOMOXA_CODEC("edge")
    int32_t Offset;

    FOMOXA_FIELD(u32)
    FOMOXA_CODEC("edge")
    uint32_t Count;

    FOMOXA_FIELD(i64)
    FOMOXA_CODEC("edge")
    int64_t Delta;

    FOMOXA_FIELD(u64)
    FOMOXA_CODEC("edge")
    uint64_t Sequence;

    FOMOXA_FIELD(f32)
    FOMOXA_CODEC("edge")
    float Ratio;

    FOMOXA_FIELD(f64)
    FOMOXA_CODEC("edge")
    double Precise;

    FOMOXA_FIELD(string)
    FOMOXA_CODEC("edge")
    const char *Label;

    FOMOXA_FIELD(bytes)
    FOMOXA_CODEC("edge")
    FomoxaBytes Blob;
};

FOMOXA_MODEL
FOMOXA_CODEC("edge")
struct PlayerInfo {
    FOMOXA_FIELD(u32)
    FOMOXA_CODEC("edge")
    uint32_t Level;
};

FOMOXA_MODEL
FOMOXA_CODEC("edge")
struct Player {
    FOMOXA_FIELD(u32)
    FOMOXA_CODEC("edge")
    uint32_t Id;

    FOMOXA_FIELD(f32)
    FOMOXA_CODEC("edge")
    float X;

    FOMOXA_FIELD(f32)
    FOMOXA_CODEC("edge")
    float Y;
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
