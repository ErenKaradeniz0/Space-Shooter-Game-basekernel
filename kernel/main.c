#include "console.h"
#include "page.h"
#include "process.h"
#include "keyboard.h"
#include "mouse.h"
#include "interrupt.h"
#include "clock.h"
#include "ata.h"
#include "device.h"
#include "cdromfs.h"
#include "string.h"
#include "graphics.h"
#include "kernel/ascii.h"
#include "kernel/syscall.h"
#include "rtc.h"
#include "kernelcore.h"
#include "kmalloc.h"
#include "memorylayout.h"
#include "kshell.h"
#include "cdromfs.h"
#include "diskfs.h"
#include "serial.h"

#define SIDE_BAR_WIDTH 20

struct console *console;
int X_SIZE, Y_SIZE;

void drawBoundaries(struct console *console) {
    // Draw vertical boundaries
    for (int i = 0; i < Y_SIZE; i++) {
        kprint_at(console, 0, i, "#");                       // Left boundary
        kprint_at(console, SIDE_BAR_WIDTH, i, "#");          // Right boundary
        kprint_at(console, X_SIZE - 1, i, "#");              // Right-most boundary
    }

    // Draw horizontal boundaries
    for (int i = 0; i < X_SIZE; i++) {
        kprint_at(console, i, 0, "#");                       // Top boundary
        kprint_at(console, i, Y_SIZE - 1, "#");              // Bottom boundary
    }
}


int kernel_main() {
	
    console = console_create_root();
    console_addref(console);
	console_size(console, &X_SIZE, &Y_SIZE);
    page_init();
    kmalloc_init((char *) KMALLOC_START, KMALLOC_LENGTH);
    
	drawBoundaries(console);

	//keyboard_init();
    //rtc_init();
    //clock_init();
    //interrupt_init();
    //mouse_init();
    //process_init();
    //ata_init();
    //cdrom_init();
    //diskfs_init();

    return 0;
}
