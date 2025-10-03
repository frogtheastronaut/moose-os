# Resolve repo root (one directory up from scripts/)
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT_H="$REPO_ROOT/user/scripts/include/elf_upload.h"

if [ $# -ne 1 ]; then
    echo "Usage: $0 <file>.elf"
    echo "Example: $0 hello.elf"
    exit 1
fi

ELF_NAME="$1"
ELF_FILE="$REPO_ROOT/programs/$ELF_NAME"

if [ ! -f "$ELF_FILE" ]; then
    echo "Error: ELF file '$ELF_FILE' not found in programs/"
    exit 1
fi

mkdir -p "$(dirname "$OUT_H")"

# Generate raw xxd header first
TMP="$(mktemp)"
xxd -i "$ELF_FILE" > "$TMP"

# Extract original symbol name
ORIG_NAME="$(sed -n 's/^unsigned char \([A-Za-z0-9_]*\)\[\].*/\1/p' "$TMP" | head -n1)"
if [ -z "$ORIG_NAME" ]; then
    echo "Error: could not parse xxd output"
    rm -f "$TMP"
    exit 1
fi

# Replace symbol with consistent upload_elf
sed "s/${ORIG_NAME}/upload_elf/g" "$TMP" > "$OUT_H"
rm -f "$TMP"

# Prepend header guard and define, then append helper inline function
cat > "$OUT_H.tmp" <<EOF
#ifndef ELF_UPLOAD_H
#define ELF_UPLOAD_H

#define UPLOADED_ELF

EOF

cat "$OUT_H" >> "$OUT_H.tmp"

cat >> "$OUT_H.tmp" <<EOF

#include "file/file.h"

// Auto-generated helper to install this ELF into the filesystem
static inline int elf_install(const char *dest_name) {
    // Ensure filesystem is initialized / directory set before calling
    return filesystem_make_file_binary(dest_name, (const char*)upload_elf, upload_elf_len);
}

#endif // ELF_UPLOAD_H
EOF

mv "$OUT_H.tmp" "$OUT_H"

echo "Generated $OUT_H."
