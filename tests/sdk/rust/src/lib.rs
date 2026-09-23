pub mod schema;

pub use schema::{MessageSchema, Schema, SchemaError};

pub const MAX_MESSAGES: usize = 1_000_000;
