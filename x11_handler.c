/* SPDX-License-Identifier: EUPL-1.2 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xutil.h>

#include "x11_handler.h"

struct xdata *xhandler_init(void) {
	struct xdata *data;

	data = calloc(sizeof(struct xdata), 1);

	XInitThreads();
	data->display = XOpenDisplay(NULL);
	XLockDisplay(data->display);
	data->window = XCreateSimpleWindow(data->display, RootWindow(data->display, 0), 0, 0, 800, 600, 1, 0, 0);
	data->fd = ConnectionNumber(data->display);
	XSelectInput(data->display, data->window, ExposureMask);
	XStoreName(data->display, data->window, "Video Bridge");
	XMapWindow(data->display, data->window);
	XUnlockDisplay(data->display);

	return data;
}

void xhandler_resize(struct xdata *data, unsigned int width, unsigned int height) {
	XWindowChanges changes;

	data->width = width;
	data->height = height;

	XLockDisplay(data->display);
	if(data->image)
		XDestroyImage(data->image);

	data->buffer = malloc(width * height * 4);
	data->image = XCreateImage(data->display, DefaultVisual(data->display, 0), DefaultDepth(data->display, DefaultScreen(data->display)), ZPixmap, 0, (char *)data->buffer, width, height, 32, 0);

	changes.width = width;
	changes.height = height;

	XConfigureWindow(data->display, data->window, CWWidth | CWHeight, &changes);
	XUnlockDisplay(data->display);
}

void xhandler_draw(struct xdata *data, void *buffer, uint32_t size) {
	memcpy(data->buffer, buffer, size);
	XLockDisplay(data->display);
	XPutImage(data->display, data->window, DefaultGC(data->display, 0), data->image, 0, 0, 0, 0, data->width, data->height);
	XUnlockDisplay(data->display);
}

uint8_t xhandler_routine(struct xdata *data) {
	XLockDisplay(data->display);
	while(XPending(data->display)) {
		XEvent ev;
		XNextEvent(data->display, &ev);
		// Maybe TODO
	}
	XUnlockDisplay(data->display);

	return 0;
}

void xhandler_destroy(struct xdata *data) {
	if(data->image)
		XDestroyImage(data->image);
	XDestroyWindow(data->display, data->window);
	XCloseDisplay(data->display);
	free(data);
}