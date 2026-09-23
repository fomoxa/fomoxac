#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/generated/device_state_edge.h"
#include "src/generated/device_state_fomoxa.h"
#include "src/generated/device_state_unity.h"
#include "src/generated/every_primitive_edge.h"
#include "src/generated/every_primitive_fomoxa.h"
#include "src/generated/player_edge.h"
#include "src/generated/player_fomoxa.h"
#include "src/generated/net_schema.h"
#include "src/generated/player_info_edge.h"
#include "src/generated/team_edge.h"
#include "src/generated/team_fomoxa.h"

#define LINE_CAPACITY 4096
#define BYTES_CAPACITY 2048

typedef FomoxaDecodeError (*RoundTrip)(const unsigned char *payload, size_t size, FomoxaWriter *out);

#define DEFINE_ROUND_TRIP(function, Model, Codec)                                        \
    static FomoxaDecodeError function(const unsigned char *payload, size_t size,         \
                                      FomoxaWriter *out) {                               \
        struct Model value;                                                               \
        FomoxaReader reader;                                                              \
        FomoxaDecodeError error;                                                          \
        memset(&value, 0, sizeof value);                                                  \
        fomoxa_reader_init(&reader, payload, size, fomoxa_limits_unlimited());            \
        error = Codec##_decode(&reader, &value);                                          \
        if (fomoxa_decode_error_ok(&error)) {                                                    \
            Codec##_encode(out, &value);                                                  \
        }                                                                                 \
        Model##_free(&value);                                                             \
        return error;                                                                     \
    }

DEFINE_ROUND_TRIP(player_edge, Player, PlayerEdgeCodec)
DEFINE_ROUND_TRIP(device_state_edge, DeviceState, DeviceStateEdgeCodec)
DEFINE_ROUND_TRIP(device_state_unity, DeviceState, DeviceStateUnityCodec)
DEFINE_ROUND_TRIP(every_primitive_edge, EveryPrimitive, EveryPrimitiveEdgeCodec)
DEFINE_ROUND_TRIP(team_edge, Team, TeamEdgeCodec)

static struct Player held_player;
static struct DeviceState held_device_state_edge;
static struct DeviceState held_device_state_unity;
static struct EveryPrimitive held_every_primitive;
static struct Team held_team;

#define DEFINE_REUSED_ROUND_TRIP(function, Codec, held)                                  \
    static FomoxaDecodeError function(const unsigned char *payload, size_t size,         \
                                      FomoxaWriter *out) {                               \
        FomoxaReader reader;                                                              \
        FomoxaDecodeError error;                                                          \
        fomoxa_reader_init(&reader, payload, size, fomoxa_limits_unlimited());            \
        error = Codec##_decode(&reader, &held);                                           \
        if (fomoxa_decode_error_ok(&error)) {                                             \
            Codec##_encode(out, &held);                                                   \
        }                                                                                 \
        return error;                                                                     \
    }

DEFINE_REUSED_ROUND_TRIP(reused_player_edge, PlayerEdgeCodec, held_player)
DEFINE_REUSED_ROUND_TRIP(reused_device_state_edge, DeviceStateEdgeCodec, held_device_state_edge)
DEFINE_REUSED_ROUND_TRIP(reused_device_state_unity, DeviceStateUnityCodec, held_device_state_unity)
DEFINE_REUSED_ROUND_TRIP(reused_every_primitive_edge, EveryPrimitiveEdgeCodec, held_every_primitive)
DEFINE_REUSED_ROUND_TRIP(reused_team_edge, TeamEdgeCodec, held_team)

#define HELD_POINTERS 64

static size_t held_pointers(const void **out) {
    size_t count = 0;
    size_t i;
    out[count++] = held_device_state_unity.DisplayName;
    out[count++] = held_every_primitive.Label;
    out[count++] = held_every_primitive.Blob.data;
    out[count++] = held_team.Tags.items;
    out[count++] = held_team.Scores.items;
    out[count++] = held_team.Roster.items;
    for (i = 0; i < held_team.Tags.count && count < HELD_POINTERS; ++i) {
        out[count++] = held_team.Tags.items[i];
    }
    return count;
}

typedef struct {
    const char *message;
    RoundTrip round_trip;
} RoundTripEntry;

static const RoundTripEntry round_trips[] = {
    {"Player.edge", player_edge},
    {"DeviceState.edge", device_state_edge},
    {"DeviceState.unity", device_state_unity},
    {"EveryPrimitive.edge", every_primitive_edge},
    {"Team.edge", team_edge},
};

typedef struct {
    const char *name;
    FomoxaDecodeErrorKind kind;
} ErrorEntry;

static const ErrorEntry error_kinds[] = {
    {"UnexpectedEof", FOMOXA_DECODE_UNEXPECTED_EOF},
    {"InvalidBool", FOMOXA_DECODE_INVALID_BOOL},
    {"InvalidUtf8", FOMOXA_DECODE_INVALID_UTF8},
    {"LengthOverflow", FOMOXA_DECODE_LENGTH_OVERFLOW},
};

static int checks = 0;
static int failures = 0;

static void check(int passed, const char *kind, const char *name, const char *detail) {
    ++checks;
    if (!passed) {
        ++failures;
        printf("FAIL %s %s: %s\n", kind, name, detail);
    }
}

static size_t from_hex(const char *text, unsigned char *out) {
    size_t count = 0;
    size_t i;
    if (strcmp(text, "-") == 0) return 0;
    for (i = 0; text[i] != '\0' && text[i + 1] != '\0' && count < BYTES_CAPACITY; i += 2) {
        char pair[3] = {text[i], text[i + 1], '\0'};
        out[count++] = (unsigned char)strtoul(pair, NULL, 16);
    }
    return count;
}

static const RoundTripEntry reused_round_trips[] = {
    {"Player.edge", reused_player_edge},
    {"DeviceState.edge", reused_device_state_edge},
    {"DeviceState.unity", reused_device_state_unity},
    {"EveryPrimitive.edge", reused_every_primitive_edge},
    {"Team.edge", reused_team_edge},
};

static RoundTrip find_reused_round_trip(const char *message) {
    size_t i;
    for (i = 0; i < sizeof reused_round_trips / sizeof reused_round_trips[0]; ++i) {
        if (strcmp(reused_round_trips[i].message, message) == 0) return reused_round_trips[i].round_trip;
    }
    return NULL;
}

static RoundTrip find_round_trip(const char *message) {
    size_t i;
    for (i = 0; i < sizeof round_trips / sizeof round_trips[0]; ++i) {
        if (strcmp(round_trips[i].message, message) == 0) return round_trips[i].round_trip;
    }
    return NULL;
}

static int find_identity(const char *message, uint32_t *id, uint64_t *fingerprint) {
    if (strcmp(message, "Player.edge") == 0) {
        *id = PlayerEdgeCodec_MESSAGE_ID;
        *fingerprint = PlayerEdgeCodec_FINGERPRINT;
    } else if (strcmp(message, "PlayerInfo.edge") == 0) {
        *id = PlayerInfoEdgeCodec_MESSAGE_ID;
        *fingerprint = PlayerInfoEdgeCodec_FINGERPRINT;
    } else if (strcmp(message, "DeviceState.edge") == 0) {
        *id = DeviceStateEdgeCodec_MESSAGE_ID;
        *fingerprint = DeviceStateEdgeCodec_FINGERPRINT;
    } else if (strcmp(message, "DeviceState.unity") == 0) {
        *id = DeviceStateUnityCodec_MESSAGE_ID;
        *fingerprint = DeviceStateUnityCodec_FINGERPRINT;
    } else if (strcmp(message, "EveryPrimitive.edge") == 0) {
        *id = EveryPrimitiveEdgeCodec_MESSAGE_ID;
        *fingerprint = EveryPrimitiveEdgeCodec_FINGERPRINT;
    } else if (strcmp(message, "Team.edge") == 0) {
        *id = TeamEdgeCodec_MESSAGE_ID;
        *fingerprint = TeamEdgeCodec_FINGERPRINT;
    } else {
        return 0;
    }
    return 1;
}

static int error_kind(const char *name, FomoxaDecodeErrorKind *kind) {
    size_t i;
    for (i = 0; i < sizeof error_kinds / sizeof error_kinds[0]; ++i) {
        if (strcmp(error_kinds[i].name, name) == 0) {
            *kind = error_kinds[i].kind;
            return 1;
        }
    }
    return 0;
}

static void check_net_schema(void) {
    size_t index;

    check(fmx_schema_check(&FOMOXA_NET_SCHEMA) == FMX_OK, "net schema", "-", "fmx_schema_check rejected it");
    check(FOMOXA_NET_SCHEMA.fingerprint == FOMOXA_SCHEMA_FINGERPRINT, "net schema", "-", "fingerprint differs from the handshake");
    check(FOMOXA_NET_SCHEMA.message_count == FOMOXA_MESSAGES_COUNT, "net schema", "-", "message count differs from the handshake");
    for (index = 0; index < FOMOXA_MESSAGES_COUNT; ++index) {
        const FomoxaMessage *expected = &FOMOXA_MESSAGES[index];
        const fmx_message_schema *found = fmx_schema_message(&FOMOXA_NET_SCHEMA, expected->id);
        int matches = found != NULL && found->fingerprint == expected->fingerprint &&
                      found->prefix_count == expected->prefix_count &&
                      (expected->prefix_count == 0 ||
                       memcmp(found->prefixes, expected->prefixes, expected->prefix_count * sizeof(uint64_t)) == 0);
        check(matches, "net schema", expected->name, "does not match the handshake table");
    }
}

int main(void) {
    char line[LINE_CAPACITY];
    static unsigned char payload[BYTES_CAPACITY];
    static unsigned char expected[BYTES_CAPACITY];
    FomoxaWriter shared_writer;
    fomoxa_writer_init(&shared_writer);

    while (fgets(line, sizeof line, stdin) != NULL) {
        char kind[16], name[128], third[128], fourth[BYTES_CAPACITY], fifth[BYTES_CAPACITY];
        int fields = sscanf(line, "%15s %127s %127s %2047s %2047s", kind, name, third, fourth, fifth);

        if (strcmp(kind, "message") == 0 && fields >= 4) {
            uint32_t expected_id;
            uint64_t expected_fingerprint;
            if (!find_identity(name, &expected_id, &expected_fingerprint)) continue;
            check((uint32_t)strtoul(third, NULL, 16) == expected_id, "message", name, "message id mismatch");
            check(strtoull(fourth, NULL, 16) == expected_fingerprint, "message", name, "fingerprint mismatch");
            continue;
        }

        if (fields == 5 && (strcmp(kind, "accept") == 0 || strcmp(kind, "reject") == 0)) {
            RoundTrip round_trip = find_round_trip(third);
            size_t payload_size = from_hex(fourth, payload);
            FomoxaWriter writer;
            FomoxaDecodeError error;
            fomoxa_writer_init(&writer);
            error = round_trip(payload, payload_size, &writer);

            if (strcmp(kind, "accept") == 0) {
                size_t expected_size = from_hex(fifth, expected);
                FomoxaDecodeError shared_error;
                check(fomoxa_decode_error_ok(&error), kind, name, "decode failed");
                check(!fomoxa_decode_error_ok(&error) ||
                          (writer.len == expected_size &&
                           (expected_size == 0 || memcmp(writer.data, expected, expected_size) == 0)),
                      kind, name, "re-encoded bytes differ");
                fomoxa_writer_reset(&shared_writer);
                shared_error = round_trip(payload, payload_size, &shared_writer);
                check(fomoxa_decode_error_ok(&shared_error) && shared_writer.len == expected_size &&
                          (expected_size == 0 || memcmp(shared_writer.data, expected, expected_size) == 0),
                      "shared writer", name, "re-encoded bytes differ");
                {
                    RoundTrip reused = find_reused_round_trip(third);
                    FomoxaWriter reused_writer;
                    FomoxaDecodeError reused_error;
                    const void *before[HELD_POINTERS];
                    const void *after[HELD_POINTERS];
                    size_t before_count;
                    size_t after_count;
                    fomoxa_writer_init(&reused_writer);
                    reused_error = reused(payload, payload_size, &reused_writer);
                    check(fomoxa_decode_error_ok(&reused_error) && reused_writer.len == expected_size &&
                              (expected_size == 0 || memcmp(reused_writer.data, expected, expected_size) == 0),
                          "reused target", name, "re-encoded bytes differ");
                    before_count = held_pointers(before);
                    fomoxa_writer_reset(&reused_writer);
                    reused_error = reused(payload, payload_size, &reused_writer);
                    after_count = held_pointers(after);
                    check(fomoxa_decode_error_ok(&reused_error) && before_count == after_count &&
                              memcmp(before, after, before_count * sizeof before[0]) == 0,
                          "reused target", name, "decoding the same bytes again moved a held buffer");
                    fomoxa_writer_free(&reused_writer);
                }
            } else {
                FomoxaDecodeErrorKind wanted;
                FomoxaWriter reused_writer;
                fomoxa_writer_init(&reused_writer);
                find_reused_round_trip(third)(payload, payload_size, &reused_writer);
                fomoxa_writer_free(&reused_writer);
                int known = error_kind(fifth, &wanted);
                check(known && !fomoxa_decode_error_ok(&error) && error.kind == wanted, kind, name, fifth);
            }
            fomoxa_writer_free(&writer);
        }
    }

    fomoxa_writer_free(&shared_writer);
    Player_free(&held_player);
    DeviceState_free(&held_device_state_edge);
    DeviceState_free(&held_device_state_unity);
    EveryPrimitive_free(&held_every_primitive);
    Team_free(&held_team);
    check_net_schema();

    printf("%d/%d checks passed\n", checks - failures, checks);
    return failures == 0 && checks > 0 ? 0 : 1;
}
