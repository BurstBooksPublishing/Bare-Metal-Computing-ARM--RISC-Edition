Production-Ready Frame Processing Module for ARM (AArch64)


#include <stdint.h>
#include <stddef.h>

// Configuration constants
#define FRAME_MAX_PAYLOAD 64
#define RING_BUF_SIZE 512

// Application callback prototype (implemented by user)
void frame_received(uint8_t type_id, uint8_t *payload, uint8_t len);

// CRC16-CCITT (0x1021) implementation, table-free, small code.
static uint16_t crc16_ccitt(const uint8_t *data, size_t len, uint16_t init) {
    uint16_t crc = init;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; ++b) {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
        }
    }
    return crc;
}

// Ring buffer written by DMA; head updated in IRQ or polled from DMA count register.
static volatile uint8_t ring_buf[RING_BUF_SIZE];
static volatile size_t ring_head = 0; // updated by DMA completion/counter
static size_t proc_tail = 0;          // processing pointer

// Sequence state
static uint8_t expected_seq = 0;

// Helper: read contiguous bytes from ring buffer with wrap.
static void ring_read(size_t idx, uint8_t *dst, size_t len) {
    while (len--) {
        *dst++ = ring_buf[idx++];
        if (idx >= RING_BUF_SIZE) {
            idx = 0;
        }
    }
}

// Call periodically from main loop or a low priority task to process frames.
void process_incoming_frames(void) {
    // Obtain a consistent head snapshot (read DMA counter / head in IRQ).
    size_t head = ring_head;

    while (proc_tail != head) {
        // Need at least header (3 bytes) and CRC (2 bytes)
        size_t available = (head + RING_BUF_SIZE - proc_tail) % RING_BUF_SIZE;
        if (available < 5) {
            break;
        }

        uint8_t header[3];
        ring_read(proc_tail, header, 3);
        uint8_t type_id = header[0];
        uint8_t plen = header[1];
        uint8_t seq = header[2];

        if (plen > FRAME_MAX_PAYLOAD) {
            // Desync: drop one byte and resync
            proc_tail = (proc_tail + 1) % RING_BUF_SIZE;
            continue;
        }

        size_t frame_total = 3 + plen + 2;
        if (available < frame_total) {
            break; // wait for full frame
        }

        uint8_t payload[FRAME_MAX_PAYLOAD];
        ring_read((proc_tail + 3) % RING_BUF_SIZE, payload, plen);

        uint8_t crc_bytes[2];
        ring_read((proc_tail + 3 + plen) % RING_BUF_SIZE, crc_bytes, 2);
        uint16_t recv_crc = (uint16_t)crc_bytes[0] << 8 | crc_bytes[1];

        // Compute CRC over header + payload
        uint8_t crc_buf[3 + FRAME_MAX_PAYLOAD];
        ring_read(proc_tail, crc_buf, 3);
        for (uint8_t i = 0; i < plen; ++i) {
            crc_buf[3 + i] = payload[i];
        }
        uint16_t calc = crc16_ccitt(crc_buf, 3 + plen, 0xFFFF);

        if (calc != recv_crc) {
            // CRC error: advance past this frame and continue
            proc_tail = (proc_tail + frame_total) % RING_BUF_SIZE;
            continue;
        }

        // Sequence check: if mismatch, application may request recovery.
        if (seq == expected_seq) {
            ++expected_seq;
            frame_received(type_id, payload, plen); // user callback
        } else {
            // Sequence mismatch: notify or attempt resync; still deliver.
            expected_seq = seq + 1;
            frame_received(type_id, payload, plen);
        }

        proc_tail = (proc_tail + frame_total) % RING_BUF_SIZE;
    }
}