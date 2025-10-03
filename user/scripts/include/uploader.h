#ifndef ELF_UPLOADER_H
#define ELF_UPLOADER_H

#include "elf_upload.h"

// uploaded elf should have UPLOADED_ELF defined
// if not, it means that no ELF was uploaded
#ifdef UPLOADED_ELF
    // ELF is available
    #define HAS_ELF_AVAILABLE 1
#else
    // no elf uploaded
    #define HAS_ELF_AVAILABLE 0
#endif

// Example function that uses the uploaded ELF if available
static inline int install_uploaded_program(const char *name) {
#ifdef UPLOADED_ELF
    return elf_install(name);
#else
    return -1; // No ELF available
#endif
}

#endif // ELF_UPLOADER_H