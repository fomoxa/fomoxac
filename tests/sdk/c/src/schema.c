#include "fomoxa/schema.h"

fmx_result fmx_schema_check(const fmx_schema *schema) {
    size_t index;

    if (schema == NULL || (schema->messages == NULL && schema->message_count > 0)) {
        return FMX_ERR_INVALID;
    }
    if (schema->message_count > FMX_MAX_SCHEMA_MESSAGES) {
        return FMX_ERR_INVALID;
    }

    for (index = 0; index < schema->message_count; ++index) {
        const fmx_message_schema *message = &schema->messages[index];
        if (message->prefix_count > (size_t)UINT16_MAX) {
            return FMX_ERR_INVALID;
        }
        if (message->prefix_count > 0 && message->prefixes == NULL) {
            return FMX_ERR_INVALID;
        }
        if (message->prefix_count > 0 &&
            message->prefixes[message->prefix_count - 1] != message->fingerprint) {
            return FMX_ERR_INVALID;
        }
        if (index > 0 && schema->messages[index - 1].id >= message->id) {
            return FMX_ERR_INVALID;
        }
    }
    return FMX_OK;
}

const fmx_message_schema *fmx_schema_message(const fmx_schema *schema, uint32_t id) {
    size_t low = 0;
    size_t high;

    if (schema == NULL) {
        return NULL;
    }
    high = schema->message_count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        if (schema->messages[middle].id < id) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    if (low < schema->message_count && schema->messages[low].id == id) {
        return &schema->messages[low];
    }
    return NULL;
}

uint16_t fmx_message_field_count(const fmx_message_schema *message) {
    if (message == NULL) {
        return 0;
    }
    return (uint16_t)message->prefix_count;
}

bool fmx_message_prefix(const fmx_message_schema *message, uint16_t field_count, uint64_t *out) {
    if (message == NULL || field_count == 0 || (size_t)field_count > message->prefix_count) {
        return false;
    }
    *out = message->prefixes[(size_t)field_count - 1];
    return true;
}
