// Ghidra headless script: emit every decoded instruction outside a function body.
// @category DM4310.Recovery

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.symbol.ReferenceManager;

public class DumpUnownedInstructions extends GhidraScript {
    private String referencesTo(ReferenceManager references, Address address) {
        List<String> values = new ArrayList<>();
        ReferenceIterator iterator = references.getReferencesTo(address);
        while (iterator.hasNext()) {
            Reference reference = iterator.next();
            values.add(reference.getFromAddress().toString());
        }
        Collections.sort(values);
        return String.join(",", values);
    }

    @Override
    protected void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length != 1) {
            throw new IllegalArgumentException(
                "usage: DumpUnownedInstructions <output-path>");
        }

        Listing listing = currentProgram.getListing();
        FunctionManager functions = currentProgram.getFunctionManager();
        ReferenceManager references = currentProgram.getReferenceManager();
        InstructionIterator instructions = listing.getInstructions(true);
        try (BufferedWriter output = new BufferedWriter(
                new FileWriter(arguments[0]))) {
            output.write("address\tend\tlength\tmnemonic\tflow_type\tmemory_block\treferences_from\n");
            while (instructions.hasNext() && !monitor.isCancelled()) {
                Instruction instruction = instructions.next();
                Address address = instruction.getAddress();
                if (functions.getFunctionContaining(address) != null) {
                    continue;
                }
                MemoryBlock block = currentProgram.getMemory().getBlock(address);
                output.write("0x" + address);
                output.write("\t0x" + instruction.getMaxAddress());
                output.write("\t" + instruction.getLength());
                output.write("\t" + instruction.getMnemonicString());
                output.write("\t" + instruction.getFlowType());
                output.write("\t" + (block == null ? "" : block.getName()));
                output.write("\t" + referencesTo(references, address));
                output.newLine();
            }
        }
    }
}
