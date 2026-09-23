# Runtime design: Reader and Writer

Each backend's `Reader` and `Writer` are written once, as a template in `src/generator/<language>_runtime.rs`, and copied verbatim into every generated tree. This document records why they are built the way they are, what each design choice was measured against, and which alternatives were tried and dropped. Read the section for a backend before changing its runtime or the decoders its generator emits. When a change is measured, add the numbers here and in the commit message, including changes that were rejected.

## Rules every runtime keeps

These rules come from RFC-0002 and apply in every backend. Any change has to keep them.

- A length prefix is compared with the configured limit and with the bytes remaining before anything is allocated. The comparison is done in a type wide enough for any `u32`: C# compares as `uint` before narrowing to `int`, Go compares as `uint64`. A length of `0xFFFFFFFF` is therefore `UnexpectedEof` or `LengthOverflow`, never a runtime exception from a narrowing cast.
- A failed read leaves the cursor where it was.
- A `string` must be strict UTF-8, and a `bool` byte must be `0x00` or `0x01`.
- A field that is absent at a field boundary takes its zero value. A field the stream ends inside is an error (RFC-0002 §9.1).
- An array count read from the stream preallocates at most 4096 elements. The count is untrusted, so preallocating it lets a four-byte message claim gigabytes. The Rust backend set this bound first, and the Go, C# and C++ backends copied it.

## Allocation goals

Two steady states are required, in every backend:

1. A `Writer` that is cleared and reused encodes without allocating once its buffer has grown to the largest message.
2. Decoding into a value that already holds a previous message allocates nothing when the bytes are the same, and allocates only to grow when they differ.

The one exception to the second goal is a `string` whose text changed, in a language whose strings are immutable (C#, Go, TypeScript, JavaScript). The new text has to become a new string.

Decoding into a held value overwrites its buffers. A caller that shares one of those buffers with other data has to copy it before decoding. A decode that fails partway leaves the fields read before the failure already overwritten. The earlier behavior, which allocated fresh storage on every decode, also overwrote fields progressively, so this is not a new hazard.

Each backend's vector runner checks the second goal on every accept vector. It decodes the vector into a value that persists across all vectors, re-encodes it, then decodes the same bytes again and checks the second pass:

| Backend | Runner | What the second decode must show |
|---|---|---|
| C# | `tests/vectors-cs/Program.cs` | `GC.GetAllocatedBytesForCurrentThread()` unchanged |
| Go | `tests/vectors-go/main.go` | `testing.AllocsPerRun` returns 0, also under `GOARCH=386` |
| C++ | `tests/vectors-cpp/vectors_test.cpp` | a replaced global `operator new` counts 0 calls |
| C | `tests/vectors-c/vectors_test.c` | every string, bytes and array pointer unchanged, under AddressSanitizer and LeakSanitizer |
| Rust | `tests/generated.rs` | `as_ptr()` of every `String` and `Vec` unchanged |
| TypeScript, JavaScript | `tests/fixtures-ts/vectors_test.ts`, `tests/fixtures-js/vectors_test.js` | every held array, `Uint8Array` and model is the same object |

The runners visit the vectors forwards and then backwards, so a value that held a longer array or string is decoded into by a shorter one and the other way round.

## C#

### Writer

`Writer` keeps one `byte[]` across `Clear()` and doubles it when it runs out. Numbers are written with `BinaryPrimitives`. A string is encoded in one pass: the writer reserves four bytes for the length, makes room for `Encoding.UTF8.GetMaxByteCount`, encodes straight into the buffer and writes the actual byte count back into the four reserved bytes. `WrittenSpan` and `WrittenMemory` expose the written bytes without copying, and `ToArray()` copies them.

The first version used a `List<byte>`, added numbers one byte at a time, and allocated a temporary array for every string. Replacing it (commit `856a687`) took encode on the vector models from 109 to 426 ns and 200 to 656 B per message down to 13 to 92 ns and 0 B.

`WriteBytes` has overloads for `byte[]`, `ReadOnlySpan<byte>`, `ArraySegment<byte>`, `Memory<byte>` and `ReadOnlyMemory<byte>`, so the generated call `writer.WriteBytes(value.Field)` compiles for any of those host types.

### Reader

`Reader` is a `ref struct` over a `ReadOnlySpan<byte>`. It caches one strict `UTF8Encoding`. The first version created a new one for every string.

A reader built from a `byte[]`, an `ArraySegment<byte>` or a `ReadOnlyMemory<byte>` also keeps the input as memory. That is what lets a `ReadOnlyMemory<byte>` field be decoded as a slice of the input instead of a copy.

### Strings

The generated decoder calls `ReadString(ref string)`. `IsSameText` walks the held string's UTF-16 code units, encodes each scalar value to UTF-8 on the fly and compares it with the incoming bytes. When every byte matches, the held instance is kept and nothing is decoded or allocated. When the held string contains a lone surrogate, the comparison fails and the bytes are decoded normally, so validation still happens.

A first version decoded the bytes into a 256-character `stackalloc` buffer and compared that with the held string. It allocated for every string longer than 256 bytes: the vector runner measured 624 B for a 302-byte string that had not changed. An ASCII-only fast path in front of it did not help non-ASCII text. Encoding the held string on the fly has no length limit and needs no buffer.

### Bytes

fomoxac never reads a field's host type. The README states this rule: the wire type comes from the attribute alone. The generator therefore cannot choose a decode method per host type. Instead it emits

```csharp
var payloadValue = value.Payload;
reader.ReadBytes(ref payloadValue);
value.Payload = payloadValue;
```

and C# resolves the `ref` overload by the local's exact type:

| Host type | `ReadBytes(ref …)` |
|---|---|
| `byte[]` | Reuses the array when it has exactly the new length, otherwise allocates |
| `ArraySegment<byte>`, `Memory<byte>` | Writes into the array behind it from index 0 when that array is large enough, otherwise allocates |
| `ReadOnlyMemory<byte>` | Returns a slice of the reader's input memory. A reader built over a span copies instead |

The segment and memory overloads write from index 0, not from the segment's offset, because the usable capacity is the whole array. Anything else in that array is overwritten, so a segment over a shared buffer must not be decoded into.

The `ReadOnlyMemory<byte>` slice is valid only while the reader's input is. This is the type to use for a wrapper model's payload: the sender assigns another writer's `WrittenMemory` to it, and the receiver decodes the inner message straight from the slice. On a 63-byte wrapper carrying a 46-byte message, send went from 406 ns and 848 B to 69 ns and 0 B, and receive from 136 ns and 224 B to 54 ns and 0 B (commit `9316316`, .NET 8, 1,000,000 iterations).

### Arrays

The generated decoder calls `ArrayField.Reuse(value.Field, count)`, which returns the field's existing `List<T>` or a new one. The parameter is `IEnumerable<T>`, so type inference works when the property is declared as `List<T>`, `IList<T>` or `IReadOnlyList<T>`. Elements that already exist are decoded into, including model elements and strings. New elements are added, and `ArrayField.Trim` removes elements past the new count. Array elements that are `bytes` go through the same `ReadBytes(ref …)` overloads, so `List<ReadOnlyMemory<byte>>` works.

### Unity

The generated C# has to compile for Unity, which means `netstandard2.1` and C# 9. Every API the runtime uses is available there, including `Encoding.GetBytes(ReadOnlySpan<char>, Span<byte>)`, `MemoryMarshal.TryGetArray` and `BinaryPrimitives`. Check a runtime change by building the generated tree under that target with warnings as errors.

## Go

`Writer.Reset()` keeps the slice's capacity, and `NewWriterSize(n)` starts with room for `n` bytes. Numbers are appended with `binary.LittleEndian.AppendUint16/32/64`. A reused writer encodes DeviceState.unity in 7 ns instead of 57 ns and Team in 39 ns instead of 227 ns (commit `312fe72`).

`ReadStringInto` validates the bytes with `utf8.Valid` before comparing them with the held string. A Go string can hold invalid UTF-8, so an equal comparison proves nothing about validity. The comparison `string(bytes) != *dst` does not allocate. `ReadBytesInto` appends into `(*dst)[:0]`, reusing its capacity, and returns an empty non-nil slice for an empty blob.

`Reader.Reset(buf)` exists because `NewReader` returns a `*Reader`. A reader passed through an interface or a function value escapes to the heap, and the vector runner measured one allocation per message from that alone.

A slice field is resliced to the new count and grown with `append` within its capacity. Elements that existed before are decoded in place. Slots between the old length and the capacity start from the zero value instead of whatever an earlier, longer value left there, because the caller may have shortened the slice deliberately and still share that memory.

The generated code has to build where `int` is 32 bits. `UnlimitedLimits` uses the largest `int` there, and CI runs the vectors under `GOARCH=386`.

## Rust

`read_string_into` leaves the `String` untouched when its bytes equal the input. Otherwise it validates the input, clears the string and pushes the new text, which keeps the allocation. A `String` is always valid UTF-8, so equal bytes need no validation. `read_bytes_into` clears the `Vec<u8>` and extends it from the input.

A `Vec<T>` field is truncated to the new count and reserved up to the 4096 bound. Missing elements are pushed as `Default::default()`, and every element is decoded in place. Nested arrays are decoded the same way at every depth, with loop variables numbered by depth (`elements0`, `index1`) so the levels do not collide.

## C++

`Writer` appends each byte of a number with `push_back`. Resizing the vector once per number and then writing the bytes by index measured slower: Team went from 86 ns to 172 ns at `-O2` (commit `c282643`). That change was tried a second time during the decode-reuse work and reverted before commit. Do not reintroduce it without a new measurement that shows otherwise.

`write_string` takes a `std::string_view`, and `write_bytes` also takes a pointer and a size. `read_string` and `read_bytes` use `assign`, which reuses the target's capacity.

A `std::vector` field is cut with `erase` and extended with `emplace_back`, and each element is decoded into its existing storage. `Array<bool>` still reads into a local `bool` and assigns it, because `std::vector<bool>` has no `bool&` to decode into. The reserve is bounded at 4096 elements, so a hostile count cannot make `decode` throw `std::length_error` or `std::bad_alloc` before an element is read.

## C

C has no capacity field in `FomoxaBytes` or `FomoxaArray_T`, and `const char *` strings have no length, so reuse goes through `realloc`, which can extend or shrink a block in place.

`fomoxa_reader_read_string_into` keeps the held string when `strlen` and `memcmp` match the input, and otherwise reallocs it to the new length. `fomoxa_reader_read_bytes_into` copies in place when the length is unchanged, reallocs otherwise, and frees the buffer for an empty blob, which keeps the invariant that `data` is `NULL` exactly when `len` is 0. Both leave the target untouched on any error, including an allocation failure.

An array frees the elements past the new count (strings, blobs and nested models own memory), then grows as needed: to 8 elements first, then doubling, capped at the count read. An earlier version called `calloc(count)` with the count from the stream, which let one message request an allocation as large as the limits allowed. The growth size is checked against `SIZE_MAX / sizeof(T)` before multiplying.

Reuse makes the decode target rule explicit: a value passed to `_decode` must be zero-initialized, freed by `<Model>_free`, or filled by an earlier decode. An uninitialized struct would hand garbage pointers to `realloc`. A value stays safe to free after a failed decode, and `<Model>_free` is called once, when the value is no longer needed.

## TypeScript and JavaScript

Every new `Writer`, and every time its buffer grows, allocates an `ArrayBuffer` for its `DataView`, which costs about a microsecond on Node 24. Reusing one writer took encoding DeviceState.unity from 2627 ns to 161 ns and Team from 8005 ns to 439 ns (commit `66e98fb`). Strings are encoded with `TextEncoder.encodeInto` straight into the buffer.

`Reader.reset(bytes)` keeps the `DataView` when the new bytes cover the same buffer, offset and length, and builds a new one otherwise.

`readString(current)` compares the input with `current` by encoding `current` to UTF-8 on the fly, the same way the C# runtime does, and returns `current` without calling `TextDecoder` when they match. `readBytes(current)` copies into `current` when the length is unchanged. Blobs of up to 64 bytes are copied with a loop, and longer ones with `set(subarray(…))`. The threshold exists because `subarray` creates a view object on every call.

An array field is reused: its length is set to the new count, and each element that is already a model instance is decoded into. JavaScript has no allocation counter a test can read, so the runners check object identity instead of allocation counts.

## Measuring a runtime change

Measure before and after in the same run, on the same machine, with the same model. The numbers in this document come from scratch projects that generate a codec with the fomoxac under test, run 1,000,000 iterations after a warm-up, and read the allocation counter the platform offers: `GC.GetAllocatedBytesForCurrentThread` in .NET, `testing.AllocsPerRun` in Go, a counting `operator new` in C++. To compare against an older fomoxac, build it from a `git worktree` of that commit instead of trusting numbers from another session.

A change to a runtime template is ready when:

1. Its effect is measured, and the numbers are in the commit message.
2. The allocation check in the backend's vector runner still passes, and every other backend's runner still passes too.
3. Every committed generated tree is regenerated and `fomoxac generate --check` passes on it.
4. For C#, the generated tree builds under `netstandard2.1` and C# 9.
5. This document records the decision, and records the alternative when one was tried and dropped.
