/* SPDX-License-Identifier: EUPL-1.2 */
#ifndef X11_HANDLER_H
#define X11_HANDLER_H

#include <stdint.h>
#include <X11/Xlib.h>

struct xdata {
	int fd;

	unsigned int width;
	unsigned int height;

	uint32_t *buffer;

	Display *display;
	Window window;
	XImage *image;
};

struct xdata *xhandler_init(void);

void xhandler_resize(struct xdata *data, unsigned int width, unsigned int height);

void xhandler_draw(struct xdata *data, void *buffer, uint32_t size);

uint8_t xhandler_routine(struct xdata *data);

void xhandler_destroy(struct xdata *data);

#endif /* X11_HANDLER_H */