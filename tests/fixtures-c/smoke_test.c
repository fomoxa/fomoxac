/* Not part of the generated tree or the fixture fomoxac reads - a
 * throwaway smoke test compiled by hand (see README's C section) to prove
 * the generated headers actually build and round-trip, since no C toolchain
 * step runs inside `cargo test` itself. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "src/generated/handshake.h"
#include "src/generated/player_fomoxa.h"
#include "src/generated/player_edge.h"
#include "src/generated/player_info_fomoxa.h"
#include "src/generated/player_info_edge.h"
#include "src/generated/player_unity.h"
#include "src/generated/team_fomoxa.h"
#include "src/generated/team_edge.h"

int main(void) {
    struct Team team;
    memset(&team, 0, sizeof(team));
    team.Captain.Level = 7;

    const char *tags[2];
    tags[0] = "red";
    tags[1] = "blue";
    team.Tags.items = tags;
    team.Tags.count = 2;

    uint32_t scores[3];
    scores[0] = 10;
    scores[1] = 20;
    scores[2] = 30;
    team.Scores.items = scores;
    team.Scores.count = 3;

    struct PlayerInfo roster[1];
    roster[0].Level = 3;
    team.Roster.items = roster;
    team.Roster.count = 1;

    /* None of `team`'s array fields are heap-owned here (string literals,
     * stack arrays) - it is never passed to Team_free. Only what
     * fomoxac's own generated decode actually `malloc`s gets freed below. */

    FomoxaWriter writer;
    fomoxa_writer_init(&writer);
    assert(TeamEdgeCodec_encode(&writer, &team));

    FomoxaReader reader;
    fomoxa_reader_init(&reader, writer.data, writer.len, fomoxa_limits_unlimited());
    struct Team decoded;
    memset(&decoded, 0, sizeof(decoded));
    FomoxaDecodeError error = TeamEdgeCodec_decode(&reader, &decoded);
    assert(fomoxa_decode_error_ok(&error));
    assert(decoded.Captain.Level == 7);
    assert(decoded.Tags.count == 2);
    assert(strcmp(decoded.Tags.items[0], "red") == 0);
    assert(strcmp(decoded.Tags.items[1], "blue") == 0);
    assert(decoded.Scores.count == 3 && decoded.Scores.items[1] == 20);
    assert(decoded.Roster.count == 1 && decoded.Roster.items[0].Level == 3);

    Team_free(&decoded);
    fomoxa_writer_free(&writer);

    /* RFC-0002 section 9.1: a payload from an older writer that only wrote
     * Id still decodes, with X/Y/Name/Payload taking their zero value. */
    FomoxaWriter id_only;
    fomoxa_writer_init(&id_only);
    assert(fomoxa_writer_write_u32(&id_only, 42));

    FomoxaReader skew_reader;
    fomoxa_reader_init(&skew_reader, id_only.data, id_only.len, fomoxa_limits_unlimited());
    struct Player decoded_player;
    memset(&decoded_player, 0, sizeof(decoded_player));
    decoded_player.X = 99.0f; /* proves the decoder actually zeroes it */
    FomoxaDecodeError player_error = PlayerEdgeCodec_decode(&skew_reader, &decoded_player);
    assert(fomoxa_decode_error_ok(&player_error));
    assert(decoded_player.Id == 42);
    assert(decoded_player.X == 0.0f);
    assert(decoded_player.Y == 0.0f);
    assert(decoded_player.Name == NULL);
    assert(decoded_player.Payload.data == NULL && decoded_player.Payload.len == 0);

    Player_free(&decoded_player);
    fomoxa_writer_free(&id_only);

    /* A truncated field (not absent - the stream ends *inside* X) is an
     * error, not a zero. */
    FomoxaWriter truncated;
    fomoxa_writer_init(&truncated);
    assert(fomoxa_writer_write_u32(&truncated, 1));
    assert(fomoxa_writer_write_u8(&truncated, 0)); /* one byte of a 4-byte f32 */

    FomoxaReader truncated_reader;
    fomoxa_reader_init(&truncated_reader, truncated.data, truncated.len,
                         fomoxa_limits_unlimited());
    struct Player truncated_player;
    memset(&truncated_player, 0, sizeof(truncated_player));
    FomoxaDecodeError truncated_error =
        PlayerEdgeCodec_decode(&truncated_reader, &truncated_player);
    assert(!fomoxa_decode_error_ok(&truncated_error));
    assert(truncated_error.kind == FOMOXA_DECODE_UNEXPECTED_EOF);
    /* Nothing was allocated before the truncated field failed - no free
     * needed for `truncated_player`. */
    fomoxa_writer_free(&truncated);

    /* The handshake: identical schema is Current, and a peer that only knows
     * an older subset of messages with matching fingerprints is Outdated -
     * one with a mismatched fingerprint for a message both sides know is
     * Reject. */
    FomoxaHandshake current = fomoxa_handshake(FOMOXA_SCHEMA_FINGERPRINT, NULL, 0);
    assert(current == FOMOXA_HANDSHAKE_CURRENT);

    FomoxaPeerMessage outdated_peer[1];
    outdated_peer[0].id = PLAYER_EDGE_MESSAGE_ID;
    outdated_peer[0].field_count = (uint32_t)PLAYER_EDGE_PREFIX_COUNT;
    outdated_peer[0].fingerprint = PLAYER_EDGE_FINGERPRINT;
    FomoxaHandshake outdated =
        fomoxa_handshake(FOMOXA_SCHEMA_FINGERPRINT + 1, outdated_peer, 1);
    assert(outdated == FOMOXA_HANDSHAKE_OUTDATED);

    FomoxaPeerMessage reject_peer[1];
    reject_peer[0].id = PLAYER_EDGE_MESSAGE_ID;
    reject_peer[0].field_count = (uint32_t)PLAYER_EDGE_PREFIX_COUNT;
    reject_peer[0].fingerprint = PLAYER_EDGE_FINGERPRINT + 1;
    FomoxaHandshake reject = fomoxa_handshake(FOMOXA_SCHEMA_FINGERPRINT + 1, reject_peer, 1);
    assert(reject == FOMOXA_HANDSHAKE_REJECT);

    puts("c fixture smoke test: ok");
    return 0;
}
