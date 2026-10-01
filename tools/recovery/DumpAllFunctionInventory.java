// Ghidra headless script: emit the complete factory function/call closure.
// @category DM4310.Recovery

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Set;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

public class DumpAllFunctionInventory extends GhidraScript {
    private static String addresses(Set<Function> functions) {
        List<String> values = new ArrayList<>();
        for (Function function : functions) {
            values.add(function.getEntryPoint().toString());
        }
        Collections.sort(values);
        return String.join(",", values);
    }

    @Override
    protected void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length != 1) {
            throw new IllegalArgumentException(
                "usage: DumpAllFunctionInventory <output-path>");
        }

        try (BufferedWriter output = new BufferedWriter(
                new FileWriter(arguments[0]))) {
            output.write("entry\tend\tbody_bytes\tname\tthunk_target\tcallers\tcallees\n");
            FunctionIterator functions =
                currentProgram.getFunctionManager().getFunctions(true);
            while (functions.hasNext() && !monitor.isCancelled()) {
                Function function = functions.next();
                String thunkTarget = "";
                Function thunked = function.getThunkedFunction(false);
                if (thunked != null) {
                    thunkTarget = thunked.getEntryPoint().toString();
                }
                output.write("0x" + function.getEntryPoint());
                output.write("\t0x" + function.getBody().getMaxAddress());
                output.write("\t0x" + Long.toHexString(
                    function.getBody().getNumAddresses()));
                output.write("\t" + function.getName());
                output.write("\t" + thunkTarget);
                output.write("\t" + addresses(
                    function.getCallingFunctions(monitor)));
                output.write("\t" + addresses(
                    function.getCalledFunctions(monitor)));
                output.newLine();
            }
        }
    }
}
