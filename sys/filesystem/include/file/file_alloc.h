#ifndef FILE_ALLOC_H
#define FILE_ALLOC_H
#include "file.h"


File* file_alloc();
void file_free(File* file);

#endif // FILE_ALLOC_H