# fomoxac

`fomoxac` is the source generator for the Fomoxa protocol. It reads Fomoxa annotations from a single model source tree in one of eight supported languages, and writes generated codec source files plus schema and fingerprint metadata. It is comparable in role to `protoc`: it reads source, writes source, and exits. It is not a compiler and not a runtime component; nothing it generates depends on `fomoxac` itself at run time.

The crate builds two binaries: `fomoxac` (generation, compatibility checking, CI integration) and `fomoxa-inspect` (decoding a raw payload against a schema for debugging).

Package name: `fomoxac`. Current version: `0.2.0` (see `Cargo.toml`). License: Apache-2.0.

The generator and everything it generates have zero runtime dependencies. `Cargo.toml` declares no `[dependencies]`; the one entry under `[dev-dependencies]` (`fomoxa-attributes = "0.1.1"`) is used only by the test fixtures, so that `#[network]`/`#[codec(...)]` are defined types when the fixture crate is compiled by `cargo test`.

---

## Installation

```bash
cargo install fomoxac
```

This installs both `fomoxac` and `fomoxa-inspect` from crates.io (version `0.2.0`).

To build from a checkout of this repository instead:

```bash
cargo build --release --bin fomoxac
cargo build --release --bin fomoxa-inspect
```

---

## What it reads

`fomoxac` reads exactly one language per run: Rust, Go, C#, GDScript, C++, C, TypeScript, or JavaScript. A run over a source tree containing more than one of these languages fails with an error; a project that uses more than one language needs a separate `--src`/`--out` pair (and typically a separate `fomoxa.toml`) per language.

Within a source file, the scanner looks for four pieces of information and ignores everything else: whether a type is a model, which codecs a model generates, a field's wire type, and which codecs a field belongs to. Each language expresses these with its own syntax, since only Rust and C# have an attribute mechanism `fomoxac` can extend:

| | Rust | Go | C# | GDScript | C++ / C | TypeScript / JavaScript |
|---|---|---|---|---|---|---|
| this type is a model | `#[network]` | `//fomoxa:model` | `[Network]` | `# fomoxa:model` | `FOMOXA_MODEL` | `// FOMOXA_MODEL` |
| generate these codecs | `#[codec(edge, unity)]` | `//fomoxa:model codec=edge,unity` | `[Codec("edge", "unity")]` | `# fomoxa:model codec=edge,unity` | `FOMOXA_CODEC("edge", "unity")` | `// FOMOXA_CODEC("edge", "unity")` |
| this field's wire type | `#[network(u32)]` | `` `fomoxa:"u32"` `` (struct tag) | `[Network("u32")]` | `# fomoxa:u32` | `FOMOXA_FIELD(u32)` | `// FOMOXA_FIELD(u32)` |
| this field's codecs | `#[codec(edge)]` | `` `codec:"edge"` `` (struct tag) | `[Codec("edge")]` | `# fomoxa:u32 codec=edge` | `FOMOXA_CODEC("edge")` | `// FOMOXA_CODEC("edge")` |

Go, GDScript, TypeScript and JavaScript have no attribute syntax `fomoxac` can extend, so they use a comment directive instead; a `//fomoxa:...` comment, a `# fomoxa:...` comment, or a `// FOMOXA_...` comment is already valid source in each of those languages, with no meaning until `fomoxac` reads it. C++ and C have neither attributes nor an established comment-directive convention, so they use three macros (`FOMOXA_MODEL`, `FOMOXA_CODEC(...)`, `FOMOXA_FIELD(...)`) that a small shared header defines to expand to nothing.

The wire type is never inferred from the host field's type. `#[network(u32)]` (or its equivalent) is four bytes regardless of the width of the host field; whether the host compiler accepts the resulting generated call is a question for that compiler, not for `fomoxac`.

Each per-language scanner is a lexer for that language's annotation syntax, not a full parser: it does not resolve types, traits, generics, modules, or packages. It does track string and comment boundaries, so that a `#[` inside a string literal or a `struct` keyword inside a comment is not mistaken for source.

`fomoxac` only reads these markers; the host compiler still has to accept them:

- **Rust** needs `#[network]`/`#[codec]` defined somewhere the model crate depends on — the [`fomoxa-attributes`](https://crates.io/crates/fomoxa-attributes) crate, or an equivalent no-op definition. This is a dependency of the model source, never of the generated code.
- **Go** needs nothing extra: a comment and a struct tag are already valid Go.
- **C#** needs a small `Network`/`Codec` attribute pair defined somewhere the models can see.
- **GDScript** needs nothing extra: a `# fomoxa:` comment is already valid GDScript.
- **C++ and C** both need the same header defining `FOMOXA_MODEL`, `FOMOXA_FIELD`, and `FOMOXA_CODEC` as no-ops.
- **TypeScript and JavaScript** need nothing extra: a `// FOMOXA_...` comment is already valid source in both, with no decorator and no package to install.

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

```gdscript
# fomoxa:model codec=edge,unity
class_name DeviceState

# fomoxa:u32 codec=edge,unity
var id: int = 0

# fomoxa:f32 codec=edge
var temperature: float = 0.0

# fomoxa:string codec=unity
var display_name: String = ""

# fomoxa:u32
var unrouted: int = 0

var cache: String = ""
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

JavaScript uses the identical directives with type annotations dropped (`Id;` instead of `Id: number = 0;`); the host type is never consulted in either language, only `FOMOXA_FIELD`'s own argument is.

This example generates exactly two codecs per model: `…EdgeCodec` (fields `id`/`ID`/`Id` and `temperature`) and `…UnityCodec` (fields `id`/`ID`/`Id` and `display_name`).

---

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

`Array<Array<T>>` (a directly nested array) is rejected with an error in every backend except Rust; the field must be flattened, or split across two codecs.

---

## What a run produces

```text
fomoxac generate --src src --out generated
```

produces, for the Rust backend:

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

The other backends produce an analogous file set in their own layout — see [Per-language backend notes](#per-language-backend-notes) below. In every backend, a generated file imports the model type it operates on rather than copying its fields into an intermediate representation; there is no DTO type and no runtime reflection or registry involved in encoding or decoding.

The generator has to be told where model types live relative to the generated code. By default this is read from the source layout — for Rust, `src/models/player.rs` maps to `crate::models::player`, mirroring how the Rust module system itself resolves paths. `model_path` in `fomoxa.toml`, or `--model-path` on the command line, overrides this for a project whose module layout does not mirror its directory layout.

### `fomoxa.toml`

Placed in the project root, `fomoxa.toml` supplies default values for the path-related CLI flags; an explicit CLI flag always overrides the corresponding `fomoxa.toml` value.

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

Keys may be written at the top level of the file, or under a `[fomoxa]` table; any other table is ignored. Lines starting with `#` (outside a quoted string) are comments.

Models must be reachable by the generated code: a Rust struct has to be `pub`, with the fields any codec touches also visible to it, and the equivalent visibility rule applies per language.

---

## CLI reference

```text
fomoxac generate [--src <PATH>]... [--out <PATH>] [--check] [--watch] [-q]
fomoxac compat --base <SCHEMA> [--head <SCHEMA>] [--src <PATH>]...
fomoxac ci --base-ref <REF> [--src <PATH>]... [--out <PATH>]
```

`generate` is the default command: `fomoxac` with no subcommand is equivalent to `fomoxac generate`.

### `fomoxac generate`

Reads source, writes the generated tree, `.fomoxa/schema.json`, and `.fomoxa/build-graph.json`. If a previous `.fomoxa/schema.json` exists, prints a compatibility report comparing it against the newly computed schema, but this command never fails because of a breaking change — it reports the change and generates anyway.

### `fomoxac compat`

Compares a base schema against either the current source tree or an explicit `--head` schema file, and prints the same compatibility report `generate` prints. Exits with a non-zero status if the verdict is `BREAKING`.

### `fomoxac ci`

Intended for CI. First verifies that the committed `.fomoxa/schema.json` matches the current branch's source (failing if it does not, since every later comparison would otherwise be against a stale baseline); then reads the target branch's `.fomoxa/schema.json` via `git show <base-ref>:.fomoxa/schema.json`; then compares the two schemas the same way `compat` does. Exits with a non-zero status if the schema on disk does not match source, or if the comparison verdict is `BREAKING`. If the target branch has no `.fomoxa/schema.json` at all, this is treated as `COMPATIBLE` (nothing to break yet) rather than an error.

### Flags

| Flag | Applies to | Meaning |
|---|---|---|
| `--src <PATH>` | all | A directory (scanned recursively) or a single file to read models from. Repeatable. Default: `fomoxa.toml`'s `src`, else `src`. Every discovered file must be a single language's extension (`.rs`; `.go`; `.cs`; `.gd`; `.hpp`/`.cpp`/`.cc`/`.cxx` for C++; `.c`/`.h` for C; or `.ts`/`.js` for TypeScript/JavaScript) — a mixture in one `--src` set is an error. |
| `-o`, `--out <PATH>` | all | Where generated source is written. Default: `fomoxa.toml`'s `out`, else `generated`. |
| `--model-path <PATH>` | all | Overrides how a generated codec locates a model type; meaning is per-language (see below). Default: `fomoxa.toml`'s `model_path`, else computed from source layout. |
| `--check` | `generate` | Writes nothing; exits with a non-zero status if anything on disk does not match what would be generated. Does not combine with `--watch`. |
| `--watch` | `generate` | Generates once, then keeps rereading `--src` and regenerating as it changes, until the process is terminated. Does not combine with `--check`. |
| `--base <SCHEMA>` | `compat` | Required. Path to a `schema.json` to compare against. |
| `--head <SCHEMA>` | `compat` | Path to a `schema.json` to compare, in place of reading and building the schema from `--src`. |
| `--base-ref <REF>` | `ci` | Required, and never defaulted. The git ref of the branch this change merges into, e.g. `origin/develop`. There is no default such as `main`, since a repository that merges into a branch other than `main` would otherwise get a comparison against the wrong baseline. |
| `-q`, `--quiet` | all | Reports only problems; suppresses informational output. |
| `-h`, `--help` | all | Prints usage and exits `0`. |
| `-V`, `--version` | all | Prints `fomoxac <version>` and exits `0`. |

`--model-path` means different things per backend:

| Backend | `--model-path` overrides | Default when omitted |
|---|---|---|
| Rust | A module path, e.g. `crate::models`. | Computed from the source file's path, mirroring Rust's own module resolution (`src/models/player.rs` → `crate::models::player`). |
| Go | An import path, e.g. `github.com/acme/game/models`. | Computed from the nearest `go.mod`'s `module` line plus the model source's own directory. `go.mod` must sit at the project root, next to `fomoxa.toml`. |
| C# | A namespace, e.g. `Game.Models`. | The `namespace` the model's own source declares, or none. |
| C++ | A namespace, e.g. `Game::Models`. Never affects the physical `#include` path, which is always the model's own source path. | The first `namespace` the model's own source opens, or none. |
| GDScript | No effect — a model's own `class_name` is already reachable project-wide. | N/A |
| C | No effect — there is no namespace concept in C; only the physical `#include` path applies, and that path is never overridden. | N/A |
| TypeScript / JavaScript | An ES module specifier, e.g. `@/models`, used for every model (typically a barrel file re-exporting each one). | A relative `import` computed from the model's own source path. |

There is no `--codec` flag on `generate`, `compat`, or `ci`: a model declares which codecs it generates in its own annotations, and a command-line override could only ever disagree with the source.

### Exit codes

| Code | Meaning |
|---|---|
| `0` | Success. Includes `-h`/`--help` and `-V`/`--version`. |
| `1` | `generate --check` found stale or missing output; `compat` or `ci` found a `BREAKING` verdict; `ci` found the committed schema out of date; or a runtime error occurred (missing file, parse error, `go.mod` not found, etc.). |
| `2` | A command-line usage error: an unknown flag, a missing required value, or a missing required flag (`compat` without `--base`, `ci` without `--base-ref`). |

---

## `fomoxa-inspect`

Decodes a raw payload against a `.fomoxa/schema.json`, for debugging what was actually written to the wire.

```text
fomoxa-inspect --schema <SCHEMA> --message <NAME> (--file <PATH> | --hex <HEX>)
```

| Flag | Meaning |
|---|---|
| `--schema <PATH>` | Required. Path to a `.fomoxa/schema.json`. There is no default, since the message a byte stream holds cannot be guessed. |
| `--message <NAME>` | Required. `Player`, or `Player.edge` to also name the codec. |
| `--codec <NAME>` | The codec, if `--message` did not already name one. |
| `--file <PATH>` | A binary file holding the payload. Mutually exclusive with `--hex`. |
| `--hex <HEX>` | The payload as hex digits; whitespace, `,`, `_`, and a `0x` prefix are all ignored. Mutually exclusive with `--file`. |
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

The decoder follows the same rules a generated decoder follows: a field the stream ends before is reported as absent, a field the stream ends in the middle of is a truncated-packet error, and bytes remaining after the last known field are reported as belonging to a newer, unrecognized message shape rather than treated as an error.

---

## Decoding and version skew

A byte stream that ends exactly on a field boundary is treated as valid: the writer used an older model, and the reader's remaining fields are absent (zero-valued for that read). A stream that ends in the middle of a field is a truncated packet and is treated as an error. A generated decoder implements this by asking one question per field:

```rust
value.level = if reader.field_absent() { 0u32 } else { reader.read_u32()? };
```

| Where the stream ends | Interpretation |
|---|---|
| before a field starts | that field, and every field after it, is absent (zero) |
| partway through a field's bytes | a truncated read — an error |
| after the last known field, with bytes still remaining | trailing bytes belonging to a newer writer's model — ignored |

A partial field is never treated as a zero; a truncated read is always an error, never a plausible-looking decoded value. Array elements are read strictly — a count of 3 followed by only two elements is a truncated array, not a compatible one. A nested model applies the same rule at its own level, since its own generated codec asks the same absent/truncated question of each of its own fields.

---

## Fingerprints

Every message — one model rendered through one codec — has a 32-byte SHA-256 fingerprint computed over a fully specified canonical text form, tagged `fomoxa-fingerprint/2`. The canonical form is defined normatively in [SPEC-FINGERPRINT.md](SPEC-FINGERPRINT.md); `src/fingerprint.rs` is the reference implementation, and `tests/cross_language.rs` checks that the Rust, Go, C#, GDScript, C++, C, TypeScript, and JavaScript scanners produce identical fingerprints from equivalent schemas.

A field's name is folded to a canonical spelling before hashing — lowercased, with `_`, `-`, and space removed — so that `id`, `ID`, and `Id` produce the same fingerprint, but a genuine rename (`x` to `position_x`) still changes it. Two fields of one message must not share both a canonical name and a wire type; `fomoxac` refuses to generate such a schema.

A fingerprint answers only "the same, or different." `fomoxac compat` and `fomoxac ci` answer *how* two schemas differ, from two schema files, at build time.

`handshake.rs` (and its per-language equivalent) publishes the fingerprints as constants:

```rust
pub const FOMOXA_SCHEMA_FINGERPRINT: u64 = 0x7791A1AE074FD09C;

pub const PLAYER_FINGERPRINT: u64 = 0xBA355F0228D36280;
pub const PLAYER_EDGE_MESSAGE_ID: u32 = 0x5AD3FC4F;
pub const PLAYER_EDGE_FINGERPRINT: u64 = 0x19D8F679A9BB419F;
```

These values are always generated, never hand-written.

A message id (a 32-bit value) is derived from the message name alone (the first four bytes, big-endian, of `SHA-256("fomoxa-message-id/1\n" + <Model>.<codec> + "\n")`), so that appending a field does not change the id — a peer can identify which message it is looking at while disagreeing about that message's current shape.

---

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

No schema is ever sent over the wire; both peers already have their own compiled in. The `peer_messages` argument is the peer's own `(id, field count, fingerprint)` table (`FOMOXA_MESSAGES` on this side). Distinguishing "a field was appended" from any other kind of disagreement requires comparing at the shared field count, which is why each message publishes a fingerprint per field-count prefix (`…_PREFIXES`), not only a fingerprint for its full field list — see [SPEC-FINGERPRINT.md](SPEC-FINGERPRINT.md) §3.5 for the prefix construction.

By default, no frame carries a fingerprint. Setting `validate_message_fingerprint = true` in `fomoxa.toml` adds a `[MessageId: u32][MessageFingerprint: u64]` prefix to every generated frame, with `fomoxa_write_envelope`/`fomoxa_read_envelope` generated to write and check it.

---

## Schema evolution

`.fomoxa/schema.json` is the schema written to disk as a JSON artifact: input to `fomoxa-inspect`, input to `fomoxac compat`/`fomoxac ci`, and the baseline the next generation's schema is compared against. It is never read as an input to generation itself — every `generate` run recomputes the schema from source, then compares the new result against whatever `.fomoxa/schema.json` already contained.

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

The fingerprint itself does not distinguish which of these occurred — it has no internal structure to inspect. `fomoxac compat`/`fomoxac ci` reach that verdict by comparing the two schemas field by field:

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

### Locally versus in CI

`fomoxac generate` prints the compatibility report and generates anyway — a breaking schema change on a local branch is not blocked. `fomoxac ci --base-ref <ref>` is what enforces the check:

```bash
fomoxac ci --base-ref "origin/${GITHUB_BASE_REF}"
```

It (1) verifies `.fomoxa/schema.json` still matches the current branch's source, (2) reads the target branch's `.fomoxa/schema.json` out of git via `git show`, and (3) compares the two, exiting non-zero on `BREAKING`. See [`.github/workflows/schema.yml`](.github/workflows/schema.yml).

---

## The build graph

`.fomoxa/build-graph.json` records, per source file, which model names it declares and which output files were generated from it — each output entry carrying its path, model name, codec name, message fingerprint, and a SHA-256 of the generated file's contents (with the `// generated-at:` timestamp line blanked out first, so an unchanged schema keeps an unchanged digest across runs on different days).

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

This is used to find where a generated file came from (even after its source model was deleted), to detect a generated file that was hand-edited (its digest no longer matches), and to let `fomoxac generate` delete the generated files of a model that was removed from source.

---

## Per-language backend notes

### Go

Go compiles by package rather than by file. Every file `fomoxac` writes in one run shares a single `package` clause, derived from `--out`'s own directory name; there is no module root to declare, and a codec is referenced directly, e.g. `generated.PlayerEdgeCodec{}`. A codec's `import` for its model type is computed by default from the nearest `go.mod`'s `module` line plus the model source's own directory; `go.mod` must therefore be at the project root, alongside `fomoxa.toml`. `Decode` returns `error`, checked explicitly after each read.

### C#

C# compiles by project rather than by file or package. A generated codec never writes a `using` directive; it spells a fully qualified reference (e.g. `Models.Player`) whenever the model is outside this run's own namespace (derived from `--out`'s directory name, PascalCased), and a bare reference otherwise. `Decode` takes `ref Reader` and throws `DecodeException` on failure. A nested model field is decoded through a local variable (read out, decode into it by `ref`, assign back), since a C# property cannot be passed by `ref` directly; this requires the field to already hold an instance before `Decode` runs.

### GDScript

GDScript compiles by file, and each `.gd` file exposes exactly one project-wide name via `class_name`. Each codec file declares its own `class_name` and needs no `preload`; `encode`/`decode` are `static func`s, called directly (`PlayerEdgeCodec.encode(writer, value)`) with nothing to instantiate. There is no `try`/`catch`, so every read returns a 2-element `Array`, and `decode` returns a `DecodeError` (or `null` on success) rather than throwing. Because GDScript's only integer type is signed 64-bit, every fingerprint constant is assembled from two 32-bit halves rather than a single 16-digit hex literal. `--model-path` has no effect on this backend. This backend is not compiled in this project's own CI, since no official headless Godot GitHub Action exists to build against; only `generate --check` runs automatically, and `tests/fixtures-gd/` has to be opened in the Godot editor to verify it compiles.

### C++

C++ compiles by translation unit. This backend is header-only: every method is defined inside its `struct` body (implicitly `inline`), so a generated header can be `#include`d from multiple `.cpp` files without a separate compilation unit. A model's header is always `#include`d by its own source path exactly as `--src` found it (e.g. `src/models/player.hpp`); the project's include path needs to be configured accordingly. A namespaced model reference is always written fully qualified from the global namespace (e.g. `::Game::Models::Player`). Rather than throw, every `Reader` read takes its result via an output reference and returns a `DecodeError` (a default-constructed one means "no error"), so this backend works with `-fno-exceptions`. Generated code targets C++17. This backend is compiled and its generated tree executed (via a hand-written smoke test, `tests/fixtures-cpp/smoke_test.cpp`) in this project's own CI.

### C

C reads the same `FOMOXA_MODEL`/`FOMOXA_CODEC`/`FOMOXA_FIELD` macros as C++, from the same header, but generates free functions rather than methods (`PlayerEdgeCodec_encode`, `PlayerEdgeCodec_decode`, both `static inline`). A model is always referenced as `struct Name`, never a bare `Name`, matching the plain tagged-struct declaration style the macros are written against. `--model-path` has no effect, since C has no namespace concept; only the physical `#include` path (the model's own source path) applies. Every `Reader` read returns a `FomoxaDecodeError` by output pointer (zero-initialized means "no error"); every `_encode` function and every `FomoxaWriter` method returns `bool`, since a fallible allocation has to be checked explicitly in C. A `string` field decodes to a heap-allocated `const char *`; `bytes` decodes to a `FomoxaBytes { data, len }`; `Array<T>` decodes to a generated `FomoxaArray_T { items, count }` (one such type per distinct `T` used, in a shared `arrays.h`). Each model gets a `<Model>_fomoxa.h` file with a `<Model>_free` function that releases everything any of that model's codecs allocated when decoding; it must be called exactly once per decoded value, and only on a value that is freshly zero-initialized or freshly freed. Generated code targets C99. Like C++, this backend is compiled and its generated tree run in CI.

### TypeScript

TypeScript needs no project file analogous to `go.mod`; a generated codec reaches a model class through an ordinary relative ES `import`, computed from the model's own source path by default (`src/generated/player_edge.ts` importing from `src/models/player.ts` writes `import { Player } from "../models/player";`). `encode`/`decode` are `static` methods that mutate the model class directly. `i64`/`u64` fields, and every fingerprint and per-frame envelope value, are `bigint` (a JS `number` is only exact up to 2^53); every other primitive maps to the expected JS/TS type (`number`, `string`, `boolean`, `Uint8Array` for `bytes`). A nested model field is constructed with `new ModelName()` if it does not already hold an instance, so the nested class needs a public, parameterless constructor. `decode` throws `DecodeError` on failure. This backend is compiled with `tsc` and its generated tree run (via `tests/fixtures-ts/smoke_test.ts`) in CI.

### JavaScript

The JavaScript backend generates the same IR as the TypeScript backend with every type annotation erased (replaced with `@param`/`@returns` JSDoc), and one non-cosmetic difference: the generated file is meant to be run directly by Node's ESM loader or a browser, so every relative `import` carries an explicit `.js` extension. A JavaScript codec file imports fewer models than its TypeScript counterpart, since an untyped function parameter never needs to spell a type name — only a model actually constructed with `new` (a nested field, or an array element) is imported. This backend needs no build step; its fixture is run directly with `node`.

---

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
│   │   ├── gdscript.rs         # fomoxa:model / # fomoxa:TYPE comments
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
│   ├── fixtures-gd/             GDScript fixture project
│   ├── fixtures-cpp/            C++ fixture project
│   ├── fixtures-c/              C fixture project
│   ├── fixtures-ts/             TypeScript fixture project
│   ├── fixtures-js/             JavaScript fixture project
│   └── vectors/fomoxa-vectors.json
└── SPEC-FINGERPRINT.md         normative: the fingerprint canonical form
```

Adding a further target language means adding a `parser/<lang>.rs` and a `generator/<lang>.rs` + `generator/<lang>_runtime.rs` + `generator/<lang>_handshake.rs` set; everything above the IR (`ir.rs`, `fingerprint.rs`, `schema.rs`, `compat.rs`, `buildgraph.rs`) is language-independent.

---

## Tests

```bash
cargo test
```

- **`src/**`** — unit tests for the scanners, the IR and its validation, the canonical fingerprint text against pinned digests, every row of the compatibility table, the JSON round trip, and SHA-256 against published test vectors.
- **`tests/generated.rs`** — the committed `tests/fixtures/src/generated/` tree, compiled into a real crate and run against the same annotated model files `fomoxac` scanned.
- **`tests/cli.rs`** — the real `fomoxac` binary run over real files: what is written and where, `--check`, the compatibility warnings, `compat`'s exit codes, `ci` against a real git repository, `fomoxa-inspect`, and per-backend behavior (one file per codec, mixed-language `--src` refused, `--model-path` behavior) for each of the eight backends, plus `--watch` driven once through the real binary.
- **`tests/watch.rs`** — `--watch` scenarios driven directly against `fomoxac::watch::run` rather than through a subprocess: modification, creation, and deletion of a source file; an invalid model reported and watched past, then regenerated once fixed; `--out` never watched; and two filesystem writes from one logical save settled into a single regeneration.
- **`tests/cross_language.rs`** — one schema definition, parsed through the Rust, TypeScript, JavaScript, Go, and C# scanners, checked for identical fingerprints.
- **`tests/vectors.rs`** — `tests/vectors/fomoxa-vectors.json`, the fixed cross-SDK reference vectors, checked against the real generated codecs.
- **`tests/fixtures-go/` and `tests/fixtures-cs/`, built in CI, not by `cargo test`** — `cargo test` has neither a Go toolchain nor a .NET SDK; `.github/workflows/ci.yml` builds (and, for Go, `go vet`s) each fixture directly.
- **`tests/fixtures-gd/`, in CI, `generate --check` only** — no headless Godot toolchain runs in this project's CI; only that the committed tree is current is checked automatically.
- **`tests/fixtures-cpp/` and `tests/fixtures-c/`, built and run in CI** — compiled with g++ (`-std=c++17`) and gcc (`-std=c99`) respectively, under `-Wall -Wextra -Wpedantic -Werror`, and their hand-written smoke tests executed against the real compiled output.
- **`tests/fixtures-ts/`, built and run in CI** — type-checked under `strict` with `tsc`, compiled, and its smoke test run with `node`.
- **`tests/fixtures-js/`, run in CI, no build step** — its smoke test is run directly with `node`.

`.github/workflows/ci.yml` runs on every push and pull request: `cargo fmt --check`, `cargo clippy --all-targets -- -D warnings`, `cargo test`, then a freshness check (`generate --check`) and, where a toolchain is available in CI, a real build and run, for each of the eight fixture trees. It provisions Rust (`stable`), Go `1.21`, .NET `8.0.x`, and Node `22`.

`.github/workflows/schema.yml` runs on pull requests: it fetches the target branch, builds `fomoxac`, and runs `fomoxac ci --base-ref origin/<base-ref>` against the Rust fixture, failing the check if the pull request contains a breaking schema change relative to its target branch.

---

## References

- [SPEC-FINGERPRINT.md](SPEC-FINGERPRINT.md) — the normative fingerprint canonical form.
- [`.github/workflows/ci.yml`](.github/workflows/ci.yml) — the per-language build/test matrix.
- [`.github/workflows/schema.yml`](.github/workflows/schema.yml) — the pull-request schema-compatibility gate.

## License

Apache-2.0
