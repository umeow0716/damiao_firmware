// Export a reproducible reverse-engineering snapshot for source reconstruction.
// @category DM4310

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.Set;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;

public class ExportRecovery extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) {
            throw new IllegalArgumentException("usage: ExportRecovery.java <output-directory>");
        }

        String stem = currentProgram.getName().toLowerCase().contains("app") ? "app" : "bootloader";
        File root = new File(args[0]);
        if (!root.exists() && !root.mkdirs()) {
            throw new IllegalStateException("Cannot create " + root);
        }

        File functionsFile = new File(root, stem + "_functions.tsv");
        File edgesFile = new File(root, stem + "_calls.tsv");
        File stringsFile = new File(root, stem + "_strings.tsv");
        File sourceFile = new File(root, stem + "_ghidra.c");
        File assemblyFile = new File(root, stem + "_asm.tsv");

        try (PrintWriter functions = new PrintWriter(new BufferedWriter(new FileWriter(functionsFile)));
             PrintWriter edges = new PrintWriter(new BufferedWriter(new FileWriter(edgesFile)));
             PrintWriter strings = new PrintWriter(new BufferedWriter(new FileWriter(stringsFile)));
             PrintWriter source = new PrintWriter(new BufferedWriter(new FileWriter(sourceFile)));
             PrintWriter assembly = new PrintWriter(new BufferedWriter(new FileWriter(assemblyFile)))) {

            functions.println("address\tsize\tname\tcalling_convention");
            edges.println("caller_address\tcaller\tcallee_address\tcallee");
            strings.println("address\tvalue\treference_from");
            assembly.println(
                "function_address\tfunction\tinstruction_address\tbytes\tinstruction"
            );
            source.println("/* Raw Ghidra decompilation. This is evidence, not hand-cleaned source. */");
            source.println("/* Program: " + currentProgram.getName() + " */\n");

            DecompInterface decompiler = new DecompInterface();
            decompiler.toggleCCode(true);
            decompiler.toggleSyntaxTree(true);
            if (!decompiler.openProgram(currentProgram)) {
                throw new IllegalStateException("Decompiler failed to open program");
            }

            FunctionIterator iterator = currentProgram.getFunctionManager().getFunctions(true);
            int count = 0;
            while (iterator.hasNext() && !monitor.isCancelled()) {
                Function function = iterator.next();
                functions.printf("%s\t%d\t%s\t%s%n",
                    function.getEntryPoint(), function.getBody().getNumAddresses(),
                    clean(function.getName()), clean(function.getCallingConventionName()));

                Set<Function> called = function.getCalledFunctions(monitor);
                for (Function callee : called) {
                    edges.printf("%s\t%s\t%s\t%s%n",
                        function.getEntryPoint(), clean(function.getName()),
                        callee.getEntryPoint(), clean(callee.getName()));
                }

                InstructionIterator instructions = currentProgram.getListing()
                    .getInstructions(function.getBody(), true);
                while (instructions.hasNext()) {
                    Instruction instruction = instructions.next();
                    assembly.printf("%s\t%s\t%s\t%s\t%s%n",
                        function.getEntryPoint(), clean(function.getName()),
                        instruction.getAddress(), hex(instruction.getBytes()),
                        clean(instruction.toString()));
                }

                source.printf("\n/* ===== %s @ %s, %d bytes ===== */%n",
                    function.getName(), function.getEntryPoint(), function.getBody().getNumAddresses());
                DecompileResults result = decompiler.decompileFunction(function, 30, monitor);
                if (result.decompileCompleted() && result.getDecompiledFunction() != null) {
                    source.println(result.getDecompiledFunction().getC());
                }
                else {
                    source.println("/* DECOMPILATION FAILED: " +
                        clean(result.getErrorMessage()) + " */");
                }
                count++;
            }
            decompiler.dispose();

            DataIterator dataIterator = currentProgram.getListing().getDefinedData(true);
            while (dataIterator.hasNext() && !monitor.isCancelled()) {
                Data data = dataIterator.next();
                if (!data.hasStringValue()) {
                    continue;
                }
                String value = clean(String.valueOf(data.getValue()));
                ReferenceIterator references = currentProgram.getReferenceManager()
                    .getReferencesTo(data.getAddress());
                if (!references.hasNext()) {
                    strings.printf("%s\t%s\t%n", data.getAddress(), value);
                }
                while (references.hasNext()) {
                    Reference reference = references.next();
                    strings.printf("%s\t%s\t%s%n", data.getAddress(), value,
                        reference.getFromAddress());
                }
            }

            println("Exported " + count + " functions from " + currentProgram.getName());
        }
    }

    private String clean(String value) {
        if (value == null) {
            return "";
        }
        return value.replace("\\", "\\\\")
                    .replace("\t", "\\t")
                    .replace("\r", "\\r")
                    .replace("\n", "\\n");
    }

    private String hex(byte[] bytes) {
        StringBuilder result = new StringBuilder(bytes.length * 2);
        for (byte value : bytes) {
            result.append(String.format("%02x", value & 0xff));
        }
        return result.toString();
    }
}
