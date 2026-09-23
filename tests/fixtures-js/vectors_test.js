import * as fs from "node:fs";
import * as path from "node:path";
import { fileURLToPath } from "node:url";

import { Player, Team } from "./src/models/player.js";
import { DeviceState } from "./src/models/device_state.js";
import { EveryPrimitive } from "./src/models/every_primitive.js";
import { DecodeError, Reader, Writer } from "./src/generated/runtime.js";
import { PlayerEdgeCodec } from "./src/generated/player_edge.js";
import { PlayerInfoEdgeCodec } from "./src/generated/player_info_edge.js";
import { TeamEdgeCodec } from "./src/generated/team_edge.js";
import { DeviceStateEdgeCodec } from "./src/generated/device_state_edge.js";
import { DeviceStateUnityCodec } from "./src/generated/device_state_unity.js";
import { EveryPrimitiveEdgeCodec } from "./src/generated/every_primitive_edge.js";

const sharedWriter = new Writer(1);

function roundTrip(codec, create) {
    return (payload) => {
        const value = create();
        codec.decode(new Reader(payload), value);
        const writer = new Writer();
        codec.encode(writer, value);
        return writer.toUint8Array();
    };
}

function sharedRoundTrip(codec, create) {
    return (payload) => {
        const value = create();
        codec.decode(new Reader(payload), value);
        sharedWriter.clear();
        codec.encode(sharedWriter, value);
        return sharedWriter.writtenView();
    };
}

const roundTrips = {
    "Player.edge": roundTrip(PlayerEdgeCodec, () => new Player()),
    "DeviceState.edge": roundTrip(DeviceStateEdgeCodec, () => new DeviceState()),
    "DeviceState.unity": roundTrip(DeviceStateUnityCodec, () => new DeviceState()),
    "EveryPrimitive.edge": roundTrip(EveryPrimitiveEdgeCodec, () => new EveryPrimitive()),
    "Team.edge": roundTrip(TeamEdgeCodec, () => new Team()),
};

const sharedRoundTrips = {
    "Player.edge": sharedRoundTrip(PlayerEdgeCodec, () => new Player()),
    "DeviceState.edge": sharedRoundTrip(DeviceStateEdgeCodec, () => new DeviceState()),
    "DeviceState.unity": sharedRoundTrip(DeviceStateUnityCodec, () => new DeviceState()),
    "EveryPrimitive.edge": sharedRoundTrip(EveryPrimitiveEdgeCodec, () => new EveryPrimitive()),
    "Team.edge": sharedRoundTrip(TeamEdgeCodec, () => new Team()),
};

const identities = {
    "Player.edge": PlayerEdgeCodec,
    "PlayerInfo.edge": PlayerInfoEdgeCodec,
    "DeviceState.edge": DeviceStateEdgeCodec,
    "DeviceState.unity": DeviceStateUnityCodec,
    "EveryPrimitive.edge": EveryPrimitiveEdgeCodec,
    "Team.edge": TeamEdgeCodec,
};

const errorPrefixes = {
    UnexpectedEof: "unexpected eof",
    InvalidBool: "invalid bool",
    InvalidUtf8: "invalid utf-8",
    LengthOverflow: "length overflow",
};

let checks = 0;
let failures = 0;

function check(passed, failure) {
    checks += 1;
    if (!passed) {
        failures += 1;
        console.log(`FAIL ${failure}`);
    }
}

function hex(text) {
    return Uint8Array.from(Buffer.from(text.replace(/\s+/g, ""), "hex"));
}

function toHex(bytes) {
    return Buffer.from(bytes).toString("hex");
}

function describe(error) {
    return error instanceof Error ? `${error.name}: ${error.message}` : String(error);
}

const here = path.dirname(fileURLToPath(import.meta.url));
const vectorsPath = process.argv[2] ?? path.join(here, "..", "vectors", "fomoxa-vectors.json");
const vectors = JSON.parse(fs.readFileSync(vectorsPath, "utf8"));

for (const [name, codec] of Object.entries(identities)) {
    const message = vectors.messages[name];
    const id = Number.parseInt(message.id.slice(2), 16);
    const fingerprint = BigInt(`0x${message.fingerprint.slice("sha256:".length, "sha256:".length + 16)}`);
    check(codec.MESSAGE_ID === id, `${name}: message id ${codec.MESSAGE_ID.toString(16)}, vectors say ${id.toString(16)}`);
    check(codec.FINGERPRINT === fingerprint, `${name}: fingerprint ${codec.FINGERPRINT.toString(16)}, vectors say ${fingerprint.toString(16)}`);
}

for (const vector of vectors.accept) {
    const expected = hex(vector.reencode ?? "");
    try {
        const actual = roundTrips[vector.message](hex(vector.hex));
        check(toHex(actual) === toHex(expected), `accept ${vector.name}: re-encoded ${toHex(actual)}, expected ${toHex(expected)}`);
    } catch (error) {
        check(false, `accept ${vector.name}: ${describe(error)}`);
    }
}

for (const vector of vectors.reject) {
    try {
        roundTrips[vector.message](hex(vector.hex));
        check(false, `reject ${vector.name}: decoded, expected ${vector.error}`);
    } catch (error) {
        const matched = error instanceof DecodeError && error.message.startsWith(errorPrefixes[vector.error]);
        check(matched, `reject ${vector.name}: ${describe(error)}, expected DecodeError ${vector.error}`);
    }
}

for (const vector of vectors.accept) {
    const expected = toHex(roundTrips[vector.message](hex(vector.hex)));
    const actual = toHex(sharedRoundTrips[vector.message](hex(vector.hex)));
    check(actual === expected, `shared writer ${vector.name}: ${actual}, expected ${expected}`);
}

console.log(`${checks - failures}/${checks} checks passed`);
process.exitCode = failures === 0 ? 0 : 1;
