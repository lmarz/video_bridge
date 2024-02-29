/* SPDX-License-Identifier: EUPL-1.2 */
#ifndef DBUS_HANDLER_H
#define DBUS_HANDLER_H

#include <stdint.h>

#include <dbus/dbus.h>

struct ddata {
	int fd;

	DBusConnection *con;
	char *session;

	int pipewire_fd;
	uint32_t node_id;
};

struct ddata *dhandler_init(void);

uint8_t dhandler_create_session(struct ddata *data);

uint8_t dhandler_select_sources(struct ddata *data);

uint8_t dhandler_start(struct ddata *data);

uint8_t dhandler_open_pipewire_remote(struct ddata *data);

uint8_t dhandler_routine(struct ddata *data);

void dhandler_destroy(struct ddata *data);

#endif /* DBUS_HANDLER_H */