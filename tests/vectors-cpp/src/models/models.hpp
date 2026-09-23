#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "fomoxa.h"

namespace models {

FOMOXA_MODEL
FOMOXA_CODEC("edge", "unity")
struct DeviceState
{
    FOMOXA_FIELD(u32)
    FOMOXA_CODEC("edge", "unity")
    uint32_t Id = 0;

    FOMOXA_FIELD(f32)
    FOMOXA_CODEC("edge")
    float Temperature = 0.0f;

    FOMOXA_FIELD(string)
    FOMOXA_CODEC("unity")
    std::string DisplayName;

    FOMOXA_FIELD(u32)
    uint32_t Unrouted = 0;

    std::string Cache;
};

FOMOXA_MODEL
FOMOXA_CODEC("edge")
struct EveryPrimitive
{
    FOMOXA_FIELD(bool)
    FOMOXA_CODEC("edge")
    bool Flag = false;

    FOMOXA_FIELD(i8)
    FOMOXA_CODEC("edge")
    int8_t Tiny = 0;

    FOMOXA_FIELD(u8)
    FOMOXA_CODEC("edge")
    uint8_t Byte = 0;

    FOMOXA_FIELD(i16)
    FOMOXA_CODEC("edge")
    int16_t Small = 0;

    FOMOXA_FIELD(u16)
    FOMOXA_CODEC("edge")
    uint16_t Port = 0;

    FOMOXA_FIELD(i32)
    FOMOXA_CODEC("edge")
    int32_t Offset = 0;

    FOMOXA_FIELD(u32)
    FOMOXA_CODEC("edge")
    uint32_t Count = 0;

    FOMOXA_FIELD(i64)
    FOMOXA_CODEC("edge")
    int64_t Delta = 0;

    FOMOXA_FIELD(u64)
    FOMOXA_CODEC("edge")
    uint64_t Sequence = 0;

    FOMOXA_FIELD(f32)
    FOMOXA_CODEC("edge")
    float Ratio = 0.0f;

    FOMOXA_FIELD(f64)
    FOMOXA_CODEC("edge")
    double Precise = 0.0;

    FOMOXA_FIELD(string)
    FOMOXA_CODEC("edge")
    std::string Label;

    FOMOXA_FIELD(bytes)
    FOMOXA_CODEC("edge")
    std::vector<uint8_t> Blob;
};

FOMOXA_MODEL
FOMOXA_CODEC("edge")
struct PlayerInfo
{
    FOMOXA_FIELD(u32)
    FOMOXA_CODEC("edge")
    uint32_t Level = 0;
};

FOMOXA_MODEL
FOMOXA_CODEC("edge")
struct Player
{
    FOMOXA_FIELD(u32)
    FOMOXA_CODEC("edge")
    uint32_t Id = 0;

    FOMOXA_FIELD(f32)
    FOMOXA_CODEC("edge")
    float X = 0.0f;

    FOMOXA_FIELD(f32)
    FOMOXA_CODEC("edge")
    float Y = 0.0f;
};

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

}
