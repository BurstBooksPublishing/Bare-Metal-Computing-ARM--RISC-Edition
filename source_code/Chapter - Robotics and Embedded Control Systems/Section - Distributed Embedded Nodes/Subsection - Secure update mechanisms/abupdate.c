Production-Ready Staged Activation Function for Secure Boot Systems



Language: C with ARM (AArch64) and RISC-V System Integration


#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "crypto.h"     // VerifyEd25519(pubkey, data, len, sig)
#include "rpmb.h"       // rpmb_monotonic_read/write
#include "bootctrl.h"   // bootctrl_staging_mark, bootctrl_activate

#define MAX_IMAGE_SIZE (2 * 1024 * 1024) // 2 MiB staging
extern const uint8_t K_ROOT[32];         // provisioned public key (Ed25519)

// Return true on successful staged activation.
bool staged_activate(
    const uint8_t *image,
    size_t len,
    const uint8_t *sig,
    uint64_t version)
{
    uint64_t current = 0;

    if (!rpmb_monotonic_read(&current)) {
        return false; // secure storage failure
    }

    // Anti-rollback check
    if (!(version > current)) {
        return false;
    }

    // Signature verification (streaming if needed)
    if (!VerifyEd25519(K_ROOT, image, len, sig)) {
        return false;
    }

    // Write image to staging partition (assumed atomic per block)
    if (!bootctrl_staging_mark(image, len, version)) {
        return false;
    }

    // On coordinator activation command, perform atomic swap:
    // 1) Program boot pointer to staging (protected register)
    // 2) Update monotonic counter to new version
    // Both operations must be durable; ensure ordering via secure element.
    if (!bootctrl_activate()) {
        return false;
    }

    // Finally update monotonic counter; if this fails,
    // node must halt to avoid rollback.
    if (!rpmb_monotonic_write(version)) {
        // Attempt safe halt or enter recovery mode.
        bootctrl_force_recovery();
        return false;
    }

    return true;
}