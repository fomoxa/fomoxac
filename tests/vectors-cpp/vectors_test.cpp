#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <new>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "generated/device_state_edge.hpp"
#include "generated/device_state_unity.hpp"
#include "generated/every_primitive_edge.hpp"
#include "generated/net_schema.hpp"
#include "generated/player_edge.hpp"
#include "generated/player_info_edge.hpp"
#include "generated/team_edge.hpp"

using namespace generated;

namespace {

std::size_t allocations = 0;

}

void* operator new(std::size_t size) {
    ++allocations;
    if (void* memory = std::malloc(size == 0 ? 1 : size)) return memory;
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept { std::free(memory); }

void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }

namespace {

using Bytes = std::vector<std::uint8_t>;
using RoundTrip = std::function<DecodeError(const Bytes&, Bytes&)>;

Writer shared_writer(1);

template <typename Codec, typename Model>
RoundTrip round_trip() {
    return [](const Bytes& payload, Bytes& out) {
        Model value{};
        Reader reader(payload.data(), payload.size());
        DecodeError error = Codec::decode(reader, value);
        if (!error.ok()) return error;
        Writer writer;
        Codec::encode(writer, value);
        out = writer.bytes();
        return error;
    };
}

template <typename Codec, typename Model>
RoundTrip shared_round_trip() {
    return [](const Bytes& payload, Bytes& out) {
        Model value{};
        Reader reader(payload.data(), payload.size());
        DecodeError error = Codec::decode(reader, value);
        if (!error.ok()) return error;
        shared_writer.clear();
        Codec::encode(shared_writer, value);
        out = shared_writer.bytes();
        return error;
    };
}

std::size_t reused_allocations = 0;

template <typename Codec, typename Model>
RoundTrip reused_round_trip() {
    return [](const Bytes& payload, Bytes& out) {
        static Model held{};
        Reader reader(payload.data(), payload.size());
        DecodeError error = Codec::decode(reader, held);
        if (!error.ok()) return error;
        shared_writer.clear();
        Codec::encode(shared_writer, held);
        out = shared_writer.bytes();
        std::size_t before = allocations;
        Reader again(payload.data(), payload.size());
        error = Codec::decode(again, held);
        reused_allocations = allocations - before;
        return error;
    };
}

const std::map<std::string, RoundTrip> kReusedRoundTrips = {
    {"Player.edge", reused_round_trip<PlayerEdgeCodec, models::Player>()},
    {"DeviceState.edge", reused_round_trip<DeviceStateEdgeCodec, models::DeviceState>()},
    {"DeviceState.unity", reused_round_trip<DeviceStateUnityCodec, models::DeviceState>()},
    {"EveryPrimitive.edge", reused_round_trip<EveryPrimitiveEdgeCodec, models::EveryPrimitive>()},
    {"Team.edge", reused_round_trip<TeamEdgeCodec, models::Team>()},
};

const std::map<std::string, RoundTrip> kSharedRoundTrips = {
    {"Player.edge", shared_round_trip<PlayerEdgeCodec, models::Player>()},
    {"DeviceState.edge", shared_round_trip<DeviceStateEdgeCodec, models::DeviceState>()},
    {"DeviceState.unity", shared_round_trip<DeviceStateUnityCodec, models::DeviceState>()},
    {"EveryPrimitive.edge", shared_round_trip<EveryPrimitiveEdgeCodec, models::EveryPrimitive>()},
    {"Team.edge", shared_round_trip<TeamEdgeCodec, models::Team>()},
};

const std::map<std::string, RoundTrip> kRoundTrips = {
    {"Player.edge", round_trip<PlayerEdgeCodec, models::Player>()},
    {"DeviceState.edge", round_trip<DeviceStateEdgeCodec, models::DeviceState>()},
    {"DeviceState.unity", round_trip<DeviceStateUnityCodec, models::DeviceState>()},
    {"EveryPrimitive.edge", round_trip<EveryPrimitiveEdgeCodec, models::EveryPrimitive>()},
    {"Team.edge", round_trip<TeamEdgeCodec, models::Team>()},
};

struct Identity {
    std::uint32_t id;
    std::uint64_t fingerprint;
};

const std::map<std::string, Identity> kIdentities = {
    {"Player.edge", {PlayerEdgeCodec::kMessageId, PlayerEdgeCodec::kFingerprint}},
    {"PlayerInfo.edge", {PlayerInfoEdgeCodec::kMessageId, PlayerInfoEdgeCodec::kFingerprint}},
    {"DeviceState.edge", {DeviceStateEdgeCodec::kMessageId, DeviceStateEdgeCodec::kFingerprint}},
    {"DeviceState.unity", {DeviceStateUnityCodec::kMessageId, DeviceStateUnityCodec::kFingerprint}},
    {"EveryPrimitive.edge", {EveryPrimitiveEdgeCodec::kMessageId, EveryPrimitiveEdgeCodec::kFingerprint}},
    {"Team.edge", {TeamEdgeCodec::kMessageId, TeamEdgeCodec::kFingerprint}},
};

const std::map<std::string, DecodeError::Kind> kErrorKinds = {
    {"UnexpectedEof", DecodeError::Kind::UnexpectedEof},
    {"InvalidBool", DecodeError::Kind::InvalidBool},
    {"InvalidUtf8", DecodeError::Kind::InvalidUtf8},
    {"LengthOverflow", DecodeError::Kind::LengthOverflow},
};

int checks = 0;
int failures = 0;

void check(bool passed, const std::string& failure) {
    ++checks;
    if (!passed) {
        ++failures;
        std::printf("FAIL %s\n", failure.c_str());
    }
}

Bytes from_hex(const std::string& text) {
    Bytes out;
    if (text == "-") return out;
    for (std::size_t i = 0; i + 1 < text.size(); i += 2) {
        out.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(i, 2), nullptr, 16)));
    }
    return out;
}

std::string to_hex(const Bytes& bytes) {
    std::string out;
    char digits[3];
    for (std::uint8_t byte : bytes) {
        std::snprintf(digits, sizeof digits, "%02x", byte);
        out += digits;
    }
    return out;
}

}

void check_net_schema() {
    check(fmx_schema_check(&FOMOXA_NET_SCHEMA) == FMX_OK, "net schema: fmx_schema_check rejected it");
    check(FOMOXA_NET_SCHEMA.fingerprint == FOMOXA_SCHEMA_FINGERPRINT, "net schema: fingerprint differs from the handshake");
    check(FOMOXA_NET_SCHEMA.message_count == FOMOXA_MESSAGES.size(), "net schema: message count differs from the handshake");
    for (const FomoxaMessage& expected : FOMOXA_MESSAGES) {
        const fmx_message_schema* found = fmx_schema_message(&FOMOXA_NET_SCHEMA, expected.id);
        bool matches = found != nullptr && found->fingerprint == expected.fingerprint &&
                       found->prefix_count == expected.prefix_count &&
                       std::equal(expected.prefixes, expected.prefixes + expected.prefix_count, found->prefixes);
        check(matches, std::string("net schema: ") + expected.name + " does not match the handshake table");
    }
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream fields(line);
        std::string kind;
        std::string name;
        fields >> kind >> name;

        if (kind == "message") {
            std::string id_text;
            std::string fingerprint_text;
            fields >> id_text >> fingerprint_text;
            auto expected = kIdentities.find(name);
            if (expected == kIdentities.end()) continue;
            std::uint32_t id = static_cast<std::uint32_t>(std::stoul(id_text, nullptr, 16));
            std::uint64_t fingerprint = std::stoull(fingerprint_text, nullptr, 16);
            check(id == expected->second.id, name + ": message id mismatch");
            check(fingerprint == expected->second.fingerprint, name + ": fingerprint mismatch");
            continue;
        }

        std::string message;
        std::string payload_text;
        std::string last;
        fields >> message >> payload_text >> last;
        Bytes payload = from_hex(payload_text);
        Bytes actual;
        DecodeError error = kRoundTrips.at(message)(payload, actual);

        if (kind == "accept") {
            Bytes expected = from_hex(last);
            check(error.ok(), "accept " + name + ": " + error.message());
            check(!error.ok() || actual == expected,
                  "accept " + name + ": re-encoded " + to_hex(actual) + ", expected " + to_hex(expected));
            Bytes shared;
            DecodeError shared_error = kSharedRoundTrips.at(message)(payload, shared);
            check(shared_error.ok() && shared == expected,
                  "shared writer " + name + ": " + to_hex(shared) + ", expected " + to_hex(expected));
            Bytes reused;
            DecodeError reused_error = kReusedRoundTrips.at(message)(payload, reused);
            check(reused_error.ok() && reused == expected,
                  "reused target " + name + ": " + to_hex(reused) + ", expected " + to_hex(expected));
            check(reused_allocations == 0,
                  "reused target " + name + ": decoding the same bytes again allocated " + std::to_string(reused_allocations) + " times");
        } else if (kind == "reject") {
            check(!error.ok() && error.kind == kErrorKinds.at(last),
                  "reject " + name + ": " + (error.ok() ? std::string("decoded") : error.message()) + ", expected " + last);
        }
    }

    check_net_schema();

    std::printf("%d/%d checks passed\n", checks - failures, checks);
    return failures == 0 && checks > 0 ? 0 : 1;
}
