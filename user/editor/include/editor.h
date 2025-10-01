/*
    MooseOS Editor code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#ifndef EDITOR_H
#define EDITOR_H

#include "gui/gui.h"
#include "file/file.h"

// external variables
extern bool terminal_active;
extern char editor_content[MAX_CONTENT];
extern char editor_filename[MAX_NAME_LEN];
extern int editor_cursor_pos;
extern int editor_scroll_line;
extern int editor_cursor_line;
extern int editor_cursor_col;
extern bool editor_modified;

// function prototypes
void editor_cursor_visible();
void editor_draw(void);
void cursorpos2linecol(int pos, int* line, int* col);
int linecol2cursorpos(int line, int col);

#endif // EDITOR_H
