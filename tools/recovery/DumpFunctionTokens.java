// Ghidra headless script: export normalized per-instruction token sequences.
// @category DM.Recovery

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.Reference;

public class DumpFunctionTokens extends GhidraScript {
    private static String normalizedObject(Object object) {
        if (object instanceof Address) {
            return "A";
        }
        if (object instanceof Register) {
            return "R:" + ((Register) object).getName();
        }
        if (object instanceof Scalar) {
            Scalar scalar = (Scalar) object;
            return "S:" + scalar.bitLength() + ":" +
                Long.toUnsignedString(scalar.getUnsignedValue(), 16);
        }
        return object.getClass().getSimpleName() + ":" + object.toString();
    }

    private static String token(Instruction instruction) {
        StringBuilder result = new StringBuilder(
            instruction.getMnemonicString());
        for (int operand = 0;
                operand < instruction.getNumOperands(); ++operand) {
            result.append('|').append(instruction.getOperandType(operand));
            for (Object object : instruction.getOpObjects(operand)) {
                result.append('|').append(normalizedObject(object));
            }
        }
        return result.toString();
    }

    @Override
    protected void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length != 1) {
            throw new IllegalArgumentException(
                "usage: DumpFunctionTokens <output-path>");
        }

        try (BufferedWriter output = new BufferedWriter(
                new FileWriter(arguments[0]))) {
            output.write(
                "entry\tname\tinstructions\ttokens_base64\trefs_base64" +
                "\taddresses_base64\n");
            FunctionIterator functions =
                currentProgram.getFunctionManager().getFunctions(true);
            while (functions.hasNext() && !monitor.isCancelled()) {
                Function function = functions.next();
                List<String> tokens = new ArrayList<>();
                List<String> references = new ArrayList<>();
                List<String> addresses = new ArrayList<>();
                InstructionIterator instructions = currentProgram.getListing()
                    .getInstructions(function.getBody(), true);
                while (instructions.hasNext()) {
                    Instruction instruction = instructions.next();
                    tokens.add(token(instruction));
                    addresses.add(instruction.getAddress().toString());
                    List<String> targets = new ArrayList<>();
                    for (Reference reference : instruction.getReferencesFrom()) {
                        targets.add(reference.getReferenceType().toString() + "=" +
                            reference.getToAddress().toString());
                    }
                    references.add(String.join(",", targets));
                }
                String sequence = String.join("\n", tokens);
                String encoded = Base64.getEncoder().encodeToString(
                    sequence.getBytes(StandardCharsets.UTF_8));
                String referenceSequence = String.join("\n", references);
                String encodedReferences = Base64.getEncoder().encodeToString(
                    referenceSequence.getBytes(StandardCharsets.UTF_8));
                output.write("0x" + function.getEntryPoint());
                output.write("\t" + function.getName());
                output.write("\t" + tokens.size());
                output.write("\t" + encoded);
                output.write("\t" + encodedReferences);
                String addressSequence = String.join("\n", addresses);
                output.write("\t" + Base64.getEncoder().encodeToString(
                    addressSequence.getBytes(StandardCharsets.UTF_8)));
                output.newLine();
            }
        }
    }
}
