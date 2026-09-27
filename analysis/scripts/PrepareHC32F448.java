// Reconstruct the HC32F448 memory map and seed Cortex-M/Thumb analysis.
// @category DM4310

import java.math.BigInteger;
import java.util.LinkedHashMap;
import java.util.Map;

import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.data.Pointer32DataType;
import ghidra.program.model.lang.Register;
import ghidra.program.model.lang.RegisterValue;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.SourceType;

public class PrepareHC32F448 extends GhidraScript {
    private static final String[] VECTOR_NAMES = ("""
        __StackTop Reset_Handler NMI_Handler HardFault_Handler MemManage_Handler
        BusFault_Handler UsageFault_Handler RESERVED RESERVED RESERVED RESERVED
        SVC_Handler DebugMon_Handler RESERVED PendSV_Handler SysTick_Handler
        IRQ000_Handler IRQ001_Handler IRQ002_Handler IRQ003_Handler IRQ004_Handler
        IRQ005_Handler IRQ006_Handler IRQ007_Handler IRQ008_Handler IRQ009_Handler
        IRQ010_Handler IRQ011_Handler IRQ012_Handler IRQ013_Handler IRQ014_Handler
        IRQ015_Handler EXTINT00_SWINT16_Handler EXTINT01_SWINT17_Handler
        EXTINT02_SWINT18_Handler EXTINT03_SWINT19_Handler EXTINT04_SWINT20_Handler
        EXTINT05_SWINT21_Handler EXTINT06_SWINT22_Handler EXTINT07_SWINT23_Handler
        EXTINT08_SWINT24_Handler EXTINT09_SWINT25_Handler EXTINT10_SWINT26_Handler
        EXTINT11_SWINT27_Handler EXTINT12_SWINT28_Handler EXTINT13_SWINT29_Handler
        EXTINT14_SWINT30_Handler EXTINT15_SWINT31_Handler DMA1_Error_Handler
        DMA1_TC0_BTC0_Handler DMA1_TC1_BTC1_Handler DMA1_TC2_BTC2_Handler
        DMA1_TC3_BTC3_Handler DMA1_TC4_BTC4_Handler DMA1_TC5_BTC5_Handler
        EFM_PEError_ReadCol_Handler EFM_OpEnd_Handler QSPI_Handler DCU1_Handler
        DCU2_Handler DCU3_Handler DCU4_Handler DMA2_Error_Handler
        DMA2_TC0_BTC0_Handler DMA2_TC1_BTC1_Handler DMA2_TC2_BTC2_Handler
        DMA2_TC3_BTC3_Handler DMA2_TC4_BTC4_Handler DMA2_TC5_BTC5_Handler
        TMR0_1_Handler TMR0_2_Handler RTC_Handler CLK_XtalStop_Handler
        PWC_WKTM_Handler SWDT_Handler TMR6_1_GCmp_Handler TMR6_1_Ovf_Udf_Handler
        TMR6_1_Dte_Handler TMR6_1_SCmp_Handler TMRA_1_Ovf_Udf_Handler
        TMRA_1_Cmp_Handler TMR6_2_GCmp_Handler TMR6_2_Ovf_Udf_Handler
        TMR6_2_Dte_Handler TMR6_2_SCmp_Handler TMRA_2_Ovf_Udf_Handler
        TMRA_2_Cmp_Handler TMRA_3_Ovf_Udf_Handler TMRA_3_Cmp_Handler
        TMRA_4_Ovf_Udf_Handler TMRA_4_Cmp_Handler TMR4_1_GCmp_Handler
        TMR4_1_Ovf_Udf_Handler TMR4_1_Reload_Handler TMR4_1_SCmp_Handler
        TMR4_2_GCmp_Handler TMR4_2_Ovf_Udf_Handler TMR4_2_Reload_Handler
        TMR4_2_SCmp_Handler TMR4_3_GCmp_Handler TMR4_3_Ovf_Udf_Handler
        TMR4_3_Reload_Handler TMR4_3_SCmp_Handler I2C1_Handler I2C2_Handler
        CMP1_Handler CMP2_Handler CMP3_Handler CMP4_Handler USART1_Handler
        USART1_TxComplete_Handler USART2_Handler USART2_TxComplete_Handler
        SPI1_Handler TMRA_5_Ovf_Udf_Handler TMRA_5_Cmp_Handler EVENT_PORT1_Handler
        EVENT_PORT2_Handler EVENT_PORT3_Handler EVENT_PORT4_Handler USART3_Handler
        USART3_TxComplete_Handler USART4_Handler USART4_TxComplete_Handler
        SPI2_Handler SPI3_Handler EMB_GR0_Handler EMB_GR1_Handler EMB_GR2_Handler
        EMB_GR3_Handler USART5_Handler USART5_TxComplete_Handler USART6_Handler
        USART6_TxComplete_Handler MCAN1_INT0_Handler MCAN1_INT1_Handler
        MCAN2_INT0_Handler MCAN2_INT1_Handler USART1_WKUP_Handler PWC_LVD1_Handler
        PWC_LVD2_Handler FCM_Handler WDT_Handler CTC_Handler ADC1_Handler
        ADC2_Handler ADC3_Handler TRNG_Handler
        """).trim().split("\\s+");

    private Memory memory;
    private Listing listing;
    private Register tmode;

    @Override
    public void run() throws Exception {
        memory = currentProgram.getMemory();
        listing = currentProgram.getListing();
        tmode = currentProgram.getProgramContext().getRegister("TMode");
        boolean app = currentProgram.getName().toLowerCase().contains("app");

        renameRawBlock(app ? "FLASH_APP" : "FLASH_BOOT");
        mapCommonMemory();
        if (app) {
            mapAppRuntimeImage();
            seedAppFunctions();
        }
        else {
            mapBootRuntimeImage();
            seedBootFunctions();
        }
        labelPeripheralBases();
        seedVectors(app ? 0x00020000L : 0L);
        analyzeChanges(currentProgram);
        println("Prepared " + currentProgram.getName() + " for HC32F448 analysis");
    }

    private Address a(long offset) {
        return currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(offset);
    }

    private void renameRawBlock(String name) throws Exception {
        MemoryBlock first = memory.getBlocks()[0];
        if (!first.getName().equals(name) && memory.getBlock(name) == null) {
            first.setName(name);
        }
        first.setRead(true);
        first.setWrite(false);
        first.setExecute(true);
    }

    private MemoryBlock addZeroBlock(String name, long start, long size,
            boolean write, boolean execute) throws Exception {
        MemoryBlock existing = memory.getBlock(name);
        if (existing != null) {
            return existing;
        }
        MemoryBlock block = memory.createInitializedBlock(
            name, a(start), size, (byte) 0, monitor, false);
        block.setRead(true);
        block.setWrite(write);
        block.setExecute(execute);
        return block;
    }

    private MemoryBlock addBytesBlock(String name, long start, byte[] data,
            boolean write, boolean execute) throws Exception {
        MemoryBlock existing = memory.getBlock(name);
        MemoryBlock block = existing != null ? existing :
            addZeroBlock(name, start, data.length, write, execute);
        // Refresh non-executable initialized data on repeat runs.  Ghidra
        // rejects writes over decoded instructions in executable RAM blocks;
        // those are direct copies and do not depend on either decoder.
        if (existing == null || !execute) {
            memory.setBytes(a(start), data);
        }
        return block;
    }

    private byte[] readBytes(long start, int length) throws Exception {
        byte[] result = new byte[length];
        int count = memory.getBytes(a(start), result);
        if (count != length) {
            throw new IllegalStateException("Short read at " + Long.toHexString(start));
        }
        return result;
    }

    private void mapCommonMemory() throws Exception {
        // The initialized sub-blocks below occupy the beginning of the 64 KiB
        // main SRAM alias. RAMB and peripherals are separate address regions.
        addZeroBlock("RAMB", 0x200f0000L, 0x1000L, true, true);
        addZeroBlock("PERIPHERALS", 0x40000000L, 0x100000L, true, false);
        addZeroBlock("CORTEX_M4_SCS", 0xe0000000L, 0x100000L, true, false);
    }

    private void mapAppRuntimeImage() throws Exception {
        byte[] ramCodeAndData = readBytes(0x00028680L, 0x2510);
        addBytesBlock("APP_RAM_INIT", 0x1fff8000L, ramCodeAndData, true, true);

        // The next scatter descriptor begins at 0x2c84c, so the compressed
        // payload occupies exactly 0x1cbc bytes.
        byte[] packed = readBytes(0x0002ab90L, 0x1cbc);
        byte[] data = decodeArmScatter(packed, 0x2268);
        addBytesBlock("APP_DATA", 0x1fffa510L, data, true, false);

        addZeroBlock("APP_BSS", 0x1fffc778L, 0x3d80L, true, false);
        addZeroBlock("APP_STACK", 0x200004f8L, 0x7b08L, true, false);
    }

    private void mapBootRuntimeImage() throws Exception {
        byte[] ramCodeAndData = readBytes(0x000073e8L, 0x29c);
        addBytesBlock("BOOT_RAM_INIT", 0x1fff8000L, ramCodeAndData, true, true);

        // The IAR decoder intentionally consumes part of the erased 0xff
        // padding after 0x7700; it finishes at 0x7765 for this image.
        byte[] packed = readBytes(0x00007684L, 0x200);
        // This IAR stream may back-reference the immediately preceding
        // initialized block, so preserve that prefix while decoding.
        byte[] data = decodeIarInit(packed, ramCodeAndData, 0x190);
        addBytesBlock("BOOT_DATA", 0x1fff829cL, data, true, false);

        addZeroBlock("BOOT_BSS_STACK", 0x1fff842cL, 0xdea4L, true, false);
        addZeroBlock("BOOT_FREE_RAM", 0x200062d0L, 0x1d30L, true, false);
    }

    // Decoder implemented by the Arm Compiler scatter routine at 0x2028c.
    private byte[] decodeArmScatter(byte[] input, int outputLength) {
        byte[] out = new byte[outputLength];
        int ip = 0;
        int op = 0;
        while (op < outputLength) {
            int token = input[ip++] & 0xff;
            int literals = token & 3;
            if (literals == 0) {
                literals = input[ip++] & 0xff;
            }
            int match = token >>> 4;
            if (match == 0) {
                match = input[ip++] & 0xff;
            }
            for (int i = 1; i < literals && op < outputLength; i++) {
                out[op++] = input[ip++];
            }
            if (match != 0 && op < outputLength) {
                int low = input[ip++] & 0xff;
                int kind = token & 0x0c;
                int distance = low + (kind == 0x0c ?
                    ((input[ip++] & 0xff) << 8) : (kind << 6));
                int source = op - distance;
                int count = match + 2;
                for (int i = 0; i < count && op < outputLength; i++) {
                    out[op++] = out[source++];
                }
            }
        }
        return out;
    }

    // Decoder implemented by the IAR initialization routine at 0x676.
    private byte[] decodeIarInit(byte[] input, byte[] prefix, int outputLength) {
        byte[] work = new byte[prefix.length + outputLength];
        System.arraycopy(prefix, 0, work, 0, prefix.length);
        int ip = 0;
        int op = prefix.length;
        int end = work.length;
        while (op < end) {
            int token = input[ip++] & 0xff;
            int literals = token & 7;
            if (literals == 0) {
                literals = input[ip++] & 0xff;
            }
            int run = token >>> 4;
            if (run == 0) {
                run = input[ip++] & 0xff;
            }
            // __iar_unpack_data decrements the encoded literal count before
            // testing/copying, so this field represents literal bytes + 1.
            for (int i = 1; i < literals && op < end; i++) {
                work[op++] = input[ip++];
            }
            if ((token & 8) == 0) {
                for (int i = 0; i < run && op < end; i++) {
                    work[op++] = 0;
                }
            }
            else {
                int distance = input[ip++] & 0xff;
                int source = op - distance;
                for (int i = 0; i < run + 2 && op < end; i++) {
                    work[op++] = work[source++];
                }
            }
        }
        byte[] out = new byte[outputLength];
        System.arraycopy(work, prefix.length, out, 0, outputLength);
        return out;
    }

    private void seedAppFunctions() throws Exception {
        seed(0x00020250L, "__main");
        seed(0x00020258L, "__scatterload");
        seed(0x0002028cL, "__scatterload_decompress");
        seed(0x000202f0L, "__scatterload_copy");
        seed(0x0002030cL, "__scatterload_zeroinit");
        seed(0x00020368L, "__rt_entry");
        seed(0x00020388L, "Reset_Handler");
        seed(0x00023554L, "SystemInit");
        seed(0x000252f4L, "main");
    }

    private void seedBootFunctions() throws Exception {
        seed(0x00000250L, "__program_start");
        seed(0x00000278L, "Reset_Handler");
        seed(0x000003ccL, "__iar_data_init3");
        seed(0x00000676L, "__iar_unpack_data");
        seed(0x00004d20L, "SystemInit");
        seed(0x00005f0cL, "__iar_copy_init");
        seed(0x00005f1cL, "__iar_zero_init");
        seed(0x0000684cL, "main");
    }

    private void seedVectors(long vectorBase) throws Exception {
        for (int i = 1; i < VECTOR_NAMES.length; i++) {
            long slotOffset = vectorBase + i * 4L;
            Address slot = a(slotOffset);
            long raw = Integer.toUnsignedLong(memory.getInt(slot));
            if (raw == 0) {
                continue;
            }
            long targetOffset = raw & ~1L;
            Address target = a(targetOffset);
            if (!memory.contains(target)) {
                continue;
            }

            if (i >= 16 && (targetOffset == vectorBase + 0x3a2L ||
                            targetOffset == vectorBase + 0x292L)) {
                if (getSymbolAt(target) == null) {
                    createLabel(target, "Default_Handler", true);
                }
                seed(targetOffset, "Default_Handler");
            }
            else {
                seed(targetOffset, VECTOR_NAMES[i]);
            }

            if (listing.isUndefined(slot, slot.add(3))) {
                createData(slot, new Pointer32DataType());
            }
        }
    }

    private void seed(long offset, String name) throws Exception {
        Address address = a(offset);
        if (!memory.contains(address)) {
            return;
        }
        if (listing.getInstructionAt(address) == null) {
            currentProgram.getProgramContext().setValue(tmode, address, address, BigInteger.ONE);
            AddressSet set = new AddressSet(address, address);
            DisassembleCommand command = new DisassembleCommand(set, null, true);
            command.setInitialContext(new RegisterValue(tmode, BigInteger.ONE));
            command.applyTo(currentProgram, monitor);
        }
        Function function = listing.getFunctionAt(address);
        if (function == null) {
            function = createFunction(address, name);
        }
        else if (function.getName().startsWith("FUN_") ||
                 function.getName().startsWith("LAB_")) {
            function.setName(name, SourceType.USER_DEFINED);
        }
        else if (!function.getName().equals(name) && getSymbolAt(address) == null) {
            createLabel(address, name, false);
        }
    }

    private void labelPeripheralBases() throws Exception {
        Map<String, Long> bases = new LinkedHashMap<>();
        bases.put("CM_AES", 0x40008000L); bases.put("CM_HASH", 0x40008400L);
        bases.put("CM_CRC", 0x40008c00L); bases.put("CM_EFM", 0x40010400L);
        bases.put("CM_AOS", 0x40010800L); bases.put("CM_EMB0", 0x40017c00L);
        bases.put("CM_EMB1", 0x40017c20L); bases.put("CM_EMB2", 0x40017c40L);
        bases.put("CM_EMB3", 0x40017c60L); bases.put("CM_SPI1", 0x4001c000L);
        bases.put("CM_USART1", 0x4001cc00L); bases.put("CM_USART2", 0x4001d000L);
        bases.put("CM_USART3", 0x4001d400L); bases.put("CM_SPI2", 0x4001c400L);
        bases.put("CM_SPI3", 0x40020000L); bases.put("CM_USART4", 0x40020c00L);
        bases.put("CM_USART5", 0x40021000L); bases.put("CM_USART6", 0x40021400L);
        bases.put("CM_TMR0_1", 0x40024000L); bases.put("CM_TMR0_2", 0x40024400L);
        bases.put("CM_TMRA_5", 0x40026000L); bases.put("CM_MCAN1", 0x40029000L);
        bases.put("CM_MCAN2", 0x40029400L); bases.put("CM_TMR4_1", 0x40038000L);
        bases.put("CM_TMR4_2", 0x40038400L); bases.put("CM_CMP1", 0x40038800L);
        bases.put("CM_CMP2", 0x40038900L); bases.put("CM_CMP3", 0x40038c00L);
        bases.put("CM_CMP4", 0x40038d00L); bases.put("CM_TMR4_3", 0x40038e00L);
        bases.put("CM_TMRA_1", 0x4003a000L); bases.put("CM_TMRA_2", 0x4003a400L);
        bases.put("CM_TMRA_3", 0x4003a800L); bases.put("CM_TMRA_4", 0x4003ac00L);
        bases.put("CM_I2C1", 0x4003b400L); bases.put("CM_I2C2", 0x4003b800L);
        bases.put("CM_TMR6_1", 0x4003c000L); bases.put("CM_TMR6_COMMON", 0x4003c300L);
        bases.put("CM_TMR6_2", 0x4003c400L); bases.put("CM_ADC1", 0x40040000L);
        bases.put("CM_ADC2", 0x40040400L); bases.put("CM_ADC3", 0x40040800L);
        bases.put("CM_DAC", 0x40041000L); bases.put("CM_TRNG", 0x40042000L);
        bases.put("CM_CMU", 0x40048000L); bases.put("CM_RTC", 0x4004c000L);
        bases.put("CM_RMU", 0x4004cce0L); bases.put("CM_MPU", 0x40050000L);
        bases.put("CM_SRAMC", 0x40050800L); bases.put("CM_KEYSCAN", 0x40050c00L);
        bases.put("CM_INTC", 0x40051000L); bases.put("CM_DMA1", 0x40053000L);
        bases.put("CM_DMA2", 0x40053400L); bases.put("CM_GPIO", 0x40053800L);
        bases.put("CM_DCU1", 0x40056000L); bases.put("CM_DCU2", 0x40056400L);
        bases.put("CM_DCU3", 0x40056800L); bases.put("CM_DCU4", 0x40056c00L);
        bases.put("NVIC", 0xe000e100L); bases.put("SCB", 0xe000ed00L);
        bases.put("FPU", 0xe000ef30L); bases.put("DWT", 0xe0001000L);
        for (Map.Entry<String, Long> item : bases.entrySet()) {
            if (getSymbolAt(a(item.getValue())) == null) {
                createLabel(a(item.getValue()), item.getKey(), true);
            }
        }
    }
}
