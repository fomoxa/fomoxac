// Every primitive RFC-0002 §2 defines, in one model, in one codec - the
// TypeScript counterpart of tests/fixtures/src/models/every_primitive.rs.

// FOMOXA_MODEL
// FOMOXA_CODEC("edge")
export class EveryPrimitive {
    // FOMOXA_FIELD(bool)
    // FOMOXA_CODEC("edge")
    Flag: boolean = false;

    // FOMOXA_FIELD(i8)
    // FOMOXA_CODEC("edge")
    Tiny: number = 0;

    // FOMOXA_FIELD(u8)
    // FOMOXA_CODEC("edge")
    Byte: number = 0;

    // FOMOXA_FIELD(i16)
    // FOMOXA_CODEC("edge")
    Small: number = 0;

    // FOMOXA_FIELD(u16)
    // FOMOXA_CODEC("edge")
    Port: number = 0;

    // FOMOXA_FIELD(i32)
    // FOMOXA_CODEC("edge")
    Offset: number = 0;

    // FOMOXA_FIELD(u32)
    // FOMOXA_CODEC("edge")
    Count: number = 0;

    // FOMOXA_FIELD(i64)
    // FOMOXA_CODEC("edge")
    Delta: bigint = 0n;

    // FOMOXA_FIELD(u64)
    // FOMOXA_CODEC("edge")
    Sequence: bigint = 0n;

    // FOMOXA_FIELD(f32)
    // FOMOXA_CODEC("edge")
    Ratio: number = 0;

    // FOMOXA_FIELD(f64)
    // FOMOXA_CODEC("edge")
    Precise: number = 0;

    // FOMOXA_FIELD(string)
    // FOMOXA_CODEC("edge")
    Label: string = "";

    // FOMOXA_FIELD(bytes)
    // FOMOXA_CODEC("edge")
    Blob: Uint8Array = new Uint8Array(0);
}
