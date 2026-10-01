// Add the DM4310 SRAM windows before auto-analysis of a raw factory image.
// This lets Ghidra represent literal 0x1fffxxxx values as data references.

import java.io.ByteArrayInputStream;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.Memory;

public class MapDm4310Memory extends GhidraScript {
    private void addBlock(String name, long start, long size) throws Exception {
        Memory memory = currentProgram.getMemory();
        Address address = currentProgram.getAddressFactory()
            .getDefaultAddressSpace().getAddress(start);
        if (memory.getBlock(address) == null) {
            memory.createUninitializedBlock(name, address, size, false);
        }
    }

    @Override
    public void run() throws Exception {
        Memory memory = currentProgram.getMemory();
        Address flashSource = toAddr(0x00028680L);
        Address ramCode = toAddr(0x1fff8000L);
        if (memory.getBlock(ramCode) == null) {
            byte[] scatter = new byte[0x2510];
            memory.getBytes(flashSource, scatter);
            memory.createInitializedBlock("SRAM_CODE", ramCode,
                new ByteArrayInputStream(scatter), scatter.length,
                monitor, false);
            memory.createUninitializedBlock("SRAM_BSS", toAddr(0x1fffa510L),
                0xdaf0L, false);
        }
        addBlock("SRAMB", 0x200f0000L, 0x1000L);

        long[] entries = {
            0x1fff8000L, 0x1fff8136L, 0x1fff8640L, 0x1fff8744L, 0x1fff8770L,
            0x1fff88b8L, 0x1fff987cL, 0x1fff9950L, 0x1fff9accL,
            0x1fff9ba0L, 0x1fff9c40L, 0x1fff9dc8L, 0x1fff9e44L,
            0x1fff9fdeL, 0x1fffa07aL,
            0x1fffa160L, 0x1fffa234L, 0x1fffa30cL, 0x1fffa38aL,
            0x1fffa3aaL, 0x1fffa414L, 0x1fffa46aL, 0x1fffa492L,
            0x1fffa4dcL, 0x1fffa4e6L, 0x1fffa4f0L, 0x1fffa4faL,
            0x1fffa504L
        };
        for (long value : entries) {
            Address entry = toAddr(value);
            disassemble(entry);
            if (getFunctionAt(entry) == null) {
                createFunction(entry, null);
            }
            addEntryPoint(entry);
        }
    }
}
