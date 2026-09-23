#ifndef FOMOXA_COMMON_H
#define FOMOXA_COMMON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum fmx_result {
    FMX_OK = 0,
    FMX_ERR_NOT_READY = -1,
    FMX_ERR_CONGESTED = -2,
    FMX_ERR_TOO_LARGE = -3,
    FMX_ERR_CLOSED = -4,
    FMX_ERR_NO_MEMORY = -5,
    FMX_ERR_INVALID = -6
} fmx_result;

const char *fmx_result_name(fmx_result result);

typedef struct fmx_config {
    uint32_t handshake_timeout_ms;
    uint32_t heartbeat_interval_ms;
    uint32_t heartbeat_timeout_ms;
    uint32_t max_frames_per_tick;
    uint32_t max_message_bytes;
    uint32_t max_peers;
} fmx_config;

void fmx_config_defaults(fmx_config *config);

uint64_t fmx_now_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* FOMOXA_COMMON_H */
