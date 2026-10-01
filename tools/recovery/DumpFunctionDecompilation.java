// Recovery-only Ghidra script. It is not part of the firmware build.
//@category DM4310.Recovery

import java.io.BufferedWriter;
import java.io.FileWriter;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

public class DumpFunctionDecompilation extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length != 2) {
            throw new IllegalArgumentException(
                "usage: DumpFunctionDecompilation <address> <output-path>");
        }

        long offset = Long.decode(arguments[0]);
        Address address = currentProgram.getAddressFactory()
            .getDefaultAddressSpace().getAddress(offset);
        Function function = currentProgram.getFunctionManager()
            .getFunctionContaining(address);
        if (function == null) {
            throw new IllegalArgumentException(
                "no function contains " + arguments[0]);
        }

        DecompInterface decompiler = new DecompInterface();
        try {
            if (!decompiler.openProgram(currentProgram)) {
                throw new IllegalStateException(
                    "could not open program in decompiler");
            }
            DecompileResults result = decompiler.decompileFunction(
                function, 120, monitor);
            if (!result.decompileCompleted()) {
                throw new IllegalStateException(result.getErrorMessage());
            }
            try (BufferedWriter output = new BufferedWriter(
                    new FileWriter(arguments[1]))) {
                output.write(result.getDecompiledFunction().getC());
            }
        } finally {
            decompiler.dispose();
        }
    }
}
