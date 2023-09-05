#pragma once

#include <stdbool.h>

void console_init(void);
void console_free(void);

// returns true if current console is attached to parent terminal window
bool console_is_attached(void);

// returns true if separate console window has been allocated
bool console_is_allocated(void);

// blocks till any key is pressed
void console_wait_anykey(void);
