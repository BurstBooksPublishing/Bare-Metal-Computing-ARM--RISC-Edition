RISC-V Register Constraints and Allocation Helper (RV64)


from typing import List, Set

class RV64Constraints:
    def __init__(self, require_compressible: bool = False):
        # ARBITRARY: canonical register names x0..x31 as strings
        self.all_regs: List[str] = [f"x{i}" for i in range(32)]
        # Reserve hardwired and ABI-critical registers
        self.reserved: Set[str] = {"x0", "x2"}            # zero, sp
        self.reserved.add("x1")                           # ra for callable code
        # Define caller/callee saved sets per SysV RISC-V psABI
        self.caller_saved: Set[str] = {
            "x1", "x5", "x6", "x7", "x10", "x11",
            "x12", "x13", "x14", "x15", "x16", "x17",
            "x28", "x29", "x30", "x31"
        }
        self.callee_saved: Set[str] = {
            "x8", "x9", "x18", "x19", "x20", "x21",
            "x22", "x23", "x24", "x25", "x26", "x27"
        }
        # Compressed-register preference
        self.require_compressible = require_compressible
        self.c_regs: Set[str] = {f"x{i}" for i in range(8, 16)}  # x8..x15
        # Effective allocatable registers computed on demand

    def allocatable(self) -> List[str]:
        # Exclude reserved and prefer callee-saved for spills
        base = [r for r in self.all_regs if r not in self.reserved]
        if self.require_compressible:
            # Restrict to C-registers; x2 is not included unless explicitly allowed
            base = [r for r in base if r in self.c_regs]
        return base

    def effective_R(self) -> int:
        return len(self.allocatable())

    def required_spills(self, temporaries: int) -> int:
        R = self.effective_R()
        return max(0, temporaries - R)

    # Small helper for frame alignment checks
    @staticmethod
    def check_stack_align(sp: int) -> bool:
        return (sp & 0xF) == 0  # 16-byte alignment

# Example usage:
# c = RV64Constraints(require_compressible=True)
# print(c.allocatable(), c.required_spills(10))