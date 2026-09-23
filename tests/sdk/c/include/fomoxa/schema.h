#ifndef FOMOXA_SCHEMA_H
#define FOMOXA_SCHEMA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fomoxa/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FMX_MAX_SCHEMA_MESSAGES ((size_t)1000000)

typedef struct fmx_message_schema {
    uint32_t id;
    uint64_t fingerprint;
    const uint64_t *prefixes;
    size_t prefix_count;
} fmx_message_schema;

typedef struct fmx_schema {
    uint64_t fingerprint;
    const fmx_message_schema *messages;
    size_t message_count;
} fmx_schema;

fmx_result fmx_schema_check(const fmx_schema *schema);
const fmx_message_schema *fmx_schema_message(const fmx_schema *schema, uint32_t id);
uint16_t fmx_message_field_count(const fmx_message_schema *message);
bool fmx_message_prefix(const fmx_message_schema *message, uint16_t field_count, uint64_t *out);

#ifdef __cplusplus
}
#endif

#endif /* FOMOXA_SCHEMA_H */
