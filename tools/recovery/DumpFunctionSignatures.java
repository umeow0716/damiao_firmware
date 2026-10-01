// Ghidra headless script: export relocation-normalized instruction signatures.
// @category DM.Recovery

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.scalar.Scalar;

public class DumpFunctionSignatures extends GhidraScript {
    private static String hex(byte[] bytes) {
        StringBuilder result = new StringBuilder(bytes.length * 2);
        for (byte value : bytes) {
            result.append(String.format("%02x", value & 0xff));
        }
        return result.toString();
    }

    private static void update(MessageDigest digest, String value) {
        digest.update(value.getBytes(StandardCharsets.UTF_8));
        digest.update((byte) 0);
    }

    private static String normalizedObject(Object object) {
        if (object instanceof Address) {
            return "ADDRESS";
        }
        if (object instanceof Register) {
            return "REGISTER:" + ((Register) object).getName();
        }
        if (object instanceof Scalar) {
            Scalar scalar = (Scalar) object;
            return "SCALAR:" + scalar.bitLength() + ":" +
                Long.toUnsignedString(scalar.getUnsignedValue(), 16);
        }
        return object.getClass().getSimpleName() + ":" + object.toString();
    }

    @Override
    protected void run() throws Exception {
        String[] arguments = getScriptArgs();
        if (arguments.length != 1) {
            throw new IllegalArgumentException(
                "usage: DumpFunctionSignatures <output-path>");
        }

        try (BufferedWriter output = new BufferedWriter(
                new FileWriter(arguments[0]))) {
            output.write(
                "entry\tname\tinstructions\tshape_sha256\t"
                + "normalized_sha256\n");
            FunctionIterator functions =
                currentProgram.getFunctionManager().getFunctions(true);
            while (functions.hasNext() && !monitor.isCancelled()) {
                Function function = functions.next();
                MessageDigest shape = MessageDigest.getInstance("SHA-256");
                MessageDigest normalized = MessageDigest.getInstance("SHA-256");
                InstructionIterator instructions = currentProgram.getListing()
                    .getInstructions(function.getBody(), true);
                long count = 0;
                while (instructions.hasNext()) {
                    Instruction instruction = instructions.next();
                    ++count;
                    String mnemonic = instruction.getMnemonicString();
                    update(shape, mnemonic);
                    update(shape, Integer.toString(instruction.getNumOperands()));
                    update(normalized, mnemonic);
                    update(normalized,
                        Integer.toString(instruction.getNumOperands()));
                    for (int operand = 0;
                            operand < instruction.getNumOperands(); ++operand) {
                        update(shape,
                            Integer.toString(instruction.getOperandType(operand)));
                        update(normalized,
                            Integer.toString(instruction.getOperandType(operand)));
                        for (Object object :
                                instruction.getOpObjects(operand)) {
                            update(normalized, normalizedObject(object));
                        }
                    }
                }
                output.write("0x" + function.getEntryPoint());
                output.write("\t" + function.getName());
                output.write("\t" + count);
                output.write("\t" + hex(shape.digest()));
                output.write("\t" + hex(normalized.digest()));
                output.newLine();
            }
        }
    }
}
