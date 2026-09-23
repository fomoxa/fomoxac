import * as fs from "node:fs";
import * as path from "node:path";

import { Player, Team } from "./src/models/player";
import { DeviceState } from "./src/models/device_state";
import { EveryPrimitive } from "./src/models/every_primitive";
import { DecodeError, Reader, Writer } from "./src/generated/runtime";
import { PlayerEdgeCodec } from "./src/generated/player_edge";
import { PlayerInfoEdgeCodec } from "./src/generated/player_info_edge";
import { TeamEdgeCodec } from "./src/generated/team_edge";
import { DeviceStateEdgeCodec } from "./src/generated/device_state_edge";
import { DeviceStateUnityCodec } from "./src/generated/device_state_unity";
import { EveryPrimitiveEdgeCodec } from "./src/generated/every_primitive_edge";
import { FOMOXA_MESSAGES, FOMOXA_SCHEMA_FINGERPRINT } from "./src/generated/handshake";
import { fomoxaNetSchema } from "./src/generated/net_schema";

interface Codec<T> {
    MESSAGE_ID: number;
    FINGERPRINT: bigint;
    encode(writer: Writer, value: T): void;
    decode(reader: Reader, value: T): void;
}

interface Vector {
    name: string;
    message: string;
    hex: string;
    reencode?: string;
    error?: string;
}

const sharedWriter = new Writer(1);

function roundTrip<T>(codec: Codec<T>, create: () => T): (payload: Uint8Array) => Uint8Array {
    return (payload) => {
        const value = create();
        codec.decode(new Reader(payload), value);
        const writer = new Writer();
        codec.encode(writer, value);
        return writer.toUint8Array();
    };
}

function sharedRoundTrip<T>(codec: Codec<T>, create: () => T): (payload: Uint8Array) => Uint8Array {
    return (payload) => {
        const value = create();
        codec.decode(new Reader(payload), value);
        sharedWriter.clear();
        codec.encode(sharedWriter, value);
        return sharedWriter.writtenView();
    };
}

const roundTrips: Record<string, (payload: Uint8Array) => Uint8Array> = {
    "Player.edge": roundTrip(PlayerEdgeCodec, () => new Player()),
    "DeviceState.edge": roundTrip(DeviceStateEdgeCodec, () => new DeviceState()),
    "DeviceState.unity": roundTrip(DeviceStateUnityCodec, () => new DeviceState()),
    "EveryPrimitive.edge": roundTrip(EveryPrimitiveEdgeCodec, () => new EveryPrimitive()),
    "Team.edge": roundTrip(TeamEdgeCodec, () => new Team()),
};

const sharedRoundTrips: Record<string, (payload: Uint8Array) => Uint8Array> = {
    "Player.edge": sharedRoundTrip(PlayerEdgeCodec, () => new Player()),
    "DeviceState.edge": sharedRoundTrip(DeviceStateEdgeCodec, () => new DeviceState()),
    "DeviceState.unity": sharedRoundTrip(DeviceStateUnityCodec, () => new DeviceState()),
    "EveryPrimitive.edge": sharedRoundTrip(EveryPrimitiveEdgeCodec, () => new EveryPrimitive()),
    "Team.edge": sharedRoundTrip(TeamEdgeCodec, () => new Team()),
};

const identities: Record<string, { MESSAGE_ID: number; FINGERPRINT: bigint }> = {
    "Player.edge": PlayerEdgeCodec,
    "PlayerInfo.edge": PlayerInfoEdgeCodec,
    "DeviceState.edge": DeviceStateEdgeCodec,
    "DeviceState.unity": DeviceStateUnityCodec,
    "EveryPrimitive.edge": EveryPrimitiveEdgeCodec,
    "Team.edge": TeamEdgeCodec,
};

const errorPrefixes: Record<string, string> = {
    UnexpectedEof: "unexpected eof",
    InvalidBool: "invalid bool",
    InvalidUtf8: "invalid utf-8",
    LengthOverflow: "length overflow",
};

let checks = 0;
let failures = 0;

function check(passed: boolean, failure: string): void {
    checks += 1;
    if (!passed) {
        failures += 1;
        console.log(`FAIL ${failure}`);
    }
}

function hex(text: string): Uint8Array {
    return Uint8Array.from(Buffer.from(text.replace(/\s+/g, ""), "hex"));
}

function toHex(bytes: Uint8Array): string {
    return Buffer.from(bytes).toString("hex");
}

function describe(error: unknown): string {
    return error instanceof Error ? `${error.name}: ${error.message}` : String(error);
}

const vectorsPath = process.argv[2] ?? path.join(__dirname, "..", "..", "vectors", "fomoxa-vectors.json");
const vectors = JSON.parse(fs.readFileSync(vectorsPath, "utf8"));

for (const [name, codec] of Object.entries(identities)) {
    const message = vectors.messages[name];
    const id = Number.parseInt(message.id.slice(2), 16);
    const fingerprint = BigInt(`0x${message.fingerprint.slice("sha256:".length, "sha256:".length + 16)}`);
    check(codec.MESSAGE_ID === id, `${name}: message id ${codec.MESSAGE_ID.toString(16)}, vectors say ${id.toString(16)}`);
    check(codec.FINGERPRINT === fingerprint, `${name}: fingerprint ${codec.FINGERPRINT.toString(16)}, vectors say ${fingerprint.toString(16)}`);
}

for (const vector of vectors.accept as Vector[]) {
    const expected = hex(vector.reencode ?? "");
    try {
        const actual = roundTrips[vector.message](hex(vector.hex));
        check(toHex(actual) === toHex(expected), `accept ${vector.name}: re-encoded ${toHex(actual)}, expected ${toHex(expected)}`);
    } catch (error) {
        check(false, `accept ${vector.name}: ${describe(error)}`);
    }
}

for (const vector of vectors.reject as Vector[]) {
    const expected = vector.error ?? "";
    try {
        roundTrips[vector.message](hex(vector.hex));
        check(false, `reject ${vector.name}: decoded, expected ${expected}`);
    } catch (error) {
        const matched = error instanceof DecodeError && error.message.startsWith(errorPrefixes[expected]);
        check(matched, `reject ${vector.name}: ${describe(error)}, expected DecodeError ${expected}`);
    }
}

for (const vector of vectors.accept as Vector[]) {
    const expected = toHex(roundTrips[vector.message](hex(vector.hex)));
    const actual = toHex(sharedRoundTrips[vector.message](hex(vector.hex)));
    check(actual === expected, `shared writer ${vector.name}: ${actual}, expected ${expected}`);
}

function fingerprint64(tagged: string): bigint {
    return BigInt(`0x${tagged.slice("sha256:".length, "sha256:".length + 16)}`);
}

try {
    const schema = fomoxaNetSchema();
    check(schema.fingerprint === FOMOXA_SCHEMA_FINGERPRINT, `net schema: fingerprint ${schema.fingerprint.toString(16)}, handshake says ${FOMOXA_SCHEMA_FINGERPRINT.toString(16)}`);
    check(schema.messages.length === FOMOXA_MESSAGES.length, `net schema: ${schema.messages.length} messages, handshake declares ${FOMOXA_MESSAGES.length}`);
    for (const [name, message] of Object.entries(vectors.messages) as [string, { id: string; fingerprint: string; prefixes: string[] }][]) {
        const found = schema.byId.get(Number.parseInt(message.id.slice(2), 16));
        const expected = message.prefixes.map(fingerprint64);
        const matches =
            found !== undefined &&
            found.fingerprint === fingerprint64(message.fingerprint) &&
            found.prefixes.length === expected.length &&
            found.prefixes.every((prefix, index) => prefix === expected[index]);
        check(matches, `net schema: ${name} does not match the vectors`);
    }
} catch (error) {
    check(false, `net schema: ${describe(error)}`);
}

console.log(`${checks - failures}/${checks} checks passed`);
process.exitCode = failures === 0 ? 0 : 1;
