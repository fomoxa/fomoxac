// The brief's own example (issue.md §1), verified parsing correctly and
// generating a working codec - see tests/cli.rs's
// `js_the_brief_s_device_state_example_parses_and_generates`.

// FOMOXA_MODEL
// FOMOXA_CODEC("edge", "unity")
export class DeviceState {
    // FOMOXA_FIELD(u32)
    // FOMOXA_CODEC("edge", "unity")
    Id = 0;

    // FOMOXA_FIELD(f32)
    // FOMOXA_CODEC("edge")
    Temperature = 0;

    // FOMOXA_FIELD(string)
    // FOMOXA_CODEC("unity")
    DisplayName = "";
}

// Codec names the generator has never heard of, and the PascalCase they
// turn into - and a 64-bit field, which is a `bigint` here and nowhere else
// in this fixture.
// FOMOXA_MODEL
// FOMOXA_CODEC("edge", "orange_pi")
export class Telemetry {
    // FOMOXA_FIELD(u64)
    // FOMOXA_CODEC("edge", "orange_pi")
    Sequence = 0n;
}
