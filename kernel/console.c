/*
Copyright (C) 2015-2019 The University of Notre Dame
This software is distributed under the GNU General Public License.
See the file LICENSE for details.
*/

#include "console.h"
#include "serial.h"
#include "graphics.h"
#include "kmalloc.h"
#include "string.h"
#include "main.h"

struct console {
	struct window *window;
	struct graphics *gx;
	int xsize;
	int ysize;
	int xpos;
	int ypos;
	int onoff;
	int refcount;
};

struct console console_root = {0};

static struct graphics_color bgcolor = { 0, 0, 0 };
static struct graphics_color fgcolor = { 255, 255, 255 };

struct console * console_create_root()
{
	console_root.window = window_create_root();
	console_root.gx = window_graphics(console_root.window);
	console_reset(&console_root);
	//console_putstring(&console_root,"\nconsole: initialized\n");
	return &console_root;
}

void console_reset( struct console *d )
{
	if(!d || !d->gx) return;
	d->xpos = d->ypos = 0;
	d->xsize = graphics_width(d->gx) / 8;
	d->ysize = graphics_height(d->gx) / 8;
	d->onoff = 0;
	graphics_fgcolor(d->gx, fgcolor);
	graphics_bgcolor(d->gx, bgcolor);
	graphics_clear(d->gx, 0, 0, graphics_width(d->gx), graphics_height(d->gx));
}

void console_heartbeat( struct console *d )
{
	char c = d->onoff ? ' ' : '_';
	graphics_char(d->gx, d->xpos * 8, d->ypos * 8, c );
	d->onoff = !d->onoff;
}

int console_post( struct console *c, const char *data, int size )
{
	int total = 0;

	struct event e;
	e.type = EVENT_KEY_DOWN;
	e.x = 0;
	e.y = 0;

	while(size>0) {
		e.code = *data;
		window_post_events(c->window,&e,sizeof(e));
		size--;
		data++;
		total++;
	}

	return total;
}
int X_SIZE_CONSOLE, Y_SIZE_CONSOLE;
int console_write(struct console *d, const char *data, int size)
{
	console_size(d, &X_SIZE_CONSOLE, &Y_SIZE_CONSOLE);
    graphics_char(d->gx, d->xpos * 8, d->ypos * 8, ' ');

    int i;
    for (i = 0; i < size; i++) {
        char c = data[i];
        switch (c) {
            case 13:
            case 10:
                d->xpos = 0;
                if (d->ypos + 1 < Y_SIZE_CONSOLE) { // Check if moving down is within bounds
                    d->ypos++;
                }
                break;
            case '\f':
                d->xpos = d->ypos = 0;
                d->xsize = graphics_width(d->gx) / 8;
                d->ysize = graphics_height(d->gx) / 8;
                graphics_fgcolor(d->gx, fgcolor);
                graphics_bgcolor(d->gx, bgcolor);
                graphics_clear(d->gx, 0, 0, graphics_width(d->gx), graphics_height(d->gx));
                break;
            case '\b':
                if (d->xpos > 0) { // Check if moving left is within bounds
                    d->xpos--;
                }
                break;
            default:
                if (d->xpos < X_SIZE_CONSOLE) { // Check if moving right is within bounds
                    graphics_char(d->gx, d->xpos * 8, d->ypos * 8, c);
                    d->xpos++;
                }
                break;
        }
    }

    return i;
}


int console_read( struct console *c, char *data, int length )
{
	int total=0;

	struct event e;
	while(length>0 && window_read_events(c->window,&e,sizeof(e))) {
		if(e.type==EVENT_KEY_DOWN) {
			*data = e.code;
			length--;
			total++;
			data++;
		}
	}

	return total;
}

int console_read_nonblock( struct console *c, char *data, int length )
{
	int total=0;

	struct event e;
	while(length>0 && window_read_events_nonblock(c->window,&e,sizeof(e))) {
		if(e.type==EVENT_KEY_DOWN) {
			*data = e.code;
			length--;
			total++;
			data++;
		}
	}

	return total;
}

int console_getchar( struct console *c )
{
	char ch;
	if(console_read(c,&ch,1)>0) {
		return ch;
	} else {
		return 0;
	}
}

void console_putchar( struct console *c, char ch )
{
	console_write(c,&ch,1);
}

void console_putstring( struct console *c, const char *str)
{
	console_write(c,str,strlen(str));
}

struct console *console_create( struct window *w )
{
	struct console *c = kmalloc(sizeof(*c));
	c->window = window_addref(w);
	c->gx = window_graphics(w);
	c->refcount = 1;
	console_reset(c);
	return c;
}

struct console *console_addref( struct console *c )
{
	c->refcount++;
	return c;
}

void console_delete( struct console *c )
{
	if(c==&console_root) return;

	c->refcount--;
	if(c->refcount==0) {
		window_delete(c->window);
		kfree(c);
	}
}

void console_size( struct console *c, int *xsize, int *ysize )
{
	*xsize = c->xsize;
	*ysize = c->ysize;
}


void console_set_cursor(struct console *c, int x, int y) {
    c->xpos = x;
    c->ypos = y;
}

void kprint_at(struct console *console, int x, int y, const char *str) {
    console_set_cursor(console, x, y); // Set cursor position
    console_putstring(console, str); // Print the string at the specified position
}