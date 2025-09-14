/**
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang and Contributors.

    Header file for ../editor.c
*/
#ifndef EDITOR_H
#define EDITOR_H

#include "gui.h"

// External variable
extern bool terminal_active;

// Functions
void editor_cursor_visible();
void editor_draw(void);

#endif