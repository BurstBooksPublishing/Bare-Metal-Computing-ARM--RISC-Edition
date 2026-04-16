Privilege Instruction Validator for ARM (AArch64) and RISC-V


from capstone import Cs, CS_ARCH_ARM64, CS_ARCH_RISCV, \
                    CS_MODE_LITTLE_ENDIAN, CS_MODE_RISCV64
from typing import List, Dict, Any

FORBIDDEN = {
    'arm64': {
        'svc', 'hvc', 'smc', 'mrs', 'msr', 'eret', 'sys'
    },
    'riscv64': {
        'ecall', 'ebreak', 'mret', 'sret', 'uret',
        'csrrw', 'csrrs', 'csrrc', 'csrrwi', 'csrrsi', 'csrrci', 'sfence.vma'
    }
}

def disassembler_for(arch: str):
    if arch == 'arm64':
        return Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN)
    if arch == 'riscv64':
        return Cs(CS_ARCH_RISCV, CS_MODE_RISCV64 | CS_MODE_LITTLE_ENDIAN)
    raise ValueError("unsupported arch")

def validate_privilege_compliance(blob: bytes, arch: str,
                                  base_addr: int = 0) -> Dict[str, Any]:
    cs = disassembler_for(arch)
    forbidden = FORBIDDEN[arch]
    violations: List[Dict[str, Any]] = []
    total = 0
    for insn in cs.disasm(blob, base_addr):
        total += 1
        mnem = insn.mnemonic.lower()
        if mnem in forbidden:
            violations.append({
                'addr': insn.address,
                'bytes': insn.bytes.hex(),
                'mnemonic': insn.mnemonic,
                'op_str': insn.op_str
            })
    f = (len(violations) / total) if total else 0.0
    return {
        'arch': arch,
        'total_instructions': total,
        'forbidden_count': len(violations),
        'forbidden_density': f,
        'violations': violations
    }