"use strict";

class SchemaError extends Error {
    constructor(message) {
        super(message);
        this.name = "SchemaError";
    }
}

function buildSchema(fingerprint, messages) {
    const byId = new Map();
    let previousId = -1;

    for (const message of messages) {
        if (message.id <= previousId) {
            throw new SchemaError("messages must be sorted by id, without duplicates");
        }
        previousId = message.id;

        if (message.prefixes.length > 0xffff) {
            throw new SchemaError(`message ${message.id} declares more than 65535 fields`);
        }
        if (
            message.prefixes.length > 0 &&
            message.prefixes[message.prefixes.length - 1] !== message.fingerprint
        ) {
            throw new SchemaError(
                `message ${message.id}: the last prefix must equal the message fingerprint`,
            );
        }
        byId.set(message.id, message);
    }

    return { fingerprint, messages, byId };
}

exports.SchemaError = SchemaError;
exports.buildSchema = buildSchema;
