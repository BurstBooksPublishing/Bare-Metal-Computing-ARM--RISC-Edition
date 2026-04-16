# Bare Metal Computing Arm  Risc Edition

### Cover
<img src="covers/Front4.png" alt="Book Cover" width="300" style="max-width: 100%; height: auto; border-radius: 6px; box-shadow: 0 3px 8px rgba(0,0,0,0.1);"/>

### Repository Structure
- `covers/`: Book cover images
- `blurbs/`: Promotional blurbs
- `infographics/`: Marketing visuals
- `source_code/`: Code samples
- `manuscript/`: Drafts and format.txt for TOC
- `marketing/`: Ads and press releases
- `additional_resources/`: Extras

View the live site at [burstbookspublishing.github.io/bare-metal-computing-arm--risc-edition](https://burstbookspublishing.github.io/bare-metal-computing-arm--risc-edition/)
---

- Bare Metal Computing ARM & RISC Edition
- Cross Architecture Instruction Level Engineering
- ARM & RISC-V Edition

---
## Chapter 1. Processor Execution Models in ARM and RISC-V
### Section 1. Instruction Set Architecture Fundamentals
- Fixed-length vs variable-length encoding
- Load-store architecture principles
- General-purpose register files
- Condition flags and status registers
- System and privileged register classes
- Exception level hierarchy (EL0–EL3 / M/S/U modes)

### Section 2. Privilege and Execution Levels
- ARM exception levels (EL0–EL3)
- RISC-V machine, supervisor, and user modes
- Secure vs non-secure world (ARM TrustZone)
- Delegation mechanisms (medeleg / mideleg)
- Trap return sequences
- System register access control

### Section 3. Microarchitectural Considerations
- Pipeline behavior and hazard handling
- Branch prediction mechanisms
- Cache hierarchies and coherency
- Memory ordering models
- Performance monitoring units (PMU)
- Power and frequency scaling implications

---
## Chapter 2. Boot and Control Transfer on ARM and RISC-V
### Section 1. Reset State and Firmware
- ARM reset vector behavior
- RISC-V reset entry conventions
- Boot ROM and SoC initialization
- Device tree fundamentals
- Secure monitor involvement (ARM)
- SBI (Supervisor Binary Interface) in RISC-V

### Section 2. Early Execution Environment Setup
- Stack initialization
- Clearing BSS and initializing data
- Setting exception vector base (VBAR / mtvec)
- Configuring system control registers
- Enabling caches and MMU
- Transitioning to higher exception levels

### Section 3. Establishing Full Bare-Metal Control
- Exiting firmware and boot services
- Setting up page tables (ARM TTBR / RISC-V satp)
- Exception delegation configuration
- Interrupt controller initialization
- Verifying privilege configuration
- Hand-off to runtime environment

---
## Chapter 3. Memory Architecture and Address Translation
### Section 1. Physical and Virtual Memory Models
- ARM translation table hierarchy
- RISC-V Sv39 / Sv48 page structures
- Address space layout design
- Device vs normal memory attributes
- Page permissions and execute-never bits
- Large page mappings

### Section 2. MMU Configuration and Control
- Translation table base registers (TTBR0/TTBR1)
- satp register configuration
- Enabling address translation
- TLB management and invalidation
- Cache maintenance operations
- Memory barrier instructions

### Section 3. Deterministic Memory Behavior
- Cache maintenance patterns
- Avoiding aliasing hazards
- Managing DMA coherency
- Controlling speculative execution
- Interrupt masking and priority configuration
- Ensuring predictable access timing

---
## Chapter 4. Interrupts and Exception Handling
### Section 1. Exception Vector Configuration
- ARM vector table structure
- RISC-V trap vector modes
- Setting VBAR and mtvec
- Exception entry sequences
- Context save and restore patterns
- Nested exception handling

### Section 2. Interrupt Controllers
- ARM Generic Interrupt Controller (GIC)
- RISC-V Platform-Level Interrupt Controller (PLIC)
- Local interrupt sources
- Interrupt priority and masking
- Inter-processor interrupts
- Deterministic interrupt routing

### Section 3. Fault Diagnosis and Recovery
- Decoding syndrome registers (ARM ESR)
- RISC-V trap cause register usage
- Page fault analysis
- Illegal instruction handling
- Safe recovery sequences
- Structured crash reporting

---
## Chapter 5. Direct Hardware and SoC Integration
### Section 1. Memory-Mapped I/O
- Peripheral register access patterns
- Volatile access and ordering rules
- Peripheral clock configuration
- GPIO initialization
- Timer and UART configuration
- Safe hardware probing

### Section 2. DMA and Accelerator Integration
- Configuring DMA controllers
- Cache coherency considerations
- Synchronization barriers
- Accelerator invocation patterns
- Shared memory regions
- Interrupt-driven completion

### Section 3. Device Tree and Platform Discovery
- Flattened Device Tree (FDT) structure
- Parsing hardware descriptions
- Peripheral address extraction
- Interrupt mapping resolution
- Multi-core topology discovery
- Platform abstraction strategies

---
## Chapter 6. Timing and Deterministic Scheduling
### Section 1. Timer Subsystems
- ARM generic timer
- RISC-V timer interrupts
- High-resolution timing strategies
- Reading cycle counters
- Calibrating timing sources
- Interrupt latency measurement

### Section 2. Bare-Metal Task Scheduling
- Cooperative scheduling
- Preemptive scheduling via timer interrupt
- Context switch mechanics
- Register save/restore patterns
- Stack allocation models
- Multi-core coordination

### Section 3. Real-Time Guarantees
- Priority-based scheduling
- Lock-free synchronization
- Spinlock and atomic primitives
- Memory ordering enforcement
- Jitter minimization
- Deadline enforcement patterns

---
## Chapter 7. AI as Machine Code Author on ARM and RISC-V
### Section 1. Architecture-Aware Code Generation
- Constraint declaration for AArch64
- Constraint declaration for RV64
- Register allocation awareness
- Instruction selection for load-store ISAs
- Mode-safe generation patterns
- Vector extension considerations (NEON / RVV)

### Section 2. Validation and Inspection
- Disassembly verification workflows
- Control flow graph inspection
- Privilege compliance checks
- Exception safety validation
- Performance counter validation
- Deterministic replay testing

### Section 3. Safe Deployment Patterns
- Hardware boundary declaration
- Memory protection configuration
- Interrupt masking safety
- Watchdog integration
- Recovery and rollback strategy
- Multi-core containment models

---
## Chapter 8. Robotics and Embedded Control Systems
### Section 1. Deterministic Motor Control
- PWM generation
- ADC integration
- Sensor polling loops
- Fixed-point arithmetic optimization
- Latency budgeting
- Fault detection

### Section 2. Edge AI Integration
- NPU/TPU invocation
- Direct model execution
- Accelerator memory mapping
- Streaming inference pipelines
- Thermal and power constraints
- Real-time constraint enforcement

### Section 3. Distributed Embedded Nodes
- Lightweight interconnect protocols
- Deterministic message passing
- Multi-node synchronization
- Fault detection across nodes
- Redundant execution models
- Secure update mechanisms
- Appendix A. Minimal Boot Skeletons (ARM and RISC-V)

### Section 1. ARM (AArch64) Minimal Boot Flow
- Reset vector assumptions
- Initial stack pointer setup
- Zeroing BSS and initializing data
- Configuring system control register (SCTLR_EL1)
- Setting exception vector base (VBAR_EL1)
- Establishing translation tables
- Enabling MMU and caches
- Transition to EL1 runtime execution

### Section 2. RISC-V Minimal Boot Flow
- Reset entry and machine mode assumptions
- Initial stack pointer configuration
- Clearing BSS and initializing memory sections
- Setting mtvec for trap handling
- Configuring privilege delegation (medeleg / mideleg)
- Establishing page tables (Sv39 / Sv48)
- Writing satp and enabling virtual memory
- Transition from machine mode to supervisor mode

### Section 3. Multi-Core Boot Considerations
- Primary vs secondary core initialization
- Core parking and wake-up sequences
- Inter-core synchronization primitives
- Shared memory initialization
- Interrupt routing configuration
- Ensuring deterministic startup order

### Section 4. Early Boot Failure Diagnostics
- Detecting invalid page table setup
- Diagnosing misconfigured vector base
- Cache enable ordering issues
- Debugging early boot hangs
- Structured early fault reporting pattern
- Appendix B. Exception and Trap Handling Templates

### Section 1. ARM Exception Handling Patterns
- Synchronous exception entry sequence
- Saving and restoring general-purpose registers
- Decoding ESR_ELx syndrome registers
- Returning with ERET
- Nested exception management
- Secure vs non-secure exception flow

### Section 2. RISC-V Trap Handling Patterns
- Trap entry sequence via mtvec
- Context save template
- Using mcause and mtval for diagnosis
- Delegated trap flow
- Returning with MRET or SRET
- Handling nested interrupts safely

### Section 3. Structured Fault Capture
- Register dump format template
- Stack frame inspection pattern
- Page fault diagnosis workflow
- Illegal instruction analysis
- Logging and structured crash output
- Controlled shutdown procedure

### Section 4. Deterministic Exception Policy
- Interrupt masking strategy
- Priority assignment models
- Fault containment boundaries
- Watchdog-triggered recovery
- Avoiding re-entrant instability
- Safe restart vectors
- Appendix C. Deterministic Timing Reference

### Section 1. Cycle Counter Usage
- ARM generic timer registers
- Reading CNTVCT_EL0
- RISC-V mcycle and time registers
- Ensuring counter stability
- Frequency calibration patterns
- Detecting counter rollover

### Section 2. Interrupt Latency Measurement
- Timestamp at interrupt entry
- Measuring ISR execution duration
- Quantifying jitter
- Identifying cache-induced delays
- Measuring multi-core interference
- Validating real-time deadlines

### Section 3. Memory Ordering and Barriers
- ARM DMB, DSB, ISB semantics
- RISC-V fence instruction patterns
- Enforcing I/O ordering
- Preventing speculative hazards
- Atomic primitive timing implications
- Lock-free synchronization timing effects

### Section 4. Jitter Reduction Strategies
- Cache warming procedures
- Disabling unnecessary interrupts
- Minimizing branch unpredictability
- Isolating critical code paths
- Affinity assignment in multi-core systems
- Power-state stabilization techniques
- Appendix D. AI–Machine Code Engineering Workflow Checklists

### Section 1. Architecture Constraint Declaration
- Target ISA declaration
- Privilege level declaration
- Memory model specification
- Deterministic timing requirements
- Interrupt policy definition
- Hardware boundary constraints

### Section 2. Machine Code Generation Checklist
- Register allocation validation
- Stack discipline verification
- Mode-safe instruction validation
- Memory access compliance checks
- Exception safety verification
- Barrier placement verification

### Section 3. Binary Validation Checklist
- Disassembly confirmation
- Control flow graph inspection
- Privilege instruction audit
- MMU configuration verification
- Interrupt vector correctness
- Deterministic timing validation

### Section 4. Deterministic Testing Workflow
- Reproducible test harness pattern
- Emulator-based verification
- Hardware validation pass
- Fault injection scenario testing
- Regression test structure
- Safe deployment readiness check

### Section 5. Deployment and Safety Checklist
- Watchdog configuration verification
- Interrupt masking confirmation
- Memory protection validation
- Multi-core containment boundaries
- Recovery and rollback strategy
- Field update validation protocol
---
