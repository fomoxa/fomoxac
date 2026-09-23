use std::fmt;

use crate::MAX_MESSAGES;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MessageSchema {
    id: u32,
    fingerprint: u64,
    prefixes: Vec<u64>,
}

impl MessageSchema {
    pub fn new(id: u32, fingerprint: u64, prefixes: impl Into<Vec<u64>>) -> MessageSchema {
        MessageSchema { id, fingerprint, prefixes: prefixes.into() }
    }

    pub fn id(&self) -> u32 {
        self.id
    }

    pub fn fingerprint(&self) -> u64 {
        self.fingerprint
    }

    pub fn field_count(&self) -> u16 {
        self.prefixes.len() as u16
    }

    pub fn prefixes(&self) -> &[u64] {
        &self.prefixes
    }

    pub fn prefix(&self, field_count: u16) -> Option<u64> {
        if field_count == 0 {
            return None;
        }
        self.prefixes.get(usize::from(field_count) - 1).copied()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SchemaError {
    DuplicateMessage(u32),
    TooManyMessages(usize),
    TooManyFields { id: u32, count: usize },
    PrefixMismatch { id: u32 },
}

impl fmt::Display for SchemaError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            SchemaError::DuplicateMessage(id) => {
                write!(f, "message id 0x{id:08X} is declared twice")
            }
            SchemaError::TooManyMessages(count) => {
                write!(f, "{count} messages exceeds the {MAX_MESSAGES} a hello may declare")
            }
            SchemaError::TooManyFields { id, count } => {
                write!(f, "message 0x{id:08X} has {count} fields, more than a u16 can carry")
            }
            SchemaError::PrefixMismatch { id } => write!(
                f,
                "message 0x{id:08X}: the last prefix fingerprint is not the message fingerprint"
            ),
        }
    }
}

impl std::error::Error for SchemaError {}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Schema {
    fingerprint: u64,
    messages: Vec<MessageSchema>,
}

impl Schema {
    pub fn new(
        fingerprint: u64,
        messages: impl Into<Vec<MessageSchema>>,
    ) -> Result<Schema, SchemaError> {
        let mut messages = messages.into();
        if messages.len() > MAX_MESSAGES {
            return Err(SchemaError::TooManyMessages(messages.len()));
        }
        for message in &messages {
            if message.prefixes.len() > usize::from(u16::MAX) {
                return Err(SchemaError::TooManyFields {
                    id: message.id,
                    count: message.prefixes.len(),
                });
            }
            match message.prefixes.last() {
                Some(last) if *last != message.fingerprint => {
                    return Err(SchemaError::PrefixMismatch { id: message.id })
                }
                _ => {}
            }
        }
        messages.sort_by_key(MessageSchema::id);
        if let Some(window) = messages.windows(2).find(|pair| pair[0].id == pair[1].id) {
            return Err(SchemaError::DuplicateMessage(window[0].id));
        }
        Ok(Schema { fingerprint, messages })
    }

    pub fn fingerprint(&self) -> u64 {
        self.fingerprint
    }

    pub fn messages(&self) -> &[MessageSchema] {
        &self.messages
    }

    pub fn message(&self, id: u32) -> Option<&MessageSchema> {
        let index = self.messages.binary_search_by_key(&id, MessageSchema::id).ok()?;
        Some(&self.messages[index])
    }
}
