// The annotated source the JavaScript integration tests exercise - the
// JavaScript counterpart of tests/fixtures-ts/src/models/player.ts, with
// every type annotation dropped: the same annotations mean the same thing
// in both languages (issue.md §9).

// A field whose network type is another model.
// FOMOXA_MODEL
// FOMOXA_CODEC("edge")
export class PlayerInfo {
    // FOMOXA_FIELD(u32)
    // FOMOXA_CODEC("edge")
    Level = 0;
}

// The model RFC-0002 §9.1 is tested against: three fields, and a version
// that appends a fourth.
// FOMOXA_MODEL
// FOMOXA_CODEC("edge", "unity")
export class Player {
    // FOMOXA_FIELD(u32)
    // FOMOXA_CODEC("edge", "unity")
    Id = 0;

    // FOMOXA_FIELD(f32)
    // FOMOXA_CODEC("edge")
    X = 0;

    // FOMOXA_FIELD(f32)
    // FOMOXA_CODEC("edge")
    Y = 0;

    // A network field in no codec: it is written by none of them.
    // FOMOXA_FIELD(u32)
    Unrouted = 0;

    // Not a network field at all. Logic and caches stay off the wire.
    Cache = "";
}

// Composites: an array of primitives, an array of models, and a nested
// model.
// FOMOXA_MODEL
// FOMOXA_CODEC("edge")
export class Team {
    // FOMOXA_FIELD(PlayerInfo)
    // FOMOXA_CODEC("edge")
    Captain = new PlayerInfo();

    // FOMOXA_FIELD(Array<string>)
    // FOMOXA_CODEC("edge")
    Tags = [];

    // FOMOXA_FIELD(Array<u32>)
    // FOMOXA_CODEC("edge")
    Scores = [];

    // FOMOXA_FIELD(Array<PlayerInfo>)
    // FOMOXA_CODEC("edge")
    Roster = [];
}
