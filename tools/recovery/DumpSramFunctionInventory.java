// Recovery-only Ghidra script. It is not part of the firmware build.
//@category DM4310.Recovery

import java.io.BufferedWriter;
import java.io.FileWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

public class DumpSramFunctionInventory extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length != 1) {
            throw new IllegalArgumentException(
                "usage: DumpSramFunctionInventory <output-path>");
        }

        Address first = toAddr(0x1fff8000L);
        Address last = toAddr(0x1fffa50fL);
        FunctionIterator functions = currentProgram.getFunctionManager()
            .getFunctions(first, true);
        try (BufferedWriter output = new BufferedWriter(
                new FileWriter(arguments[0]))) {
            output.write("entry\tbody_min\tbody_max\tname\tis_entry\n");
            while (functions.hasNext()) {
                Function function = functions.next();
                Address entry = function.getEntryPoint();
                if (entry.compareTo(last) > 0) {
                    break;
                }
                output.write(String.format(
                    "0x%08x\t0x%08x\t0x%08x\t%s\t%s%n",
                    entry.getOffset(),
                    function.getBody().getMinAddress().getOffset(),
                    function.getBody().getMaxAddress().getOffset(),
                    function.getName(),
                    currentProgram.getSymbolTable()
                        .isExternalEntryPoint(entry)));
            }
        }
    }
}
