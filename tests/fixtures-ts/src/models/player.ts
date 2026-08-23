// The annotated source the TypeScript integration tests exercise - the
// TypeScript counterpart of tests/fixtures/src/models/player.rs: one model
// annotated in place, the generated codecs compiled against that same
// class, and the model RFC-0002 §9.1's version-skew tests decode.

// A field whose network type is another model.
// FOMOXA_MODEL
// FOMOXA_CODEC("edge")
export class PlayerInfo {
    // FOMOXA_FIELD(u32)
    // FOMOXA_CODEC("edge")
    Level: number = 0;
}

// The model RFC-0002 §9.1 is tested against: three fields, and a version
// that appends a fourth.
// FOMOXA_MODEL
// FOMOXA_CODEC("edge", "unity")
export class Player {
    // FOMOXA_FIELD(u32)
    // FOMOXA_CODEC("edge", "unity")
    Id: number = 0;

    // FOMOXA_FIELD(f32)
    // FOMOXA_CODEC("edge")
    X: number = 0;

    // FOMOXA_FIELD(f32)
    // FOMOXA_CODEC("edge")
    Y: number = 0;

    // A network field in no codec: it is written by none of them.
    // FOMOXA_FIELD(u32)
    Unrouted: number = 0;

    // Not a network field at all. Logic and caches stay off the wire.
    Cache: string = "";
}

// Composites: an array of primitives, an array of models, and a nested
// model.
// FOMOXA_MODEL
// FOMOXA_CODEC("edge")
export class Team {
    // FOMOXA_FIELD(PlayerInfo)
    // FOMOXA_CODEC("edge")
    Captain: PlayerInfo = new PlayerInfo();

    // FOMOXA_FIELD(Array<string>)
    // FOMOXA_CODEC("edge")
    Tags: string[] = [];

    // FOMOXA_FIELD(Array<u32>)
    // FOMOXA_CODEC("edge")
    Scores: number[] = [];

    // FOMOXA_FIELD(Array<PlayerInfo>)
    // FOMOXA_CODEC("edge")
    Roster: PlayerInfo[] = [];
}
