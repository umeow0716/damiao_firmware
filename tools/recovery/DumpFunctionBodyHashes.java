// Ghidra headless script: hash every exact function body address set.
// @category DM.Recovery

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.security.MessageDigest;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

public class DumpFunctionBodyHashes extends GhidraScript {
    private static String hex(byte[] bytes) {
        StringBuilder result = new StringBuilder(bytes.length * 2);
        for (byte value : bytes) {
            result.append(String.format("%02x", value & 0xff));
        }
        return result.toString();
    }

    @Override
    protected void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length != 1) {
            throw new IllegalArgumentException(
                "usage: DumpFunctionBodyHashes <output-path>");
        }

        try (BufferedWriter output = new BufferedWriter(
                new FileWriter(arguments[0]))) {
            output.write("entry\tend\tbody_bytes\tname\tsha256\n");
            FunctionIterator functions =
                currentProgram.getFunctionManager().getFunctions(true);
            while (functions.hasNext() && !monitor.isCancelled()) {
                Function function = functions.next();
                MessageDigest digest = MessageDigest.getInstance("SHA-256");
                AddressIterator addresses = function.getBody().getAddresses(true);
                while (addresses.hasNext()) {
                    Address address = addresses.next();
                    digest.update(currentProgram.getMemory().getByte(address));
                }
                output.write("0x" + function.getEntryPoint());
                output.write("\t0x" + function.getBody().getMaxAddress());
                output.write("\t0x" + Long.toHexString(
                    function.getBody().getNumAddresses()));
                output.write("\t" + function.getName());
                output.write("\t" + hex(digest.digest()));
                output.newLine();
            }
        }
    }
}
