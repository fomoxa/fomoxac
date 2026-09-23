# fomoxac

`fomoxac` is the source generator for the Fomoxa protocol. It reads Fomoxa annotations from a model source tree written in one of seven supported languages and writes codec source files, plus schema and fingerprint metadata. Its role is close to that of `protoc`: it reads source, writes source, and exits. Nothing it generates depends on `fomoxac` at run time.

The crate builds two binaries: `fomoxac` for generation, compatibility checks and CI, and `fomoxa-inspect` for decoding a raw payload against a schema while debugging.

Package name: `fomoxac`. Current version: `0.2.1` (see `Cargo.toml`). License: Apache-2.0.

The generator and the code it generates have no runtime dependencies. `Cargo.toml` declares no `[dependencies]`. Its one `[dev-dependencies]` entry, `fomoxa-attributes = "0.1.1"`, is used only by the test fixtures, so that `#[network]` and `#[codec(...)]` resolve to defined types when `cargo test` compiles the fixture crate.

## Installation

```bash
cargo install fomoxac
```

This installs `fomoxac` and `fomoxa-inspect` from crates.io (version `0.2.1`).

To build from a checkout of this repository:

```bash
cargo build --release --bin fomoxac
cargo build --release --bin fomoxa-inspect
```

## What it reads

Each run reads exactly one language: Rust, Go, C#, C++, C, TypeScript, or JavaScript. A run over a source tree that mixes these languages fails with an error. A project with models in more than one language needs one `--src`/`--out` pair per language, and usually one `fomoxa.toml` per language as well.

In a source file the scanner looks for four things and ignores everything else: whether a type is a model, which codecs a model generates, a field's wire type, and which codecs a field belongs to. Only Rust and C# have an attribute mechanism `fomoxac` can extend, so each language expresses these four things in its own syntax:

| | Rust | Go | C# | C++ / C | TypeScript / JavaScript |
|---|---|---|---|---|---|
| this type is a model | `#[network]` | `//fomoxa:model` | `[Network]` | `FOMOXA_MODEL` | `// FOMOXA_MODEL` |
| generate these codecs | `#[codec(edge, unity)]` | `//fomoxa:model codec=edge,unity` | `[Codec("edge", "unity")]` | `FOMOXA_CODEC("edge", "unity")` | `// FOMOXA_CODEC("edge", "unity")` |
| this field's wire type | `#[network(u32)]` | `` `fomoxa:"u32"` `` (struct tag) | `[Network("u32")]` | `FOMOXA_FIELD(u32)` | `// FOMOXA_FIELD(u32)` |
| this field's codecs | `#[codec(edge)]` | `` `codec:"edge"` `` (struct tag) | `[Codec("edge")]` | `FOMOXA_CODEC("edge")` | `// FOMOXA_CODEC("edge")` |

Go, TypeScript and JavaScript use comment directives. A `//fomoxa:...` or `// FOMOXA_...` comment is already valid source in those languages and means nothing until `fomoxac` reads it. C++ and C have neither attributes nor a settled convention for comment directives, so they use three macros, `FOMOXA_MODEL`, `FOMOXA_CODEC(...)` and `FOMOXA_FIELD(...)`, which a small shared header defines to expand to nothing.

The wire type is never inferred from the host field's type. `#[network(u32)]` and its equivalents are four bytes whatever the width of the host field. Whether the host compiler accepts the generated call is up to that compiler.

Each scanner is a lexer for its language's annotation syntax. It does not resolve types, traits, generics, modules, or packages. It does track string and comment boundaries, so a `#[` inside a string literal or a `struct` keyword inside a comment is not mistaken for source.

`fomoxac` only reads these markers, and the host compiler still has to accept them:

| Language | What the model source needs |
|---|---|
| Rust | `#[network]` and `#[codec]` defined somewhere the model crate depends on: the [`fomoxa-attributes`](https://crates.io/crates/fomoxa-attributes) crate, or an equivalent no-op definition. This is a dependency of the model source, never of the generated code |
| Go | Nothing. A comment and a struct tag are already valid Go |
| C# | A small `Network`/`Codec` attribute pair defined somewhere the models can see |
| C++ and C | The same header, defining `FOMOXA_MODEL`, `FOMOXA_FIELD`, and `FOMOXA_CODEC` as no-ops |
| TypeScript and JavaScript | Nothing. A `// FOMOXA_...` comment is already valid source in both, with no decorator and no package to install |

### Example: one model, two codecs, in every supported language

```rust
#[network]
#[codec(edge, unity)]
pub struct DeviceState {
    #[network(u32)]
    #[codec(edge, unity)]
    pub id: u32,

    #[network(f32)]
    #[codec(edge)]
    pub temperature: f32,

    #[network(string)]
    #[codec(unity)]
    pub display_name: String,

    // Declared as a network field but in no codec: written by neither.
    #[network(u32)]
    pub unrouted: u32,

    // Not a network field at all: not on the wire.
    pub cache: String,
}
```

```go
//fomoxa:model codec=edge,unity
type DeviceState struct {
	ID          uint32  `fomoxa:"u32" codec:"edge,unity"`
	Temperature float32 `fomoxa:"f32" codec:"edge"`
	DisplayName string  `fomoxa:"string" codec:"unity"`
	Unrouted    uint32  `fomoxa:"u32"`
	Cache       string
}
```

```csharp
[Network]
[Codec("edge", "unity")]
public class DeviceState
{
    [Network("u32")]
    [Codec("edge", "unity")]
    public uint Id { get; set; }

    [Network("f32")]
    [Codec("edge")]
    public float Temperature { get; set; }

    [Network("string")]
    [Codec("unity")]
    public string DisplayName { get; set; }

    [Network("u32")]
    public uint Unrouted { get; set; }

    public string Cache { get; set; }
}
```

```cpp
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
```

```c
FOMOXA_MODEL
FOMOXA_CODEC("edge", "unity")
struct DeviceState
{
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
```

```typescript
// FOMOXA_MODEL
// FOMOXA_CODEC("edge", "unity")
class DeviceState {
    // FOMOXA_FIELD(u32)
    // FOMOXA_CODEC("edge", "unity")
    Id: number = 0;

    // FOMOXA_FIELD(f32)
    // FOMOXA_CODEC("edge")
    Temperature: number = 0;

    // FOMOXA_FIELD(string)
    // FOMOXA_CODEC("unity")
    DisplayName: string = "";

    // FOMOXA_FIELD(u32)
    Unrouted: number = 0;

    Cache: string = "";
}
```

JavaScript uses the same directives with the type annotations dropped (`Id;` instead of `Id: number = 0;`). Neither language consults the host type; only the argument of `FOMOXA_FIELD` counts.

Each version of the example generates exactly two codecs: `…EdgeCodec`, with the fields `id`/`ID`/`Id` and `temperature`, and `…UnityCodec`, with `id`/`ID`/`Id` and `display_name`.

## Wire type reference

| Fomoxa type | Wire representation |
|---|---|
| `bool` | 1 byte: `0x00` or `0x01` |
| `i8` / `u8` | 1 byte |
| `i16` / `u16` | 2 bytes, little-endian |
| `i32` / `u32` / `f32` | 4 bytes, little-endian |
| `i64` / `u64` / `f64` | 8 bytes, little-endian |
| `string` | 4-byte little-endian length (bytes), then UTF-8 bytes |
| `bytes` | 4-byte little-endian length, then raw bytes |
| `Array<T>` | 4-byte little-endian element count, then each element |
| a model name | that model's fields, inlined in declaration order |

Every backend except Rust rejects `Array<Array<T>>`, a directly nested array, with an error. Flatten the field, or split it across two codecs.

## What a run produces

```text
fomoxac generate --src src --out generated
```

For the Rust backend this produces:

```text
src/generated/
    mod.rs             module root: declares and re-exports the rest
    runtime.rs         the wire-format runtime, copied verbatim
    handshake.rs        every fingerprint, and the handshake helpers
    player_edge.rs      one codec, one file
    player_unity.rs
.fomoxa/
    schema.json         the schema, as a JSON artifact
    build-graph.json    which source produced which output file
```

The other backends produce an equivalent set of files in their own layout; see [Per-language backend notes](#per-language-backend-notes). In every backend a generated file imports the model type it works on and reads and writes its fields directly. There is no intermediate DTO type, and encoding and decoding use no runtime reflection or registry.

The generator has to know where model types live relative to the generated code. By default it reads this from the source layout. For Rust, `src/models/player.rs` maps to `crate::models::player`, the same way the Rust module system resolves paths. `model_path` in `fomoxa.toml`, or `--model-path` on the command line, overrides this for a project whose module layout does not follow its directory layout.

### `fomoxa.toml`

`fomoxa.toml` sits in the project root and supplies default values for the path flags. A flag given on the command line always overrides the value in `fomoxa.toml`.

```toml
src = "src"
out = "src/generated"
model_path = "crate::models"            # optional
validate_message_fingerprint = true     # optional, default false
```

| Key | Type | Default | Meaning |
|---|---|---|---|
| `src` | string, or array of strings | `["src"]` | One or more directories (scanned recursively) or files to read models from. |
| `out` | string | `"generated"` | Where generated files are written. |
| `model_path` | string | (computed per language; see the flag table below) | Overrides how a generated codec locates a model type. |
| `validate_message_fingerprint` | boolean | `false` | If `true`, every generated frame gains a `[MessageId: u32][MessageFingerprint: u64]` prefix, and `fomoxa_write_envelope`/`fomoxa_read_envelope` are generated to write and check it. |

Keys may sit at the top level of the file or under a `[fomoxa]` table; other tables are ignored. A line starting with `#` outside a quoted string is a comment.

The generated code has to be able to reach the models. A Rust struct must be `pub`, and so must every field a codec touches. Each language applies its own equivalent visibility rule.

## CLI reference

```text
fomoxac generate [--src <PATH>]... [--out <PATH>] [--check] [--watch] [-q]
fomoxac compat --base <SCHEMA> [--head <SCHEMA>] [--src <PATH>]...
fomoxac ci --base-ref <REF> [--src <PATH>]... [--out <PATH>]
```

`generate` is the default command, so `fomoxac` with no subcommand is the same as `fomoxac generate`.

### `fomoxac generate`

Reads source and writes the generated tree, `.fomoxa/schema.json`, and `.fomoxa/build-graph.json`. When a `.fomoxa/schema.json` already exists, it prints a compatibility report comparing that schema with the new one. It never fails because of a breaking change: it reports the change and generates anyway.

### `fomoxac compat`

Compares a base schema with either the current source tree or a schema file given by `--head`, and prints the same compatibility report as `generate`. Exits with a non-zero status when the verdict is `BREAKING`.

### `fomoxac ci`

Meant for CI, in three steps. It first checks that the committed `.fomoxa/schema.json` matches the current branch's source, and fails if it does not, because every later comparison would use a stale baseline. It then reads the target branch's `.fomoxa/schema.json` with `git show <base-ref>:.fomoxa/schema.json`, and compares the two schemas the same way `compat` does. It exits with a non-zero status if the schema on disk does not match the source, or if the verdict is `BREAKING`. If the target branch has no `.fomoxa/schema.json`, the result is `COMPATIBLE`, since there is nothing to break yet.

### Flags

| Flag | Applies to | Meaning |
|---|---|---|
| `--src <PATH>` | all | A directory (scanned recursively) or a single file to read models from. Repeatable. Default: `fomoxa.toml`'s `src`, else `src`. Every file found must have the extension of one single language (`.rs`; `.go`; `.cs`; `.hpp`/`.cpp`/`.cc`/`.cxx` for C++; `.c`/`.h` for C; or `.ts`/`.js` for TypeScript/JavaScript). A mix in one `--src` set is an error. |
| `-o`, `--out <PATH>` | all | Where generated source is written. Default: `fomoxa.toml`'s `out`, else `generated`. |
| `--model-path <PATH>` | all | Overrides how a generated codec locates a model type. Its meaning depends on the backend (see below). Default: `fomoxa.toml`'s `model_path`, else computed from the source layout. |
| `--check` | `generate` | Writes nothing. Exits with a non-zero status if anything on disk differs from what would be generated. Cannot be combined with `--watch`. |
| `--watch` | `generate` | Generates once, then keeps rereading `--src` and regenerates when it changes, until the process is terminated. Cannot be combined with `--check`. |
| `--base <SCHEMA>` | `compat` | Required. Path to the `schema.json` to compare against. |
| `--head <SCHEMA>` | `compat` | Path to a `schema.json` to compare, used instead of building the schema from `--src`. |
| `--base-ref <REF>` | `ci` | Required, with no default. The git ref of the branch this change merges into, e.g. `origin/develop`. A default such as `main` would make a repository that merges into another branch compare against the wrong baseline. |
| `-q`, `--quiet` | all | Reports only problems and suppresses informational output. |
| `-h`, `--help` | all | Prints usage and exits `0`. |
| `-V`, `--version` | all | Prints `fomoxac <version>` and exits `0`. |

What `--model-path` sets depends on the backend:

| Backend | `--model-path` overrides | Default when omitted |
|---|---|---|
| Rust | A module path, e.g. `crate::models`. | Computed from the source file's path, following Rust's own module resolution (`src/models/player.rs` → `crate::models::player`). |
| Go | An import path, e.g. `github.com/acme/game/models`. | Computed from the nearest `go.mod`'s `module` line plus the model source's own directory. `go.mod` must sit at the project root, next to `fomoxa.toml`. |
| C# | A namespace, e.g. `Game.Models`. | The `namespace` the model's own source declares, or none. |
| C++ | A namespace, e.g. `Game::Models`. It never affects the physical `#include` path, which is always the model's own source path. | The first `namespace` the model's own source opens, or none. |
| C | No effect. C has no namespaces, and the physical `#include` path it does use is never overridden. | N/A |
| TypeScript / JavaScript | An ES module specifier, e.g. `@/models`, used for every model (typically a barrel file that re-exports each one). | A relative `import` computed from the model's own source path. |

No command has a `--codec` flag. A model declares its codecs in its own annotations, and a command-line value could only disagree with the source.

### Exit codes

| Code | Meaning |
|---|---|
| `0` | Success. Includes `-h`/`--help` and `-V`/`--version`. |
| `1` | `generate --check` found stale or missing output; `compat` or `ci` found a `BREAKING` verdict; `ci` found the committed schema out of date; or a runtime error occurred (missing file, parse error, `go.mod` not found, etc.). |
| `2` | A command-line usage error: an unknown flag, a missing required value, or a missing required flag (`compat` without `--base`, `ci` without `--base-ref`). |

## `fomoxa-inspect`

Decodes a raw payload against a `.fomoxa/schema.json`, to show what was actually written to the wire.

```text
fomoxa-inspect --schema <SCHEMA> --message <NAME> (--file <PATH> | --hex <HEX>)
```

| Flag | Meaning |
|---|---|
| `--schema <PATH>` | Required. Path to a `.fomoxa/schema.json`. There is no default, because the message a byte stream holds cannot be guessed. |
| `--message <NAME>` | Required. `Player`, or `Player.edge` to name the codec as well. |
| `--codec <NAME>` | The codec, if `--message` did not already name one. |
| `--file <PATH>` | A binary file holding the payload. Cannot be combined with `--hex`. |
| `--hex <HEX>` | The payload as hex digits. Whitespace, `,`, `_`, and a `0x` prefix are ignored. Cannot be combined with `--file`. |
| `--expect <FP>` | Fails unless the message's fingerprint equals this value, given as `sha256:…` or `0x…`. |
| `-h`, `--help` | Prints usage and exits `0`. |

```bash
fomoxa-inspect --schema .fomoxa/schema.json --message Player --file packet.bin
fomoxa-inspect --schema .fomoxa/schema.json --message Player.edge --hex '64000000 00002841 0000a041'
```

```text
Player.edge
fingerprint: sha256:19d8f679a9bb419fad6a11350c111bf9616b8f2e3bf281fb5c9be837216ca2fa (0x19D8F679A9BB419F)
message id : 0x5AD3FC4F
payload    : 12 bytes
----------------------------------------------------
id      : u32 = 100
          offset: 0
          bytes: 64 00 00 00

x       : f32 = 10.5
          offset: 4
          bytes: 00 00 28 41
```

The decoder applies the same rules as a generated decoder. A field the stream ends before is reported as absent. A field the stream ends inside is a truncated-packet error. Bytes left after the last known field are reported as belonging to a newer message shape, and are not an error.

## Decoding and version skew

A byte stream that ends exactly on a field boundary is valid: the writer used an older model, so the reader's remaining fields are absent and read as zero. A stream that ends inside a field is a truncated packet and an error. A generated decoder asks one question per field:

```rust
value.level = if reader.field_absent() { 0u32 } else { reader.read_u32()? };
```

| Where the stream ends | Interpretation |
|---|---|
| before a field starts | that field, and every field after it, is absent (zero) |
| partway through a field's bytes | a truncated read, which is an error |
| after the last known field, with bytes still remaining | trailing bytes belonging to a newer writer's model, which are ignored |

A partial field is never read as zero, so a truncated read always ends in an error and never in a plausible value. Array elements are read strictly: a count of 3 followed by two elements is a truncated array. A nested model applies the same rule at its own level, because its generated codec asks the same absent-or-truncated question of each of its fields.

## Fingerprints

Every message, meaning one model rendered through one codec, has a 32-byte SHA-256 fingerprint computed over a fully specified canonical text form tagged `fomoxa-fingerprint/2`. [SPEC-FINGERPRINT.md](SPEC-FINGERPRINT.md) defines the canonical form normatively, and `src/fingerprint.rs` is the reference implementation. `tests/cross_language.rs` checks that the Rust, Go, C#, TypeScript, and JavaScript scanners produce identical fingerprints from equivalent schemas.

Before hashing, a field's name is folded to a canonical spelling: lowercased, with `_`, `-`, and spaces removed. So `id`, `ID`, and `Id` give the same fingerprint, while a real rename such as `x` to `position_x` changes it. Two fields of one message must not share both a canonical name and a wire type; `fomoxac` refuses to generate such a schema.

A fingerprint only says whether two messages are the same. `fomoxac compat` and `fomoxac ci` say how two schemas differ, working from two schema files at build time.

`handshake.rs` and its equivalent in each language publish the fingerprints as constants:

```rust
pub const FOMOXA_SCHEMA_FINGERPRINT: u64 = 0x7791A1AE074FD09C;

pub const PLAYER_FINGERPRINT: u64 = 0xBA355F0228D36280;
pub const PLAYER_EDGE_MESSAGE_ID: u32 = 0x5AD3FC4F;
pub const PLAYER_EDGE_FINGERPRINT: u64 = 0x19D8F679A9BB419F;
```

These values are always generated, never written by hand.

A message id is a 32-bit value derived from the message name alone: the first four bytes, big-endian, of `SHA-256("fomoxa-message-id/1\n" + <Model>.<codec> + "\n")`. Appending a field therefore leaves the id unchanged, and a peer can tell which message it is looking at even when the two sides disagree on that message's current shape.

## Handshake

```text
Client  ──  FOMOXA_SCHEMA_FINGERPRINT  ──▶  Server
```

| Condition | Outcome |
|---|---|
| the schema fingerprints match exactly | `CURRENT`: accept |
| a message both ends know, and the fields both ends carry agree | `OUTDATED`: accept |
| a message both ends know, differing at an index both ends carry | `REJECT`: disconnect |
| the peer carries more fields than this side does, on a message that differs | `NEED_MORE`: request the peer's prefix fingerprint once |

```rust
match fomoxa_handshake(peer_schema_fingerprint, peer_messages) {
    FomoxaHandshake::Current => accept(),
    FomoxaHandshake::Outdated => accept(),
    FomoxaHandshake::Reject => disconnect(),
    FomoxaHandshake::NeedMore => ask_the_peer(),
}
```

No schema is ever sent over the wire, because both peers have their own compiled in. The `peer_messages` argument is the peer's `(id, field count, fingerprint)` table, the counterpart of `FOMOXA_MESSAGES` on this side. Telling an appended field apart from any other disagreement means comparing at the shared field count, so each message publishes one fingerprint per field-count prefix (`…_PREFIXES`) as well as the fingerprint of its full field list. [SPEC-FINGERPRINT.md](SPEC-FINGERPRINT.md) §3.5 gives the prefix construction.

By default no frame carries a fingerprint. Setting `validate_message_fingerprint = true` in `fomoxa.toml` adds a `[MessageId: u32][MessageFingerprint: u64]` prefix to every generated frame, and generates `fomoxa_write_envelope`/`fomoxa_read_envelope` to write and check it.

## Schema evolution

`.fomoxa/schema.json` is the schema written to disk as JSON. It is the input to `fomoxa-inspect` and to `fomoxac compat`/`fomoxac ci`, and the baseline the next run compares against. Generation never reads it as input: every `generate` run recomputes the schema from source, then compares the result with whatever `.fomoxa/schema.json` already held.

```text
Source Model
      ↓
Scanner / Parser
      ↓
Fomoxa IR  ────┬───────────────→ generated codec
               ├───────────────→ schema.json
               ├───────────────→ fingerprints
               └───────────────→ build-graph.json
```

### Compatibility table

| Change | Verdict |
|---|---|
| nothing changed | `CURRENT` |
| a field appended at the end | `COMPATIBLE` |
| trailing fields removed (the shorter field list is still a prefix of the longer one) | `COMPATIBLE` |
| a field removed from the middle | `BREAKING` |
| a field inserted in the middle | `BREAKING` |
| fields reordered | `BREAKING` |
| a field's wire type changed | `BREAKING` |
| a field renamed, nothing else | `BREAKING` (the bytes are unchanged, but the fingerprint is not) |
| a whole message added | `COMPATIBLE` |
| a whole message removed | `BREAKING` |

A fingerprint has no internal structure, so it cannot say which of these changes happened. `fomoxac compat` and `fomoxac ci` reach their verdict by comparing the two schemas field by field:

```text
⚠ Player.edge:
  field[1]:
    old: x:f32
    new: y:f32
  BREAKING: field order changed
```

```text
⚠ Player.edge:
  + level:u32 at index 3
  COMPATIBLE: append-only fields (1 appended at the end)
```

### Locally and in CI

`fomoxac generate` prints the compatibility report and generates anyway, so a breaking schema change on a local branch goes through. `fomoxac ci --base-ref <ref>` enforces the check:

```bash
fomoxac ci --base-ref "origin/${GITHUB_BASE_REF}"
```

It checks that `.fomoxa/schema.json` still matches the current branch's source, reads the target branch's `.fomoxa/schema.json` from git with `git show`, compares the two, and exits non-zero on `BREAKING`. See [`.github/workflows/schema.yml`](.github/workflows/schema.yml).

## The build graph

`.fomoxa/build-graph.json` records, for each source file, the model names it declares and the output files generated from it. Each output entry carries its path, model name, codec name, message fingerprint, and a SHA-256 of the generated file's contents. The `// generated-at:` timestamp line is blanked before hashing, so an unchanged schema keeps the same digest across runs on different days.

```json
{
  "sources": {
    "src/models/player.rs": {
      "models": ["Player", "PlayerInfo"],
      "outputs": [
        {
          "path": "src/generated/player_edge.rs",
          "model": "Player",
          "codec": "edge",
          "fingerprint": "sha256:231dd2…",
          "sha256": "9f1e35…"
        }
      ]
    }
  }
}
```

The build graph shows where a generated file came from, even after its source model was deleted. It also reveals a generated file that was edited by hand, because the digest no longer matches, and it lets `fomoxac generate` delete the generated files of a model removed from source.

## Per-language backend notes

### Go

Go compiles by package. Every file `fomoxac` writes in one run shares one `package` clause, named after the directory of `--out`. There is no module root to declare, and code refers to a codec directly, e.g. `generated.PlayerEdgeCodec{}`. By default a codec's `import` of its model type is computed from the nearest `go.mod`'s `module` line plus the model source's own directory, so `go.mod` has to be at the project root, next to `fomoxa.toml`. `Decode` returns `error`, checked after each read. `Writer.Reset()` keeps the buffer for the next message and `NewWriterSize(n)` starts with room for `n` bytes; a writer reused this way encodes without allocating. The generated code builds where `int` is 32 bits: there the permissive `UnlimitedLimits` is the largest `int` rather than `math.MaxUint32`, and CI runs the vectors under `GOARCH=386`.

### C#

C# compiles by project. A generated codec never writes a `using` directive. It writes a fully qualified reference (e.g. `Models.Player`) when the model lives outside the run's own namespace, which is the directory name of `--out` in PascalCase, and a bare reference otherwise. `Decode` takes `ref Reader` and throws `DecodeException` on failure. A C# property cannot be passed by `ref`, so a nested model field is decoded through a local variable: read it out, decode into it by `ref`, and assign it back. The field must therefore already hold an instance before `Decode` runs. `Writer` writes into one `byte[]` that `Clear()` keeps, so a writer reused across messages stops allocating once its buffer has grown to the largest message; `WrittenSpan` and `WrittenMemory` expose the written bytes without copying, and `ToArray()` copies them. An array count read from the stream preallocates at most 4096 elements, the same bound the Rust backend uses.

### C++

C++ compiles by translation unit. This backend is header-only: every method is defined inside its `struct` body and is therefore implicitly `inline`, so several `.cpp` files can `#include` a generated header without a separate compilation unit. A model's header is always included by its own source path exactly as `--src` found it (e.g. `src/models/player.hpp`), and the project's include path has to be set up for that. A reference to a namespaced model is always fully qualified from the global namespace (e.g. `::Game::Models::Player`). Every `Reader` read returns its result through an output reference and returns a `DecodeError`, where a default-constructed one means "no error", so this backend works with `-fno-exceptions`. Generated code targets C++17. This project's CI compiles this backend and runs its generated tree through a hand-written smoke test, `tests/fixtures-cpp/smoke_test.cpp`. An array count read from the stream reserves at most 4096 elements, the same bound the Rust backend uses, so a hostile count cannot make `decode` throw `std::length_error` or `std::bad_alloc` before a single element is read.

### C

C reads the same `FOMOXA_MODEL`/`FOMOXA_CODEC`/`FOMOXA_FIELD` macros as C++, from the same header, but generates free functions: `PlayerEdgeCodec_encode` and `PlayerEdgeCodec_decode`, both `static inline`. A model is always referenced as `struct Name`, never as a bare `Name`, which matches the plain tagged-struct declarations the macros are written against. `--model-path` has no effect, since C has no namespaces; only the physical `#include` path, the model's own source path, applies. Every `Reader` read returns a `FomoxaDecodeError` through an output pointer, where zero-initialized means "no error". Every `_encode` function and every `FomoxaWriter` method returns `bool`, because C code has to check a fallible allocation explicitly. A `string` field decodes to a heap-allocated `const char *`, `bytes` to a `FomoxaBytes { data, len }`, and `Array<T>` to a generated `FomoxaArray_T { items, count }`, with one such type per distinct `T` in a shared `arrays.h`. Each model gets a `<Model>_fomoxa.h` file with a `<Model>_free` function that releases everything that model's codecs allocated while decoding. Call it exactly once per decoded value, and only on a value that is freshly zero-initialized or freshly freed. Generated code targets C99. As with C++, CI compiles this backend and runs its generated tree. `fomoxa_writer_reset` empties a writer and keeps its buffer, so one writer can encode message after message without reallocating.

### TypeScript

TypeScript needs no project file like `go.mod`. A generated codec reaches a model class through an ordinary relative ES `import`, computed by default from the model's own source path: `src/generated/player_edge.ts` importing from `src/models/player.ts` writes `import { Player } from "../models/player";`. `encode` and `decode` are `static` methods that mutate the model class directly. `i64`/`u64` fields, every fingerprint, and every per-frame envelope value are `bigint`, because a JS `number` is only exact up to 2^53. The other primitives map to the usual JS/TS types: `number`, `string`, `boolean`, and `Uint8Array` for `bytes`. A nested model field is constructed with `new ModelName()` if it does not already hold an instance, so the nested class needs a public constructor with no parameters. `decode` throws `DecodeError` on failure. CI compiles this backend with `tsc` and runs its generated tree through `tests/fixtures-ts/smoke_test.ts`. `Writer` keeps its buffer across `clear()`, and `writtenView()` returns the written bytes as a `subarray` over that buffer, valid until the next write or `clear()`; `toUint8Array()` copies them. A `string` is encoded with `TextEncoder.encodeInto` straight into the buffer. Reusing one `Writer` matters more here than in any other backend: every new `Writer`, and every time its buffer grows, allocates a new `ArrayBuffer` for its `DataView`, which costs about a microsecond on Node.

### JavaScript

The JavaScript backend renders the same IR as the TypeScript backend, with every type annotation replaced by `@param`/`@returns` JSDoc. One difference affects behavior: the generated file is meant to run directly under Node's ESM loader or in a browser, so every relative `import` carries an explicit `.js` extension. A JavaScript codec file also imports fewer models than its TypeScript counterpart. An untyped function parameter never names a type, so a codec imports only the models it constructs with `new`, for a nested field or an array element. This backend has no build step, and its fixture runs directly with `node`. Its `Writer` has the same `clear()` and `writtenView()` as the TypeScript one, and the same reason to reuse it.

## Repository layout

```text
fomoxac/
├── src/
│   ├── bin/
│   │   ├── fomoxac.rs          generate / compat / ci
│   │   └── fomoxa_inspect.rs
│   ├── cli.rs                  argument parsing for fomoxac
│   ├── config.rs               fomoxa.toml
│   ├── gomod.rs                 enough of go.mod to compute an import path
│   ├── parser/
│   │   ├── rust.rs             #[network] / #[codec(...)]
│   │   ├── go.rs               //fomoxa:model + struct tags
│   │   ├── csharp.rs           [Network] / [Codec(...)]
│   │   ├── cpp.rs              FOMOXA_MODEL / FOMOXA_CODEC(...) / FOMOXA_FIELD(...)
│   │   ├── c.rs                 the same, minus namespace/class handling
│   │   └── typescript.rs       // FOMOXA_MODEL / ... comments, for .ts and .js
│   ├── model.rs                what a scanner collected
│   ├── ir.rs                   the Fomoxa IR
│   ├── fingerprint.rs          the canonical text form, and SHA-256 over it
│   ├── schema.rs               .fomoxa/schema.json
│   ├── compat.rs               CURRENT / COMPATIBLE / BREAKING
│   ├── buildgraph.rs           .fomoxa/build-graph.json
│   ├── generate.rs             discover → parse → IR → render → compare → write
│   ├── watch.rs                --watch: poll, diff, settle, regenerate
│   ├── generator/               one {lang}.rs, {lang}_runtime.rs, {lang}_handshake.rs
│   │                            triple per target language
│   ├── inspect.rs              fomoxa-inspect
│   ├── json.rs                 a small hand-written JSON writer/reader
│   ├── sha256.rs                a hand-written SHA-256 implementation
│   └── timestamp.rs
├── tests/
│   ├── cli.rs                  the real binaries, over real files
│   ├── watch.rs                --watch, driven at the library level
│   ├── generated.rs            the committed Rust generated tree, compiled and run
│   ├── vectors.rs              the cross-SDK vectors, checked
│   ├── cross_language.rs       one schema, read through every parser, checked for
│   │                           identical fingerprints
│   ├── fixtures/                Rust fixture project
│   ├── fixtures-go/             Go fixture project
│   ├── fixtures-cs/             C# fixture project
│   ├── fixtures-cpp/            C++ fixture project
│   ├── fixtures-c/              C fixture project
│   ├── fixtures-ts/             TypeScript fixture project
│   ├── fixtures-js/             JavaScript fixture project
│   ├── vectors-cs/              the vector models and a runner, C#
│   ├── vectors-go/              the vector models and a runner, Go
│   ├── vectors-cpp/             the vector models and a runner, C++
│   ├── vectors-c/               the vector models and a runner, C
│   └── vectors/                 fomoxa-vectors.json, and lines.py for the C/C++ runners
└── SPEC-FINGERPRINT.md         normative: the fingerprint canonical form
```

To add a target language, add `parser/<lang>.rs` and the set `generator/<lang>.rs`, `generator/<lang>_runtime.rs`, and `generator/<lang>_handshake.rs`. Everything above the IR (`ir.rs`, `fingerprint.rs`, `schema.rs`, `compat.rs`, `buildgraph.rs`) is independent of the language.

## Tests

```bash
cargo test
```

| Where | What it covers |
|---|---|
| `src/**` | Unit tests for the scanners, the IR and its validation, the canonical fingerprint text against pinned digests, every row of the compatibility table, the JSON round trip, and SHA-256 against published test vectors |
| `tests/generated.rs` | The committed `tests/fixtures/src/generated/` tree, compiled into a real crate and run against the same annotated model files `fomoxac` scanned |
| `tests/cli.rs` | The real `fomoxac` binary over real files: what is written and where, `--check`, the compatibility warnings, the exit codes of `compat`, `ci` against a real git repository, `fomoxa-inspect`, and per-backend behavior for each of the seven backends (one file per codec, a mixed-language `--src` refused, `--model-path`), plus one run of `--watch` through the real binary |
| `tests/watch.rs` | `--watch` driven directly through `fomoxac::watch::run` instead of a subprocess: a source file modified, created, and deleted; an invalid model reported and watched past, then regenerated once fixed; `--out` never watched; and two filesystem writes from one save settled into a single regeneration |
| `tests/cross_language.rs` | One schema definition parsed through the Rust, TypeScript, JavaScript, Go, and C# scanners, checked for identical fingerprints |
| `tests/vectors.rs` | `tests/vectors/fomoxa-vectors.json`, the fixed cross-SDK reference vectors, checked against the real generated Rust codecs |

`cargo test` has no Go toolchain and no .NET SDK, so `.github/workflows/ci.yml` builds `tests/fixtures-go/` and `tests/fixtures-cs/` directly, and runs `go vet` on the Go fixture. The remaining fixtures are checked in CI as follows:

- `tests/fixtures-cpp/` and `tests/fixtures-c/`: compiled with g++ (`-std=c++17`) and gcc (`-std=c99`) under `-Wall -Wextra -Wpedantic -Werror`, and their hand-written smoke tests run against the compiled output.
- `tests/fixtures-ts/`: type-checked under `strict` with `tsc`, compiled, and its smoke test run with `node`.

Every other backend is checked against the same vectors, byte for byte, in CI: each `accept` vector is decoded and re-encoded to its `reencode` bytes, each `reject` vector fails with its named error, and every message's id and fingerprint match the file. The runners are `tests/vectors-cs/Program.cs`, `tests/vectors-go/main.go`, `tests/vectors-cpp/vectors_test.cpp`, `tests/vectors-c/vectors_test.c` (fed by `tests/vectors/lines.py`, and built under AddressSanitizer and UndefinedBehaviorSanitizer), `tests/fixtures-ts/vectors_test.ts` and `tests/fixtures-js/vectors_test.js`. A round-trip smoke test alone cannot show this: an encoder and a decoder that are wrong the same way still round-trip.
- `tests/fixtures-js/`: its smoke test run directly with `node`, with no build step.

`.github/workflows/ci.yml` runs on every push and pull request. It runs `cargo fmt --check`, `cargo clippy --all-targets -- -D warnings`, and `cargo test`, then for each of the seven fixture trees a freshness check (`generate --check`) and, where CI has the toolchain, a real build and run. It provisions Rust (`stable`), Go `1.21`, .NET `8.0.x`, and Node `22`.

`.github/workflows/schema.yml` runs on pull requests. It fetches the target branch, builds `fomoxac`, and runs `fomoxac ci --base-ref origin/<base-ref>` against the Rust fixture, so the check fails when a pull request contains a breaking schema change relative to its target branch.

## References

- [SPEC-FINGERPRINT.md](SPEC-FINGERPRINT.md): the normative fingerprint canonical form.
- [`.github/workflows/ci.yml`](.github/workflows/ci.yml): the build and test matrix per language.
- [`.github/workflows/schema.yml`](.github/workflows/schema.yml): the schema compatibility check on pull requests.

## License

Apache-2.0
