/* SPDX-License-Identifier: EUPL-1.2 */
#ifndef PIPEWIRE_HANDLER_H
#define PIPEWIRE_HANDLER_H

#include <stdint.h>

#include <pipewire/pipewire.h>

#include "x11_handler.h"

struct pdata {
	struct xdata *xdata;
	uint32_t node_id;

	struct pw_thread_loop *loop;
	struct pw_context *context;
	struct pw_core *core;

	struct pw_registry *registry;
	struct spa_hook registry_listener;

	struct pw_stream *stream;
	struct spa_hook stream_listener;
};

struct pdata *phandler_init(int fd, uint32_t node_id, struct xdata *xdata);

void phandler_destroy(struct pdata *data);

#endif /* PIPEWIRE_HANDLER_H */
