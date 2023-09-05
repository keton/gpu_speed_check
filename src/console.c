#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <windows.h>

#include "console.h"

static bool attached_console = false;
static bool allocated_console = false;

void console_init(void)
{
	attached_console = AttachConsole(ATTACH_PARENT_PROCESS);
	if(!attached_console) {
		allocated_console = AllocConsole();
	}

	// don't mess with file handles in case of console application
	if(!attached_console && !allocated_console) {
		return;
	}

	FILE *dummy = NULL;
	freopen_s(&dummy, "CONOUT$", "wt", stdout);
	freopen_s(&dummy, "CONOUT$", "wt", stderr);
	freopen_s(&dummy, "CONIN$", "r", stdin);

	// clear command prompt that was printed into the first line
	if(attached_console) {
		printf("\33[2K\r");
	}
}

void console_free(void)
{
	// send fake enter code to unblock command line after exit
	if(attached_console) {
		SendMessage(GetConsoleWindow(), WM_CHAR, VK_RETURN, 0);
	}

	if(attached_console || allocated_console) {
		FreeConsole();
	}
}

// returns true if current console is attached to parent terminal window
bool console_is_attached(void)
{
	return attached_console;
}

// returns true if separate console window has been allocated
bool console_is_allocated(void)
{
	return allocated_console;
}

// blocks till any key is pressed
void console_wait_anykey(void)
{
	DWORD cc;
	INPUT_RECORD irec;
	while(ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &irec, 1, &cc)) {
		if(irec.EventType == KEY_EVENT && ((irec.Event).KeyEvent).bKeyDown)
			break;
	}
}
