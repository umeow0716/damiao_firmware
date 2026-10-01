// Enumerate decompiler-resolved accesses to the DM4310 fixed runtime SRAM.
// This is an isolated Ghidra headless analysis aid; it is not part of make.

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Iterator;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.MemoryAccessException;
import ghidra.program.model.pcode.HighFunction;
import ghidra.program.model.pcode.PcodeOp;
import ghidra.program.model.pcode.PcodeOpAST;
import ghidra.program.model.pcode.Varnode;

public class DumpFixedSramAccesses extends GhidraScript {
    private static final long SRAM_FIRST = 0x1ffff078L;
    private static final long SRAM_LAST = 0x1ffff493L;

    private Long constantValue(Varnode node, int depth) {
        if (node == null || depth > 24) {
            return null;
        }
        if (node.isConstant()) {
            return node.getOffset();
        }
        if (node.isAddress()) {
            Address address = node.getAddress();
            if (node.getSize() == 4 &&
                    currentProgram.getMemory().getBlock(address) != null &&
                    currentProgram.getMemory().getBlock(address).isInitialized()) {
                try {
                    return Integer.toUnsignedLong(
                        currentProgram.getMemory().getInt(address));
                } catch (MemoryAccessException error) {
                    // Fall through to the address itself.
                }
            }
            return node.getOffset();
        }
        PcodeOp definition = node.getDef();
        if (definition == null) {
            return null;
        }
        switch (definition.getOpcode()) {
        case PcodeOp.COPY:
        case PcodeOp.CAST:
        case PcodeOp.INT_ZEXT:
        case PcodeOp.INT_SEXT:
            return constantValue(definition.getInput(0), depth + 1);
        case PcodeOp.INT_ADD:
        case PcodeOp.PTRSUB: {
            Long left = constantValue(definition.getInput(0), depth + 1);
            Long right = constantValue(definition.getInput(1), depth + 1);
            return left == null || right == null ? null : left + right;
        }
        case PcodeOp.INT_SUB: {
            Long left = constantValue(definition.getInput(0), depth + 1);
            Long right = constantValue(definition.getInput(1), depth + 1);
            return left == null || right == null ? null : left - right;
        }
        case PcodeOp.INT_MULT: {
            Long left = constantValue(definition.getInput(0), depth + 1);
            Long right = constantValue(definition.getInput(1), depth + 1);
            return left == null || right == null ? null : left * right;
        }
        case PcodeOp.LOAD: {
            Long source = constantValue(definition.getInput(1), depth + 1);
            if (source == null || node.getSize() != 4) {
                return null;
            }
            Address address = currentProgram.getAddressFactory()
                .getDefaultAddressSpace().getAddress(source);
            try {
                return Integer.toUnsignedLong(
                    currentProgram.getMemory().getInt(address));
            } catch (MemoryAccessException error) {
                return null;
            }
        }
        case PcodeOp.PTRADD: {
            Long base = constantValue(definition.getInput(0), depth + 1);
            Long index = constantValue(definition.getInput(1), depth + 1);
            Long scale = constantValue(definition.getInput(2), depth + 1);
            return base == null || index == null || scale == null
                ? null : base + index * scale;
        }
        default:
            return null;
        }
    }

    private String clean(String text) {
        return text.replace('\t', ' ').replace('\n', ' ').replace('\r', ' ');
    }

    @Override
    public void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length != 1) {
            throw new IllegalArgumentException("expected output TSV path");
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.toggleCCode(false);
        decompiler.toggleSyntaxTree(true);
        if (!decompiler.openProgram(currentProgram)) {
            throw new IOException("could not open program in decompiler");
        }

        try (BufferedWriter output = new BufferedWriter(
                new FileWriter(arguments[0]))) {
            output.write("function\tentry\top_address\toperation\tsram_address\tpcode\n");
            for (Function function : currentProgram.getFunctionManager()
                    .getFunctions(true)) {
                monitor.checkCancelled();
                DecompileResults result = decompiler.decompileFunction(
                    function, 120, monitor);
                HighFunction high = result.getHighFunction();
                if (high == null) {
                    continue;
                }
                Iterator<PcodeOpAST> operations = high.getPcodeOps();
                while (operations.hasNext()) {
                    PcodeOpAST operation = operations.next();
                    int opcode = operation.getOpcode();
                    if (opcode != PcodeOp.LOAD && opcode != PcodeOp.STORE) {
                        continue;
                    }
                    Long address = constantValue(operation.getInput(1), 0);
                    if (address == null || address < SRAM_FIRST ||
                            address > SRAM_LAST) {
                        continue;
                    }
                    output.write(clean(function.getName()));
                    output.write(String.format("\t0x%08x\t0x%08x\t%s\t0x%08x\t%s\n",
                        function.getEntryPoint().getOffset(),
                        operation.getSeqnum().getTarget().getOffset(),
                        opcode == PcodeOp.LOAD ? "LOAD" : "STORE",
                        address, clean(operation.toString())));
                }
            }
        } finally {
            decompiler.dispose();
        }
    }
}
