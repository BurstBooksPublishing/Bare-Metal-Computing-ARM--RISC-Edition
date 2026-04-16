\begin{figure}[ht]
\centering
\caption{Production-Ready Heartbeat Monitor Implementation in C for ARM (AArch64) Systems}
\end{figure}

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>

typedef struct {
    uint32_t seq;
    uint32_t ts;
    uint16_t crc;
} __attribute__((packed)) hb_msg_t;

#define GAP_THRESHOLD           3
#define BYZANTINE_THRESHOLD     2
#define HEARTBEAT_TIMEOUT_MS    50

static _Atomic(int) node_state[NUM_NODES];
static uint32_t last_seq[NUM_NODES];
static uint8_t crc_fail_count[NUM_NODES];
static uint32_t last_rx_time_ms[NUM_NODES];

static uint16_t crc16_ccitt(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; ++b) {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
        }
    }
    return crc;
}

void process_heartbeat(int node, const hb_msg_t *m, uint32_t now_ms) {
    uint16_t expected = crc16_ccitt((const uint8_t*)m, sizeof(*m) - sizeof(m->crc));
    if (expected != m->crc) {
        if (++crc_fail_count[node] >= BYZANTINE_THRESHOLD) {
            atomic_store(&node_state[node], 2);
        }
        return;
    }
    crc_fail_count[node] = 0;

    uint32_t gap = m->seq - last_seq[node];
    if (last_seq[node] && gap > 1 && gap <= GAP_THRESHOLD) {
        atomic_store(&node_state[node], 1);
    } else if (gap > GAP_THRESHOLD) {
        atomic_store(&node_state[node], 2);
    } else {
        atomic_store(&node_state[node], 0);
    }

    last_seq[node] = m->seq;
    last_rx_time_ms[node] = now_ms;
}

void monitor_timeouts(uint32_t now_ms) {
    for (int n = 0; n < NUM_NODES; ++n) {
        if (now_ms - last_rx_time_ms[n] > HEARTBEAT_TIMEOUT_MS) {
            atomic_store(&node_state[n], 2);
        }
    }
}