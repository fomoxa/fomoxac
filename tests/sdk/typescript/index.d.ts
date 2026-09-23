export interface MessageSchema {
    readonly id: number;
    readonly fingerprint: bigint;
    readonly prefixes: readonly bigint[];
}

export interface Schema {
    readonly fingerprint: bigint;
    readonly messages: readonly MessageSchema[];
    readonly byId: ReadonlyMap<number, MessageSchema>;
}

export declare class SchemaError extends Error {
    constructor(message: string);
}

export declare function buildSchema(
    fingerprint: bigint,
    messages: readonly MessageSchema[],
): Schema;
