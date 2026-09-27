#!/usr/bin/env sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ghidra_home=${GHIDRA_HOME:-/home/umeow/Downloads/ghidra_12.1.3_PUBLIC}
java_home=${JAVA_HOME:-/home/umeow/.sdkman/candidates/java/25.0.4-tem}
export JAVA_HOME="$java_home"

# Headless Ghidra still writes an OSGi cache and user preferences.  Keep those
# files out of the operator's home directory so this script also works in a
# read-only/sandboxed recovery environment.
ghidra_runtime=/tmp/dm4310-ghidra-runtime
mkdir -p "$ghidra_runtime/home" "$ghidra_runtime/tmp"
if [ -n "${_JAVA_OPTIONS:-}" ]; then
  export _JAVA_OPTIONS="-Duser.home=$ghidra_runtime/home -Djava.io.tmpdir=$ghidra_runtime/tmp $_JAVA_OPTIONS"
else
  export _JAVA_OPTIONS="-Duser.home=$ghidra_runtime/home -Djava.io.tmpdir=$ghidra_runtime/tmp"
fi

headless="$ghidra_home/support/analyzeHeadless"
project_dir="$repo_dir/analysis/ghidra"
scripts="$repo_dir/analysis/scripts"
output="$repo_dir/recovered/raw"

# This is the historical program name inside the existing Ghidra database,
# not a dependency on the deleted root-level file.  Its imported bytes match
# reference/official/APP_DM4310_V3_V5017_04.decrypted.bin exactly.
"$headless" "$project_dir" dm4310 \
  -process app_mem.bin \
  -scriptPath "$scripts" \
  -postScript PrepareHC32F448.java \
  -postScript ApplyRecoveredNames.java \
  -postScript ExportRecovery.java "$output"

"$headless" "$project_dir" dm4310 \
  -process bootloader.bin \
  -scriptPath "$scripts" \
  -postScript PrepareHC32F448.java \
  -postScript ApplyRecoveredNames.java \
  -postScript ExportRecovery.java "$output"
