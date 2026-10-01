// Ghidra pre-analysis script for raw DM factory APP images.
// @category DM.Recovery

import java.math.BigInteger;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.SourceType;

public class PrepareDmFactory extends GhidraScript {
    private static final long SCATTER_DESCRIPTOR = 0x00028634L;

    private Address address(long value) {
        return currentProgram.getAddressFactory()
            .getDefaultAddressSpace().getAddress(value);
    }

    private long unsignedInt(Memory memory, long address) throws Exception {
        return Integer.toUnsignedLong(memory.getInt(address(address)));
    }

    private void setThumb(long start, long end) throws Exception {
        Register tmode = currentProgram.getRegister("TMode");
        if (tmode == null) {
            throw new IllegalStateException("ARM TMode register is unavailable");
        }
        currentProgram.getProgramContext().setValue(
            tmode, address(start), address(end), BigInteger.ONE);
    }

    private void seedFunction(long entry, String name) throws Exception {
        Address functionAddress = address(entry);
        disassemble(functionAddress);
        Function function = getFunctionAt(functionAddress);
        if (function == null) {
            function = createFunction(functionAddress, name);
        } else if (name != null) {
            function.setName(name, SourceType.USER_DEFINED);
        }
    }

    @Override
    protected void run() throws Exception {
        Memory memory = currentProgram.getMemory();
        long flashRamImage = unsignedInt(memory, SCATTER_DESCRIPTOR);
        long fixedRamBase = unsignedInt(memory, SCATTER_DESCRIPTOR + 4L);
        long fixedRamSize = unsignedInt(memory, SCATTER_DESCRIPTOR + 8L);
        long stateBase = unsignedInt(memory, SCATTER_DESCRIPTOR + 20L);
        long stateSize = unsignedInt(memory, SCATTER_DESCRIPTOR + 24L);
        long bssBase = unsignedInt(memory, SCATTER_DESCRIPTOR + 36L);
        long bssSize = unsignedInt(memory, SCATTER_DESCRIPTOR + 40L);
        if (fixedRamBase != 0x1fff8000L || stateBase != fixedRamBase + fixedRamSize ||
                bssBase != stateBase + stateSize) {
            throw new IllegalStateException("unexpected factory scatter layout");
        }

        byte[] fixedImage = new byte[(int) fixedRamSize];
        int copied = memory.getBytes(address(flashRamImage), fixedImage);
        if (copied != fixedImage.length) {
            throw new IllegalStateException("factory fixed-SRAM image is truncated");
        }

        MemoryBlock fixedBlock = memory.getBlock(address(fixedRamBase));
        if (fixedBlock == null) {
            fixedBlock = memory.createInitializedBlock(
                "fixed_sram", address(fixedRamBase), fixedRamSize,
                (byte) 0, monitor, false);
            fixedBlock.setRead(true);
            fixedBlock.setWrite(true);
            fixedBlock.setExecute(true);
        }
        memory.setBytes(address(fixedRamBase), fixedImage);

        MemoryBlock stateBlock = memory.getBlock(address(stateBase));
        if (stateBlock == null) {
            stateBlock = memory.createUninitializedBlock(
                "runtime_sram", address(stateBase), stateSize + bssSize, false);
            stateBlock.setRead(true);
            stateBlock.setWrite(true);
            stateBlock.setExecute(false);
        }

        setThumb(0x00020250L, SCATTER_DESCRIPTOR - 1L);
        setThumb(fixedRamBase, fixedRamBase + fixedRamSize - 1L);

        seedFunction(0x00020250L, "entry_stub");
        seedFunction(0x00020258L, "scatter_entry");
        seedFunction(0x00020328L, "runtime_format_dispatch");
        seedFunction(0x00020344L, "runtime_initialize");
        seedFunction(0x00020364L, "runtime_exit");
        seedFunction(0x00020368L, "runtime_main_entry");
        seedFunction(0x00020388L, "Reset_Handler");
        seedFunction(0x00020390L, "NMI_Handler");
        seedFunction(0x00020392L, "HardFault_Handler");
        seedFunction(0x00020394L, "MemManage_Handler");
        seedFunction(0x00020396L, "BusFault_Handler");
        seedFunction(0x00020398L, "UsageFault_Handler");
        seedFunction(0x0002039aL, "SVC_Handler");
        seedFunction(0x0002039cL, "DebugMon_Handler");
        seedFunction(0x0002039eL, "PendSV_Handler");
        seedFunction(0x000203a0L, "SysTick_Handler");
        seedFunction(0x000203a2L, "Default_Handler");
        seedFunction(0x000203a4L, "runtime_memory_bounds");

        for (int vectorIndex = 0x10; vectorIndex <= 0x14; ++vectorIndex) {
            long raw = unsignedInt(memory, 0x00020000L + vectorIndex * 4L);
            seedFunction(raw & ~1L, "IRQ" + (vectorIndex - 0x10) + "_Handler");
        }

        println(String.format(
            "scatter: fixed=%08x+%x state=%08x+%x bss=%08x+%x",
            fixedRamBase, fixedRamSize, stateBase, stateSize, bssBase, bssSize));
    }
}
