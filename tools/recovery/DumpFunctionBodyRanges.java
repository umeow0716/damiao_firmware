// Ghidra headless script: emit every exact address range owned by each function.
// @category DM.Recovery

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.List;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.AddressRange;
import ghidra.program.model.address.AddressRangeIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

public class DumpFunctionBodyRanges extends GhidraScript {
    @Override
    protected void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length != 1) {
            throw new IllegalArgumentException(
                "usage: DumpFunctionBodyRanges <output-path>");
        }

        try (BufferedWriter output = new BufferedWriter(
                new FileWriter(arguments[0]))) {
            output.write("entry\tbody_bytes\tname\tranges\n");
            FunctionIterator functions =
                currentProgram.getFunctionManager().getFunctions(true);
            while (functions.hasNext() && !monitor.isCancelled()) {
                Function function = functions.next();
                List<String> ranges = new ArrayList<>();
                AddressRangeIterator iterator =
                    function.getBody().getAddressRanges(true);
                while (iterator.hasNext()) {
                    AddressRange range = iterator.next();
                    ranges.add(
                        "0x" + range.getMinAddress() + "-0x" +
                        range.getMaxAddress());
                }
                output.write("0x" + function.getEntryPoint());
                output.write("\t0x" + Long.toHexString(
                    function.getBody().getNumAddresses()));
                output.write("\t" + function.getName());
                output.write("\t" + String.join(",", ranges));
                output.newLine();
            }
        }
    }
}
