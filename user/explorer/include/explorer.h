/*
    MooseOS Explorer code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#ifndef EXPLROR_H
#define EXPLROR_H

#include "gui/gui.h"
#include "editor.h"
#include "dock.h"
#include "heap/heap.h"

extern void draw_cursor(void);

// current selection for files/folders
extern int current_selection;

bool explorer_handle_mouse(void);

#endif
