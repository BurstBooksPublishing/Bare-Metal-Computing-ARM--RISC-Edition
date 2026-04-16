\title{Disassembly Verification Utility}
\caption{Python utility for verifying AArch64 or RISC-V 64-bit binary code using Capstone disassembly engine.}

from capstone import *
import struct
import re

def verify_disassembly(data: bytes, base_addr: int, isa: str):
    if isa == "aarch64":
        md = Cs(CS_ARCH_ARM64, CS_MODE_ARM)
        align = 4
        expected_sizes = {4}
    elif isa == "riscv64":
        md = Cs(CS_ARCH_RISCV, CS_MODE_RISCV64)
        align = 2
        expected_sizes = {2, 4}
    else:
        raise ValueError("unsupported ISA")

    md.detail = False
    instructions = list(md.disasm(data, base_addr))

    total = sum(insn.size for insn in instructions)
    if total != len(data):
        raise RuntimeError(f"coverage mismatch: sum sizes {total} != data len {len(data)}")

    if base_addr % align != 0:
        raise RuntimeError(f"base address {hex(base_addr)} misaligned for {isa}")

    for insn in instructions:
        if insn.size not in expected_sizes:
            raise RuntimeError(f"invalid instruction size {insn.size} at {hex(insn.address)}")

    code_start, code_end = base_addr, base_addr + len(data)
    for insn in instructions:
        m = insn.mnemonic
        ops = insn.op_str
        if m.startswith(("b", "bl", "jal", "jalr", "beq", "bne", "blt", "bge")):
            match = re.search(r"(-?0x[0-9a-fA-F]+|-?\d+)", ops)
            if match:
                imm = int(match.group(0), 0)
                pc = insn.address + (4 if isa == "aarch64" else insn.size)
                target = pc + imm
                if not (code_start <= target < code_end):
                    print(f"warning: branch at {hex(insn.address)} targets {hex(target)} outside [{hex(code_start)},{hex(code_end)})")
            else:
                pass

    return True