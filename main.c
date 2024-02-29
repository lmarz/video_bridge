/* SPDX-License-Identifier: EUPL-1.2 */
#include <poll.h>
#include <stdio.h>

#include "dbus_handler.h"
#include "pipewire_handler.h"
#include "x11_handler.h"

int main(int argc, char *argv[]) {
	struct ddata *ddata;
	struct xdata *xdata;
	struct pdata *pdata;
	struct pollfd polls[2];

	ddata = dhandler_init();
	if(!ddata)
		goto err;

	if(dhandler_create_session(ddata))
		goto err_dbus;

	if(dhandler_select_sources(ddata))
		goto err_dbus;

	if(dhandler_start(ddata))
		goto err_dbus;

	if(dhandler_open_pipewire_remote(ddata))
		goto err_dbus;

	xdata = xhandler_init();
	if(!xdata)
		goto err_dbus;

	pw_init(&argc, &argv);

	pdata = phandler_init(ddata->pipewire_fd, ddata->node_id, xdata);
	if(!pdata)
		goto err_xlib;

	polls[0].fd = ddata->fd;
	polls[0].events = POLLIN;
	polls[1].fd = xdata->fd;
	polls[1].events = POLLIN;

	while(poll(polls, 2, 1000) >= 0) {
		if(polls[0].revents != 0) {
			if(polls[0].revents & POLLIN) {
				if(dhandler_routine(ddata)) {
					fprintf(stderr, "dbus problem\n");
					break;
				}
			} else if(polls[0].revents & (POLLERR | POLLHUP)) {
				fprintf(stderr, "dbus fd problem\n");
				break;
			}
		}
		if(polls[1].revents != 0) {
			if(polls[1].revents & POLLIN) {
				if(xhandler_routine(xdata)) {
					fprintf(stderr, "xlib problem\n");
					break;
				}
			} else if(polls[1].revents & (POLLERR | POLLHUP)) {
				fprintf(stderr, "xlib fd problem\n");
				break;
			}
		}
	}

	phandler_destroy(pdata);
	pw_deinit();
	xhandler_destroy(xdata);
	dhandler_destroy(ddata);

	return 0;

err_xlib:
	pw_deinit();
	xhandler_destroy(xdata);
err_dbus:
	dhandler_destroy(ddata);
err:
	return 1;
}
