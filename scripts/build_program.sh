# Figure out the repo root
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if [ $# -ne 1 ]; then
    echo "Usage: $0 <file>.c"
    exit 1
fi

SRC_NAME="$1"
SRC_FILE="$REPO_ROOT/programs/$SRC_NAME"
BASENAME="$(basename "$SRC_NAME" .c)"
OUT_FILE="$REPO_ROOT/programs/${BASENAME}.elf"

if [ ! -f "$SRC_FILE" ]; then
    echo "Error: source file '$SRC_FILE' not found"
    exit 1
fi

# Compile C → ELF using cross-compiler for i386 (freestanding)
i386-elf-gcc -ffreestanding -nostdlib -o "$OUT_FILE" "$SRC_FILE"

echo "Built $OUT_FILE from $SRC_FILE"
