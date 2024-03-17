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

// Draw A
void draw_a(struct graphics *g, int x, int y, int w, int h)
{
    graphics_line(g, x, y, w + 3, h + 3);
    graphics_line(g, x, y, -w - 3, h + 3);
    graphics_line(g, x - 10, y + 10, 0, h + 5);
    graphics_line(g, x + 10, y + 10, 0, h + 5);
    graphics_line(g, x - 10, y + 15, 20, 0);
}

void drawSpaceship(struct graphics *g, int x, int y, int w, int h)
{
    // Draw A
    draw_a(g, x, y, w, h);
    draw_a(g, x + 100, y, w, h);
    draw_a(g, x, y + 40, w, h);
    draw_a(g, x + 100, y + 40, w, h);

    // Draw I
    graphics_line(g, x + 50, y, 0, h + 12);

    // Draw /
    graphics_line(g, x + 40, y + 40, -w - 2, h + 2);
    // Draw '\'
    graphics_line(g, x + 60, y + 40, w + 2, h + 2);
    // Draw -
    graphics_line(g, x + 40, y + 45, w + 12, 0);

    // Draw /
    graphics_line(g, x + 10, y + 75, -w - 12, h + 12);
    // Draw o
    graphics_circle(g, x + 20, y + 80, w - 3);
    graphics_circle(g, x + 50, y + 80, h - 3);
    graphics_circle(g, x + 80, y + 80, w - 3);
    // Draw '\'
    graphics_line(g, x + 90, y + 75, w + 12, h + 12);
}

void init(struct graphics *g)
{
    // initBullets();
    // initRockets();
    // intro(g);
    drawBoundaries(g);

    // x = (X_SIZE - SPACE_SHIP_WIDTH + SIDE_BAR_WIDTH) / 2; // Starting position for spaceship
    // y = Y_SIZE - SPACE_SHIP_HEIGHT - 1;                   // Adjusted starting position for the spaceship
}

// void intro(struct graphics *g)
// {
//     // Clear the screen and draw boundaries
//     console_reset(g);
//     drawBoundaries(g);

//     info(g);

//     kprint_at(g, 2, 17, "Bullets:");
//     printBulletCount(g, 11, 17);

//     kprint_at(g, 2, 18, "Score:");
//     printScore(g, 10, 18);
// }

int X_SIZE = 1016;
int Y_SIZE = 760;
#define SIDE_BAR_WIDTH 192
void drawBoundaries(struct graphics *g)
{
    // Draw vertical boundaries
    for (int i = 0; i <= Y_SIZE; i += 8)
    {
        graphics_char(g, 0, i, '#');
        graphics_char(g, SIDE_BAR_WIDTH, i, '#'); // Right boundary
        graphics_char(g, X_SIZE, i, '#');     // Right-most boundary
    }

    // Draw horizontal boundaries
    for (int i = 0; i <= X_SIZE; i += 8)
    {
        graphics_char(g, i,0, '#');          // Top boundary
        graphics_char(g, i, Y_SIZE, '#'); // Bottom boundary
    }
}
int kernel_main()
{

    page_init();
    kmalloc_init((char *)KMALLOC_START, KMALLOC_LENGTH);
    interrupt_init();
    keyboard_init();
    process_init();

    struct graphics *root_graphics = graphics_create_root();
    struct graphics *g = graphics_create(root_graphics);
    drawBoundaries(g);
    // drawSpaceship(g,500,600,8,8);

    // console_size(console, &X_SIZE, &Y_SIZE);

    // Game loop
    while (1)
    {

        // busy_wait(1000); // Wait for 50 milliseconds using busy wait
    }
    graphics_delete(g);
    return 0;
}
