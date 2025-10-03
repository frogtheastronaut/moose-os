#!/bin/bash
# main file that builds, generates header, 
# and compiles the OS with uploaded ELF

# directories
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ $# -ne 1 ]; then
	echo "Usage: $0 <file>.c"
	echo "Example: $0 hello.c"
	exit 1
fi

ELF_NAME="$1"

echo "Building program: $ELF_NAME"
if ! "$SCRIPT_DIR/build_program.sh" "$ELF_NAME"; then
    echo "Error: Failed to build program $ELF_NAME"
    exit 1
fi

# Convert .c filename to .elf filename for header generation
BASENAME="$(basename "$ELF_NAME" .c)"
ELF_FILE="${BASENAME}.elf"

echo "Generating upload header for: $ELF_FILE"
if ! "$SCRIPT_DIR/generate_upload_header.sh" "$ELF_FILE"; then
    echo "Error: Failed to generate upload header for $ELF_FILE"
    exit 1
fi

echo "Building MooseOS with uploaded ELF..."
cd "$REPO_ROOT"
if ! make; then
    echo "Error: Failed to build MooseOS"
    exit 1
fi

echo "Cleaning up upload header..."
echo "#ifndef ELF_UPLOAD_H" > "$REPO_ROOT/user/scripts/include/elf_upload.h"
echo "#define ELF_UPLOAD_H" >> "$REPO_ROOT/user/scripts/include/elf_upload.h"
echo "" >> "$REPO_ROOT/user/scripts/include/elf_upload.h"
echo "// No ELF uploaded" >> "$REPO_ROOT/user/scripts/include/elf_upload.h"
echo "" >> "$REPO_ROOT/user/scripts/include/elf_upload.h"
echo "#endif // ELF_UPLOAD_H" >> "$REPO_ROOT/user/scripts/include/elf_upload.h"

echo "Successfully built MooseOS with uploaded ELF: $ELF_NAME"