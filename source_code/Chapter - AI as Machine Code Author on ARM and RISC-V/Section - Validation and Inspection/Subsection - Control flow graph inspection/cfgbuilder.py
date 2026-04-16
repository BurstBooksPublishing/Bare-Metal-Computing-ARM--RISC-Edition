Control Flow Graph Builder for ARM (AArch64) and RISC-V


#!/usr/bin/env python3
"""Build a CFG for AArch64 or RISC-V using capstone and networkx."""
from capstone import *
import networkx as nx
from typing import Dict, List, Tuple, Any

def disassemble(arch: str, code: bytes, base: int = 0x0) -> List[Any]:
    if arch == "aarch64":
        cs = Cs(CS_ARCH_ARM64, CS_MODE_ARM)
    elif arch == "riscv64":
        cs = Cs(CS_ARCH_RISCV, CS_MODE_RISCV64)
    else:
        raise ValueError("unsupported arch")
    cs.detail = True
    return list(cs.disasm(code, base))

def build_cfg(insns: List[Any]) -> nx.DiGraph:
    # map address -> instruction
    addr_map: Dict[int, Any] = {i.address: i for i in insns}
    # find leaders
    leaders = {insns[0].address}
    for i in insns:
        if i.groups and (CS_GRP_JUMP in i.groups or CS_GRP_CALL in i.groups or CS_GRP_RET in i.groups):
            # add target(s)
            for op in i.operands if hasattr(i, "operands") else []:
                if op.type == CS_OP_IMM:
                    leaders.add(op.imm)
            # fall-through leader
            next_addr = i.address + i.size
            leaders.add(next_addr)
    # form blocks
    blocks: Dict[int, List[Any]] = {}
    cur_leader = None
    for i in insns:
        if i.address in leaders:
            cur_leader = i.address
            blocks[cur_leader] = []
        blocks[cur_leader].append(i)
    # build graph
    G = nx.DiGraph()
    for entry, block_ins in blocks.items():
        G.add_node(entry, bytes=b"".join(i.bytes for i in block_ins))
        last = block_ins[-1]
        next_addr = last.address + last.size
        is_call = CS_GRP_CALL in last.groups if last.groups else False
        is_ret = CS_GRP_RET in last.groups if last.groups else False
        is_jump = CS_GRP_JUMP in last.groups if last.groups else False
        # direct immediate targets
        for op in getattr(last, "operands", []):
            if getattr(op, "type", None) == CS_OP_IMM:
                tgt = op.imm
                if tgt in blocks:
                    G.add_edge(entry, tgt, type="direct")
        # fall-through edge
        if not is_ret and next_addr in blocks and not (is_jump and not is_call):
            G.add_edge(entry, next_addr, type="fallthrough")
        # mark indirect if registers appear in branch operands
        for op in getattr(last, "operands", []):
            if getattr(op, "type", None) == CS_OP_REG:
                G.add_edge(entry, -1, type="indirect")  # -1 denotes unknown/indirect target
    return G

# Example usage (bytes must be provided from a real binary)
# insns = disassemble("aarch64", b"\x00\x00\x00\x14...")
# G = build_cfg(insns)
# dom = nx.immediate_dominators(G, list(G.nodes())[0])  # compute dominators