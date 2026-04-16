AArch64 Critical Section Implementation


    .text
    .global critical_section

critical_section:
    mrs     x0, DAIF           // Save current interrupt mask state
    msr     DAIFSet, #0x2      // Set I bit (bit 1): disable IRQ interrupts
    dmb     ish                // Ensure memory accesses complete before critical section
    // -- critical protected work --
    bl      do_critical_work   // Execute the critical section work
    // -- end protected work --
    isb                        // Ensure PSTATE changes are visible before restoring
    msr     DAIF, x0           // Restore original interrupt mask state exactly
    ret                        // Return to caller