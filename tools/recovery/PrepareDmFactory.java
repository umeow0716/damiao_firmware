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
    private static final long APP_BASE = 0x00020000L;
    private static final long APP_END = 0x00030000L;
    private static final long FIXED_SRAM_BASE = 0x1fff8000L;

    private Address address(long value) {
        return currentProgram.getAddressFactory()
            .getDefaultAddressSpace().getAddress(value);
    }

    private long unsignedInt(Memory memory, long address) throws Exception {
        return Integer.toUnsignedLong(memory.getInt(address(address)));
    }

    private long findScatterDescriptor(Memory memory) throws Exception {
        for (long candidate = APP_BASE;
                candidate + 44L < APP_END; candidate += 4L) {
            if (!memory.contains(address(candidate)) ||
                    !memory.contains(address(candidate + 44L))) {
                continue;
            }
            long fixedSource = unsignedInt(memory, candidate);
            long fixedBase = unsignedInt(memory, candidate + 4L);
            long fixedSize = unsignedInt(memory, candidate + 8L);
            long copyRoutine = unsignedInt(memory, candidate + 12L);
            long stateSource = unsignedInt(memory, candidate + 16L);
            long stateBase = unsignedInt(memory, candidate + 20L);
            long stateSize = unsignedInt(memory, candidate + 24L);
            long zeroRoutine = unsignedInt(memory, candidate + 28L);
            long bssSource = unsignedInt(memory, candidate + 32L);
            long bssBase = unsignedInt(memory, candidate + 36L);
            long bssSize = unsignedInt(memory, candidate + 40L);

            boolean valid = fixedBase == FIXED_SRAM_BASE &&
                fixedSource >= APP_BASE && fixedSource < APP_END &&
                fixedSize >= 0x1000L && fixedSize <= 0x4000L &&
                copyRoutine >= APP_BASE && copyRoutine < fixedSource &&
                stateSource == fixedSource + fixedSize &&
                stateBase == fixedBase + fixedSize &&
                stateSize >= 0x1000L && stateSize <= 0x4000L &&
                zeroRoutine >= APP_BASE && zeroRoutine < fixedSource &&
                bssSource >= stateSource && bssSource < APP_END &&
                bssBase == stateBase + stateSize &&
                bssSize >= 0x1000L && bssSize <= 0x8000L;
            if (valid) {
                return candidate;
            }
        }
        throw new IllegalStateException(
            "could not locate the factory scatter descriptor");
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
        long scatterDescriptor = findScatterDescriptor(memory);
        long flashRamImage = unsignedInt(memory, scatterDescriptor);
        long fixedRamBase = unsignedInt(memory, scatterDescriptor + 4L);
        long fixedRamSize = unsignedInt(memory, scatterDescriptor + 8L);
        long stateBase = unsignedInt(memory, scatterDescriptor + 20L);
        long stateSize = unsignedInt(memory, scatterDescriptor + 24L);
        long bssBase = unsignedInt(memory, scatterDescriptor + 36L);
        long bssSize = unsignedInt(memory, scatterDescriptor + 40L);
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

        setThumb(0x00020250L, scatterDescriptor - 1L);
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
            "scatter@%08x: fixed=%08x->%08x+%x state=%08x->%08x+%x " +
            "bss=%08x+%x",
            scatterDescriptor, flashRamImage, fixedRamBase, fixedRamSize,
            flashRamImage + fixedRamSize, stateBase, stateSize,
            bssBase, bssSize));
    }
}
