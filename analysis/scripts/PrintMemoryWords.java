// Print little-endian words from an imported recovery image.
// @category DM4310

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;

public class PrintMemoryWords extends GhidraScript {
    @Override
    public void run() throws Exception {
        for (String value : getScriptArgs()) {
            Address address = toAddr(value);
            int word = currentProgram.getMemory().getInt(address);
            println(address + " = 0x" + String.format("%08x", word));
        }
    }
}
