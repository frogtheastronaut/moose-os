/*
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang and Contributors.

    Header file for ../explorer.c
*/
#ifndef EXPLROR_H
#define EXPLROR_H

// Includes
#include "gui/gui.h"
#include "editor.h"
#include "dock.h"
#include "heap/malloc.h"

// External functions
extern void draw_cursor(void);

// Current selection for files/folders
extern int current_selection;

// Function prototypes
bool explorer_handle_mouse(void);

#endif
