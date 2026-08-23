use std::collections::BTreeMap;

use crate::ir::Schema;
use crate::model::screaming_snake_case;
use crate::schema::hex64;

pub const FILE_NAME: &str = "handshake.h";

pub fn handshake_file(
    schema: &Schema,
    validate_message_fingerprint: bool,
) -> Result<String, String> {
    check_constant_names(schema)?;

    let mut out = super::Header {
        fingerprint: Some(schema.fingerprint.tagged()),
        note: Some(
            "Every fingerprint this schema publishes, and the handshake that compares\n\
             them. Generated - never edit, and never hand-maintain a copy of these\n\
             values anywhere else.",
        ),
        ..super::Header::default()
    }
    .render();
    out.push_str("#pragma once\n\n");
    out.push_str("#include <stdbool.h>\n#include <stddef.h>\n#include <stdint.h>\n");
    if validate_message_fingerprint {
        out.push_str("#include <stdio.h>\n\n#include \"runtime.h\"\n");
    }
    out.push('\n');

    out.push_str(
        "// The fingerprint of the whole schema: every message, by name, with its own\n\
         // fingerprint, hashed together. Two peers that agree on this agree on\n\
         // everything.\n",
    );
    out.push_str(&format!(
        "static const uint64_t FOMOXA_SCHEMA_FINGERPRINT = {}ULL;\n\n",
        hex64(schema.fingerprint.u64())
    ));

    for model in &schema.models {
        out.push_str(&format!(
            "// `{}`, as declared - every annotated field, whatever codec it joined.\n",
            model.name
        ));
        out.push_str(&format!(
            "static const uint64_t {}_FINGERPRINT = {}ULL;\n",
            screaming_snake_case(&model.name),
            hex64(model.fingerprint.u64())
        ));

        for message in &model.messages {
            out.push_str(&format!(
                "// `{}` - the wire contract `{}` encodes and decodes.\n",
                message.name,
                super::codec_type_name(&model.name, &message.codec)
            ));
            out.push_str(&format!(
                "static const uint32_t {}_MESSAGE_ID = 0x{:08X}u;\n",
                message_constant(&model.name, &message.codec),
                message.id
            ));
            out.push_str(&format!(
                "static const uint64_t {}_FINGERPRINT = {}ULL;\n",
                message_constant(&model.name, &message.codec),
                hex64(message.fingerprint.u64())
            ));
            let constant = message_constant(&model.name, &message.codec);
            out.push_str(&format!(
                "// One fingerprint per prefix of `{}`: entry `k-1` covers its first `k`\n\
                 // fields. The last entry is `{constant}_FINGERPRINT`. Never sent whole - a\n\
                 // peer sends its field count and its last entry, and the two sides compare\n\
                 // at `min` of the two counts (RFC-0002 9.1).\n",
                message.name
            ));
            if message.prefixes.is_empty() {
                out.push_str(&format!(
                    "static const uint64_t *const {constant}_PREFIXES = NULL;\n\
                     static const size_t {constant}_PREFIX_COUNT = 0;\n"
                ));
            } else {
                out.push_str(&format!(
                    "static const uint64_t {constant}_PREFIXES[] = {{\n"
                ));
                for prefix in &message.prefixes {
                    out.push_str(&format!("    {}ULL,\n", hex64(prefix.u64())));
                }
                out.push_str("};\n");
                out.push_str(&format!(
                    "static const size_t {constant}_PREFIX_COUNT = \
                     sizeof({constant}_PREFIXES) / sizeof({constant}_PREFIXES[0]);\n"
                ));
            }
        }
        out.push('\n');
    }

    out.push_str(TYPES);

    let mut messages: Vec<_> = schema.messages().collect();
    messages.sort_by_key(|message| message.id);

    out.push_str("// Every message this schema declares, sorted by id.\n");
    if messages.is_empty() {
        out.push_str("static const FomoxaMessage *const FOMOXA_MESSAGES = NULL;\n");
        out.push_str("static const size_t FOMOXA_MESSAGES_COUNT = 0;\n");
    } else {
        out.push_str("static const FomoxaMessage FOMOXA_MESSAGES[] = {\n");
        for message in &messages {
            let constant = message_constant(&message.model, &message.codec);
            out.push_str(&format!(
                "    {{{constant}_MESSAGE_ID, {:?}, {constant}_FINGERPRINT, \
                 {constant}_PREFIXES, {constant}_PREFIX_COUNT}},\n",
                message.name
            ));
        }
        out.push_str("};\n");
        out.push_str(
            "static const size_t FOMOXA_MESSAGES_COUNT = \
             sizeof(FOMOXA_MESSAGES) / sizeof(FOMOXA_MESSAGES[0]);\n",
        );
    }

    out.push_str(HANDSHAKE);

    out.push_str(&format!(
        "\n// Whether this schema was generated with `validate_message_fingerprint`.\n\
         static const bool FOMOXA_VALIDATE_MESSAGE_FINGERPRINT = {validate_message_fingerprint};\n"
    ));
    if validate_message_fingerprint {
        out.push_str(ENVELOPE);
    } else {
        out.push_str(ENVELOPE_OFF);
    }

    Ok(out)
}

fn message_constant(model: &str, codec: &str) -> String {
    format!(
        "{}_{}",
        screaming_snake_case(model),
        screaming_snake_case(codec)
    )
}

fn check_constant_names(schema: &Schema) -> Result<(), String> {
    let mut seen: BTreeMap<String, String> = BTreeMap::new();

    for model in &schema.models {
        let name = format!("{}_FINGERPRINT", screaming_snake_case(&model.name));
        seen.insert(name, model.name.clone());
    }
    for message in schema.messages() {
        let name = format!(
            "{}_FINGERPRINT",
            message_constant(&message.model, &message.codec)
        );
        if let Some(previous) = seen.insert(name.clone(), message.name.clone()) {
            return Err(format!(
                "'{previous}' and '{}' would both generate `{name}` - rename one of them",
                message.name
            ));
        }
    }

    Ok(())
}

const TYPES: &str = "\
// One message: its id, its name, and the fingerprint of its wire contract.
typedef struct FomoxaMessage {
    // Stable across schema changes; derived from the name alone.
    uint32_t id;
    // `Model.codec`.
    const char *name;
    // Changes whenever the message's fields do.
    uint64_t fingerprint;
    // One entry per field: entry `k-1` covers the first `k` fields. The last
    // entry is `fingerprint`. Stays local; only its length and its last entry
    // ever go on the wire. `NULL` when the message has no fields.
    const uint64_t *prefixes;
    size_t prefix_count;
} FomoxaMessage;

";

const HANDSHAKE: &str = "
// What a peer's fingerprints mean for this one.
typedef enum FomoxaHandshake {
    // The same schema, exactly.
    FOMOXA_HANDSHAKE_CURRENT,
    // A different schema, but every message both ends know agrees on the
    // fields both ends carry. Safe to proceed.
    FOMOXA_HANDSHAKE_OUTDATED,
    // Both ends put different fields at an index both of them carry. There is
    // nothing to negotiate: disconnect.
    FOMOXA_HANDSHAKE_REJECT,
    // Not decidable from the peer's table alone - at least one message needs
    // the extra exchange described on FOMOXA_MESSAGE_NEED_PREFIX.
    FOMOXA_HANDSHAKE_NEED_MORE,
} FomoxaHandshake;

// What one of the peer's messages means for this schema's message of the
// same id.
typedef enum FomoxaMessageCheck {
    // Either this schema does not declare the message at all, or the fields
    // both ends carry agree. Nothing to do.
    FOMOXA_MESSAGE_MATCH,
    // Both ends put different fields at an index both of them carry.
    FOMOXA_MESSAGE_REJECT,
    // Undecidable from what the peer sent: the peer has more fields than this
    // schema, so the answer lives at an index only the peer can produce. Ask
    // it for its prefix fingerprint at the reported field count, then compare
    // the reply against `fomoxa_prefix` for the same id.
    FOMOXA_MESSAGE_NEED_PREFIX,
} FomoxaMessageCheck;

// One entry of a peer's (id, field count, fingerprint) table - what
// `FOMOXA_MESSAGES` is on its side.
typedef struct FomoxaPeerMessage {
    uint32_t id;
    uint32_t field_count;
    uint64_t fingerprint;
} FomoxaPeerMessage;

// The message with this id, or `NULL` if this schema does not declare it.
static inline const FomoxaMessage *fomoxa_message(uint32_t id) {
    size_t low = 0;
    size_t high = FOMOXA_MESSAGES_COUNT;
    while (low < high) {
        size_t middle = (low + high) / 2;
        if (FOMOXA_MESSAGES[middle].id < id) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    if (low < FOMOXA_MESSAGES_COUNT && FOMOXA_MESSAGES[low].id == id) {
        return &FOMOXA_MESSAGES[low];
    }
    return NULL;
}

// This schema's fingerprint for the first `field_count` fields of a message.
// Writes it to `*out` and returns `true`, or returns `false` if this schema
// does not declare the message or does not have that many fields.
// `field_count` counts from 1; 0 is the empty prefix and has no fingerprint
// because it always matches.
static inline bool fomoxa_prefix(uint32_t id, uint32_t field_count, uint64_t *out) {
    const FomoxaMessage *message = fomoxa_message(id);
    if (message == NULL || field_count == 0 || (size_t)field_count > message->prefix_count) {
        return false;
    }
    *out = message->prefixes[field_count - 1];
    return true;
}

// Compares one of the peer's messages against this schema's.
//
// `peer_field_count` and `peer_fingerprint` are what the peer declares for
// this id. This is RFC-0002 9.1's prefix test: the two are compatible when the
// shorter field list is an exact prefix of the longer one, so the comparison
// happens at the smaller of the two field counts. `*ask_for` is the field
// count to ask the peer about, and is only meaningful for
// FOMOXA_MESSAGE_NEED_PREFIX.
static inline FomoxaMessageCheck fomoxa_check_message(uint32_t id, uint32_t peer_field_count,
                                                          uint64_t peer_fingerprint,
                                                          uint32_t *ask_for) {
    *ask_for = 0;
    const FomoxaMessage *known = fomoxa_message(id);
    if (known == NULL) {
        // Not a message this schema declares, so it is never exchanged.
        return FOMOXA_MESSAGE_MATCH;
    }
    uint32_t local_field_count = (uint32_t)known->prefix_count;

    if (peer_fingerprint == known->fingerprint) {
        return FOMOXA_MESSAGE_MATCH;
    }
    if (peer_field_count == 0 || local_field_count == 0) {
        // The empty field list is a prefix of everything.
        return FOMOXA_MESSAGE_MATCH;
    }
    if (peer_field_count == local_field_count) {
        // Same length, different content - a prefix of equal length would have
        // to be equality, and it is not.
        return FOMOXA_MESSAGE_REJECT;
    }
    if (peer_field_count < local_field_count) {
        // The peer's own fingerprint already is the value at the shared index.
        return known->prefixes[peer_field_count - 1] == peer_fingerprint
                   ? FOMOXA_MESSAGE_MATCH
                   : FOMOXA_MESSAGE_REJECT;
    }
    *ask_for = local_field_count;
    return FOMOXA_MESSAGE_NEED_PREFIX;
}

// Compares a peer's whole message table against this schema's.
//
// `peer_messages`/`peer_messages_count` is the peer's
// (id, field count, fingerprint) table - what
// `FOMOXA_MESSAGES`/`FOMOXA_MESSAGES_COUNT` is on its side. A
// FOMOXA_HANDSHAKE_NEED_MORE result means at least one message needs the
// extra round; walk the table with `fomoxa_check_message` to find which ones.
static inline FomoxaHandshake fomoxa_handshake(uint64_t peer_schema_fingerprint,
                                                   const FomoxaPeerMessage *peer_messages,
                                                   size_t peer_messages_count) {
    if (peer_schema_fingerprint == FOMOXA_SCHEMA_FINGERPRINT) {
        return FOMOXA_HANDSHAKE_CURRENT;
    }

    bool need_more = false;
    for (size_t i = 0; i < peer_messages_count; ++i) {
        uint32_t ask_for = 0;
        FomoxaMessageCheck check = fomoxa_check_message(
            peer_messages[i].id, peer_messages[i].field_count, peer_messages[i].fingerprint,
            &ask_for);
        if (check == FOMOXA_MESSAGE_REJECT) {
            // One mismatch decides the whole session. Every other message could
            // agree and it would still be unsafe to speak.
            return FOMOXA_HANDSHAKE_REJECT;
        }
        if (check == FOMOXA_MESSAGE_NEED_PREFIX) {
            need_more = true;
        }
    }

    return need_more ? FOMOXA_HANDSHAKE_NEED_MORE : FOMOXA_HANDSHAKE_OUTDATED;
}
";

const ENVELOPE: &str = "
// ==========================================================================
// Per-frame validation - validate_message_fingerprint = true.
//
//     [MessageId: u32][MessageFingerprint: u64][Payload]
//
// Twelve bytes in front of every message, so that a peer that got past the
// handshake still cannot decode one message as another. Off by default: the
// wire format's premise is that there is no metadata on it, and the
// handshake already answers this question once per connection instead of
// once per frame.
// ==========================================================================

// A frame whose envelope did not describe a message this schema can decode.
typedef enum FomoxaEnvelopeErrorKind {
    // The envelope itself could not be read.
    FOMOXA_ENVELOPE_MALFORMED,
    // An id this schema does not declare.
    FOMOXA_ENVELOPE_UNKNOWN_MESSAGE,
    // The right message, the wrong shape.
    FOMOXA_ENVELOPE_FINGERPRINT_MISMATCH,
} FomoxaEnvelopeErrorKind;

typedef struct FomoxaEnvelopeError {
    FomoxaEnvelopeErrorKind kind;
    // FOMOXA_ENVELOPE_MALFORMED.
    FomoxaDecodeError malformed;
    // FOMOXA_ENVELOPE_UNKNOWN_MESSAGE / FOMOXA_ENVELOPE_FINGERPRINT_MISMATCH.
    uint32_t id;
    // FOMOXA_ENVELOPE_FINGERPRINT_MISMATCH.
    uint64_t expected;
    // FOMOXA_ENVELOPE_FINGERPRINT_MISMATCH.
    uint64_t received;
} FomoxaEnvelopeError;

static inline void fomoxa_envelope_error_message(const FomoxaEnvelopeError *error, char *buffer,
                                                    size_t size) {
    switch (error->kind) {
        case FOMOXA_ENVELOPE_MALFORMED: {
            char inner[160];
            fomoxa_decode_error_message(&error->malformed, inner, sizeof(inner));
            snprintf(buffer, size, \"malformed envelope: %s\", inner);
            return;
        }
        case FOMOXA_ENVELOPE_UNKNOWN_MESSAGE:
            snprintf(buffer, size, \"unknown message id 0x%08X\", (unsigned)error->id);
            return;
        case FOMOXA_ENVELOPE_FINGERPRINT_MISMATCH:
            snprintf(buffer, size, \"message 0x%08X: peer fingerprint 0x%016llX, ours 0x%016llX\",
                      (unsigned)error->id, (unsigned long long)error->received,
                      (unsigned long long)error->expected);
            return;
    }
    snprintf(buffer, size, \"unknown envelope error\");
}

// Writes [MessageId][MessageFingerprint], immediately before the payload.
// Returns `false` if the writer's buffer failed to grow.
static inline bool fomoxa_write_envelope(FomoxaWriter *writer, const FomoxaMessage *message) {
    if (!fomoxa_writer_write_u32(writer, message->id)) return false;
    if (!fomoxa_writer_write_u64(writer, message->fingerprint)) return false;
    return true;
}

// Reads an envelope and resolves it against this schema. On success, `*out`
// is left pointing at the resolved message and the reader is positioned at
// the payload; on failure `*error` describes what went wrong and `*out` is
// untouched.
static inline bool fomoxa_read_envelope(FomoxaReader *reader, const FomoxaMessage **out,
                                          FomoxaEnvelopeError *error) {
    uint32_t id = 0;
    FomoxaDecodeError decode_error = fomoxa_reader_read_u32(reader, &id);
    if (!fomoxa_decode_error_ok(&decode_error)) {
        error->kind = FOMOXA_ENVELOPE_MALFORMED;
        error->malformed = decode_error;
        return false;
    }
    uint64_t fingerprint = 0;
    decode_error = fomoxa_reader_read_u64(reader, &fingerprint);
    if (!fomoxa_decode_error_ok(&decode_error)) {
        error->kind = FOMOXA_ENVELOPE_MALFORMED;
        error->malformed = decode_error;
        return false;
    }

    const FomoxaMessage *message = fomoxa_message(id);
    if (message == NULL) {
        error->kind = FOMOXA_ENVELOPE_UNKNOWN_MESSAGE;
        error->id = id;
        return false;
    }
    if (message->fingerprint != fingerprint) {
        error->kind = FOMOXA_ENVELOPE_FINGERPRINT_MISMATCH;
        error->id = id;
        error->expected = message->fingerprint;
        error->received = fingerprint;
        return false;
    }

    *out = message;
    return true;
}
";

const ENVELOPE_OFF: &str = "\
// Per-frame message validation is off, so no envelope is generated and no
// frame carries one. Turn it on in fomoxa.toml:
//
//     validate_message_fingerprint = true
//
// and every frame gains [MessageId: u32][MessageFingerprint: u64] in front of
// its payload, with fomoxa_write_envelope / fomoxa_read_envelope to match.
";

#[cfg(test)]
mod tests {
    use std::path::PathBuf;

    use super::handshake_file;
    use crate::ir::Schema;
    use crate::model::{Field, Model};

    fn model(name: &str, codecs: &[&str]) -> Model {
        Model {
            name: name.to_owned(),
            source: PathBuf::from("models.h"),
            line: 1,
            codecs: codecs.iter().map(|codec| (*codec).to_owned()).collect(),
            fields: vec![Field {
                name: "id".to_owned(),
                network_type: "u32".to_owned(),
                codecs: codecs.iter().map(|codec| (*codec).to_owned()).collect(),
                line: 2,
            }],
        }
    }

    fn generated(models: &[Model]) -> String {
        handshake_file(&Schema::build(models).expect("build"), false).expect("render")
    }

    #[test]
    fn every_constant_the_brief_asks_for_is_generated() {
        let text = generated(&[model("Player", &["edge"]), model("Enemy", &["edge"])]);

        assert!(
            text.contains("static const uint64_t FOMOXA_SCHEMA_FINGERPRINT = 0x"),
            "{text}"
        );
        assert!(
            text.contains("static const uint64_t PLAYER_FINGERPRINT = 0x"),
            "{text}"
        );
        assert!(
            text.contains("static const uint64_t ENEMY_FINGERPRINT = 0x"),
            "{text}"
        );
        assert!(
            text.contains("static const uint64_t PLAYER_EDGE_FINGERPRINT = 0x"),
            "{text}"
        );
        assert!(
            text.contains("static const uint32_t PLAYER_EDGE_MESSAGE_ID = 0x"),
            "{text}"
        );
        assert!(
            text.contains("static const FomoxaMessage FOMOXA_MESSAGES[] = {"),
            "{text}"
        );
    }

    #[test]
    fn an_empty_schema_declares_no_messages_but_still_compiles() {
        let text = handshake_file(&Schema::build(&[]).expect("build"), false).expect("render");
        assert!(
            text.contains("static const FomoxaMessage *const FOMOXA_MESSAGES = NULL;"),
            "{text}"
        );
        assert!(
            text.contains("static const size_t FOMOXA_MESSAGES_COUNT = 0;"),
            "{text}"
        );
    }

    #[test]
    fn the_message_table_is_sorted_by_id_so_lookup_can_bisect() {
        let text = generated(&[
            model("Player", &["edge", "unity"]),
            model("Enemy", &["edge"]),
        ]);

        let lines: Vec<&str> = text
            .lines()
            .filter(|line| line.trim_start().starts_with('{'))
            .collect();
        assert_eq!(lines.len(), 3, "{text}");

        let schema = Schema::build(&[
            model("Player", &["edge", "unity"]),
            model("Enemy", &["edge"]),
        ])
        .expect("build");
        let mut sorted: Vec<u32> = schema.messages().map(|message| message.id).collect();
        sorted.sort_unstable();
        for (line, id) in lines.iter().zip(sorted) {
            let name = schema
                .messages()
                .find(|message| message.id == id)
                .expect("message")
                .name
                .clone();
            assert!(line.contains(&format!("{name:?}")), "{line} / {name}");
        }
    }

    #[test]
    fn the_envelope_is_off_unless_asked_for() {
        let off = generated(&[model("Player", &["edge"])]);
        assert!(
            off.contains("FOMOXA_VALIDATE_MESSAGE_FINGERPRINT = false;"),
            "{off}"
        );
        assert!(
            !off.contains("static inline bool fomoxa_write_envelope"),
            "{off}"
        );

        let schema = Schema::build(&[model("Player", &["edge"])]).expect("build");
        let on = handshake_file(&schema, true).expect("render");
        assert!(
            on.contains("FOMOXA_VALIDATE_MESSAGE_FINGERPRINT = true;"),
            "{on}"
        );
        assert!(
            on.contains("static inline bool fomoxa_write_envelope"),
            "{on}"
        );
        assert!(
            on.contains("static inline bool fomoxa_read_envelope"),
            "{on}"
        );
        assert!(on.contains("#include \"runtime.h\"\n"), "{on}");
    }

    #[test]
    fn two_constants_spelled_the_same_are_reported_not_emitted() {
        let schema =
            Schema::build(&[model("Player", &["edge"]), model("PlayerEdge", &[])]).expect("build");
        let error = handshake_file(&schema, false).expect_err("collision");
        assert!(error.contains("PLAYER_EDGE_FINGERPRINT"), "{error}");
    }
}
