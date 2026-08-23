// Every primitive RFC-0002 §2 defines, in one model, in one codec - the
// JavaScript counterpart of tests/fixtures-ts/src/models/every_primitive.ts.

// FOMOXA_MODEL
// FOMOXA_CODEC("edge")
export class EveryPrimitive {
    // FOMOXA_FIELD(bool)
    // FOMOXA_CODEC("edge")
    Flag = false;

    // FOMOXA_FIELD(i8)
    // FOMOXA_CODEC("edge")
    Tiny = 0;

    // FOMOXA_FIELD(u8)
    // FOMOXA_CODEC("edge")
    Byte = 0;

    // FOMOXA_FIELD(i16)
    // FOMOXA_CODEC("edge")
    Small = 0;

    // FOMOXA_FIELD(u16)
    // FOMOXA_CODEC("edge")
    Port = 0;

    // FOMOXA_FIELD(i32)
    // FOMOXA_CODEC("edge")
    Offset = 0;

    // FOMOXA_FIELD(u32)
    // FOMOXA_CODEC("edge")
    Count = 0;

    // FOMOXA_FIELD(i64)
    // FOMOXA_CODEC("edge")
    Delta = 0n;

    // FOMOXA_FIELD(u64)
    // FOMOXA_CODEC("edge")
    Sequence = 0n;

    // FOMOXA_FIELD(f32)
    // FOMOXA_CODEC("edge")
    Ratio = 0;

    // FOMOXA_FIELD(f64)
    // FOMOXA_CODEC("edge")
    Precise = 0;

    // FOMOXA_FIELD(string)
    // FOMOXA_CODEC("edge")
    Label = "";

    // FOMOXA_FIELD(bytes)
    // FOMOXA_CODEC("edge")
    Blob = new Uint8Array(0);
}
