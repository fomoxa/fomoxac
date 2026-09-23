use crate::ir::{Message, Schema};
use crate::model::screaming_snake_case;
use crate::schema::hex64;

pub const RUST_FILE_NAME: &str = "net_schema.rs";
pub const RUST_MODULE: &str = "net_schema";
pub const GO_FILE_NAME: &str = "net_schema.go";
pub const CSHARP_FILE_NAME: &str = "NetSchema.cs";
pub const CPP_FILE_NAME: &str = "net_schema.hpp";
pub const C_FILE_NAME: &str = "net_schema.h";
pub const TYPESCRIPT_FILE_NAME: &str = "net_schema.ts";
pub const JAVASCRIPT_FILE_NAME: &str = "net_schema.js";

pub const FILE_NAMES: &[&str] = &[
    RUST_FILE_NAME,
    GO_FILE_NAME,
    CSHARP_FILE_NAME,
    CPP_FILE_NAME,
    C_FILE_NAME,
    TYPESCRIPT_FILE_NAME,
    JAVASCRIPT_FILE_NAME,
];

pub const KIND: &str = "net_schema";

const MAX_HANDSHAKE_PAYLOAD: usize = 1024 * 1024;
const HELLO_HEADER_SIZE: usize = 16;
const HELLO_ENTRY_SIZE: usize = 14;
const MAX_FIELD_COUNT: usize = u16::MAX as usize;

pub fn check(schema: &Schema) -> Result<(), String> {
    let count = schema.messages().count();
    let hello = HELLO_HEADER_SIZE + HELLO_ENTRY_SIZE * count;
    if hello > MAX_HANDSHAKE_PAYLOAD {
        return Err(format!(
            "net_schema: {count} messages need a {hello}-byte hello, past the \
             {MAX_HANDSHAKE_PAYLOAD} bytes a handshake body may carry"
        ));
    }
    for message in schema.messages() {
        if message.prefixes.len() > MAX_FIELD_COUNT {
            return Err(format!(
                "net_schema: '{}' has {} fields, more than the {MAX_FIELD_COUNT} a hello can carry",
                message.name,
                message.prefixes.len()
            ));
        }
    }
    Ok(())
}

fn header(schema: &Schema, sdk: &str, entry: &str) -> String {
    let note = format!(
        "The {sdk} schema this tree declares: every message in the handshake\n\
         table, under the schema fingerprint. Pass {entry}\n\
         to a connection or a server instead of listing messages by hand.\n\
         Written because fomoxa.toml sets `net_schema = true`."
    );
    super::Header {
        fingerprint: Some(schema.fingerprint.tagged()),
        note: Some(&note),
        ..super::Header::default()
    }
    .render()
}

fn sorted_messages(schema: &Schema) -> Vec<&Message> {
    let mut messages: Vec<&Message> = schema.messages().collect();
    messages.sort_by_key(|message| message.id);
    messages
}

pub fn rust(schema: &Schema) -> Result<String, String> {
    check(schema)?;
    let mut out = header(schema, "fomoxa-net", "`fomoxa_net_schema()`");
    out.push_str(super::FILE_ATTRIBUTES);
    out.push_str(
        "use super::handshake::{FOMOXA_MESSAGES, FOMOXA_SCHEMA_FINGERPRINT};\n\
         \n\
         pub fn fomoxa_net_schema() -> Result<::fomoxa_net::Schema, ::fomoxa_net::SchemaError> {\n\
         \x20   ::fomoxa_net::Schema::new(\n\
         \x20       FOMOXA_SCHEMA_FINGERPRINT,\n\
         \x20       FOMOXA_MESSAGES\n\
         \x20           .iter()\n\
         \x20           .map(|message| {\n\
         \x20               ::fomoxa_net::MessageSchema::new(message.id, message.fingerprint, message.prefixes)\n\
         \x20           })\n\
         \x20           .collect::<Vec<_>>(),\n\
         \x20   )\n\
         }\n",
    );
    Ok(out)
}

pub fn go(schema: &Schema, package: &str) -> Result<String, String> {
    check(schema)?;
    let mut out = header(schema, "github.com/fomoxa/go", "FomoxaNetSchema()");
    out.push_str(&format!("package {package}\n\n"));
    out.push_str(
        "import fomoxa \"github.com/fomoxa/go\"\n\
         \n\
         func FomoxaNetSchema() (*fomoxa.Schema, error) {\n\
         \tmessages := make([]fomoxa.Message, len(FomoxaMessages))\n\
         \tfor index, message := range FomoxaMessages {\n\
         \t\tmessages[index] = fomoxa.Message{ID: message.ID, Fingerprint: message.Fingerprint, Prefixes: message.Prefixes}\n\
         \t}\n\
         \treturn fomoxa.NewSchema(FomoxaSchemaFingerprint, messages)\n\
         }\n",
    );
    Ok(out)
}

pub fn csharp(schema: &Schema, namespace: &str) -> Result<String, String> {
    check(schema)?;
    let mut out = header(schema, "Fomoxa.Net", "NetSchema.Build()");
    out.push_str(&format!("namespace {namespace}\n{{\n\n"));
    out.push_str(
        "public static class NetSchema\n\
         {\n\
         \x20   public static global::Fomoxa.Net.Schema Build()\n\
         \x20   {\n\
         \x20       var messages = new global::Fomoxa.Net.MessageSchema[Handshake.FomoxaMessages.Length];\n\
         \x20       for (int index = 0; index < messages.Length; index++)\n\
         \x20       {\n\
         \x20           FomoxaMessage message = Handshake.FomoxaMessages[index];\n\
         \x20           messages[index] = new global::Fomoxa.Net.MessageSchema(message.Id, message.Fingerprint, message.Prefixes);\n\
         \x20       }\n\
         \x20       return new global::Fomoxa.Net.Schema(Handshake.FomoxaSchemaFingerprint, messages);\n\
         \x20   }\n\
         }\n\
         \n\
         }\n",
    );
    Ok(out)
}

pub fn typescript(schema: &Schema) -> Result<String, String> {
    check(schema)?;
    let mut out = header(schema, "@fomoxa/net", "`fomoxaNetSchema()`");
    out.push_str(
        "import { buildSchema, type Schema } from \"@fomoxa/net\";\n\
         import { FOMOXA_MESSAGES, FOMOXA_SCHEMA_FINGERPRINT } from \"./handshake\";\n\
         \n\
         export function fomoxaNetSchema(): Schema {\n\
         \x20   return buildSchema(FOMOXA_SCHEMA_FINGERPRINT, FOMOXA_MESSAGES);\n\
         }\n",
    );
    Ok(out)
}

pub fn javascript(schema: &Schema) -> Result<String, String> {
    check(schema)?;
    let mut out = header(schema, "@fomoxa/net", "`fomoxaNetSchema()`");
    out.push_str(
        "import { buildSchema } from \"@fomoxa/net\";\n\
         import { FOMOXA_MESSAGES, FOMOXA_SCHEMA_FINGERPRINT } from \"./handshake.js\";\n\
         \n\
         export function fomoxaNetSchema() {\n\
         \x20   return buildSchema(FOMOXA_SCHEMA_FINGERPRINT, FOMOXA_MESSAGES);\n\
         }\n",
    );
    Ok(out)
}

pub fn cpp(schema: &Schema, namespace: &str) -> Result<String, String> {
    check(schema)?;
    let messages = sorted_messages(schema);
    let mut out = header(schema, "Fomoxa C", "`&FOMOXA_NET_SCHEMA`");
    out.push_str("#pragma once\n\n#include \"fomoxa/schema.h\"\n\n#include \"handshake.hpp\"\n\n");
    out.push_str(&format!("namespace {namespace} {{\n\n"));
    if messages.is_empty() {
        out.push_str(
            "inline constexpr fmx_schema FOMOXA_NET_SCHEMA = {FOMOXA_SCHEMA_FINGERPRINT, nullptr, 0};\n",
        );
    } else {
        out.push_str("inline constexpr fmx_message_schema FOMOXA_NET_MESSAGES[] = {\n");
        for message in &messages {
            let constant = message_constant(message);
            out.push_str(&format!(
                "    {{{constant}_MESSAGE_ID, {constant}_FINGERPRINT, {constant}_PREFIXES, \
                 {constant}_PREFIX_COUNT}},\n"
            ));
        }
        out.push_str("};\n\n");
        out.push_str(&format!(
            "inline constexpr fmx_schema FOMOXA_NET_SCHEMA = {{FOMOXA_SCHEMA_FINGERPRINT, \
             FOMOXA_NET_MESSAGES, {}}};\n",
            messages.len()
        ));
    }
    out.push_str(&format!("\n}}  // namespace {namespace}\n"));
    Ok(out)
}

pub fn c(schema: &Schema) -> Result<String, String> {
    check(schema)?;
    let messages = sorted_messages(schema);
    let mut out = header(schema, "Fomoxa C", "`&FOMOXA_NET_SCHEMA`");
    out.push_str("#pragma once\n\n#include \"fomoxa/schema.h\"\n\n#include \"handshake.h\"\n\n");
    let fingerprint = format!("{}ULL", hex64(schema.fingerprint.u64()));
    if messages.is_empty() {
        out.push_str(&format!(
            "static const fmx_schema FOMOXA_NET_SCHEMA = {{{fingerprint}, NULL, 0}};\n"
        ));
        return Ok(out);
    }
    out.push_str("static const fmx_message_schema FOMOXA_NET_MESSAGES[] = {\n");
    for message in &messages {
        let prefixes = if message.prefixes.is_empty() {
            "NULL".to_owned()
        } else {
            format!("{}_PREFIXES", message_constant(message))
        };
        out.push_str(&format!(
            "    {{0x{:08X}u, {}ULL, {prefixes}, {}}},\n",
            message.id,
            hex64(message.fingerprint.u64()),
            message.prefixes.len()
        ));
    }
    out.push_str("};\n\n");
    out.push_str(&format!(
        "static const fmx_schema FOMOXA_NET_SCHEMA = {{{fingerprint}, FOMOXA_NET_MESSAGES, {}}};\n",
        messages.len()
    ));
    Ok(out)
}

fn message_constant(message: &Message) -> String {
    format!(
        "{}_{}",
        screaming_snake_case(&message.model),
        screaming_snake_case(&message.codec)
    )
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::fingerprint::Fingerprint;
    use crate::ir::Model;

    fn message(model: &str, codec: &str, id: u32, fields: usize) -> Message {
        let prefixes: Vec<Fingerprint> = (0..fields)
            .map(|index| Fingerprint::of(&format!("{model}.{codec}#{index}")))
            .collect();
        Message {
            model: model.to_owned(),
            codec: codec.to_owned(),
            name: format!("{model}.{codec}"),
            id,
            fingerprint: prefixes.last().copied().unwrap_or(Fingerprint::of(model)),
            prefixes,
            fields: Vec::new(),
        }
    }

    fn schema(messages: Vec<Message>) -> Schema {
        let models = messages
            .into_iter()
            .map(|message| Model {
                name: message.model.clone(),
                source: "src/models/model.rs".to_owned(),
                fingerprint: Fingerprint::of(&message.model),
                codecs: vec![message.codec.clone()],
                fields: Vec::new(),
                messages: vec![message],
            })
            .collect();
        Schema {
            schema_version: crate::ir::SCHEMA_VERSION,
            generator: "fomoxac test".to_owned(),
            fingerprint: Fingerprint::of("schema"),
            models,
        }
    }

    #[test]
    fn c_lists_every_message_sorted_by_id_with_literal_values() {
        let text = c(&schema(vec![
            message("Team", "edge", 0x7000_0000, 2),
            message("Player", "net", 0x1000_0000, 1),
            message("Ping", "net", 0x4000_0000, 0),
        ]))
        .expect("c");

        let player = text.find("0x10000000u").expect("player");
        let ping = text.find("0x40000000u").expect("ping");
        let team = text.find("0x70000000u").expect("team");
        assert!(player < ping && ping < team, "{text}");
        assert!(text.contains("PLAYER_NET_PREFIXES, 1}"), "{text}");
        assert!(text.contains("NULL, 0}"), "{text}");
        assert!(text.contains("FOMOXA_NET_MESSAGES, 3};"), "{text}");
        assert!(text.contains("#include \"fomoxa/schema.h\""), "{text}");
    }

    #[test]
    fn cpp_names_the_handshake_constants_inside_the_namespace() {
        let text = cpp(&schema(vec![message("Player", "net", 1, 2)]), "generated").expect("cpp");
        assert!(text.contains("namespace generated {"), "{text}");
        assert!(
            text.contains(
                "{PLAYER_NET_MESSAGE_ID, PLAYER_NET_FINGERPRINT, PLAYER_NET_PREFIXES, PLAYER_NET_PREFIX_COUNT}"
            ),
            "{text}"
        );
        assert!(text.contains("FOMOXA_NET_MESSAGES, 1};"), "{text}");
    }

    #[test]
    fn an_empty_schema_still_builds() {
        let empty = schema(Vec::new());
        let literal = format!(
            "FOMOXA_NET_SCHEMA = {{{}ULL, NULL, 0}};",
            hex64(empty.fingerprint.u64())
        );
        assert!(c(&empty).expect("c").contains(&literal));
        assert!(cpp(&empty, "generated")
            .expect("cpp")
            .contains("{FOMOXA_SCHEMA_FINGERPRINT, nullptr, 0};"));
    }

    #[test]
    fn every_language_builds_from_the_handshake_table() {
        let one = schema(vec![message("Player", "net", 1, 1)]);
        assert!(rust(&one)
            .expect("rust")
            .contains("pub fn fomoxa_net_schema() -> Result<::fomoxa_net::Schema"));
        assert!(go(&one, "generated")
            .expect("go")
            .contains("return fomoxa.NewSchema(FomoxaSchemaFingerprint, messages)"));
        assert!(csharp(&one, "Generated")
            .expect("csharp")
            .contains("public static global::Fomoxa.Net.Schema Build()"));
        assert!(typescript(&one)
            .expect("typescript")
            .contains("buildSchema(FOMOXA_SCHEMA_FINGERPRINT, FOMOXA_MESSAGES)"));
        assert!(javascript(&one)
            .expect("javascript")
            .contains("from \"./handshake.js\""));
    }

    #[test]
    fn a_hello_past_the_handshake_limit_is_refused() {
        let count = (MAX_HANDSHAKE_PAYLOAD - HELLO_HEADER_SIZE) / HELLO_ENTRY_SIZE + 1;
        let messages = (0..count as u32)
            .map(|index| message(&format!("M{index}"), "net", index, 0))
            .collect();
        let error = c(&schema(messages)).expect_err("too many");
        assert!(error.contains("hello"), "{error}");
    }

    #[test]
    fn a_message_past_the_field_limit_is_refused() {
        let error = go(
            &schema(vec![message("Wide", "net", 1, MAX_FIELD_COUNT + 1)]),
            "generated",
        )
        .expect_err("too wide");
        assert!(error.contains("'Wide.net'"), "{error}");
    }
}
