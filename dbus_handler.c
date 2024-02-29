/* SPDX-License-Identifier: EUPL-1.2 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dbus_handler.h"

#define method_call_new(method) dbus_message_new_method_call("org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.portal.ScreenCast", method)

struct ddata *dhandler_init(void) {
	struct ddata *data;
	DBusError error;

	data = calloc(sizeof(struct ddata), 1);

	dbus_error_init(&error);
	data->con = dbus_bus_get(DBUS_BUS_SESSION, &error);
	if(!data->con) {
		fprintf(stderr, "%s\n", error.message);
		free(data);
		return NULL;
	}

	/* I'm a naughty boy ;) */
	dbus_connection_get_unix_fd(data->con, &data->fd);

	return data;
}

static DBusMessage *wait_for_response(DBusConnection *conn, const char *request) {
	while(dbus_connection_read_write_dispatch(conn, 1000)) {
		DBusMessage *msg;

		while((msg = dbus_connection_pop_message(conn)) != NULL) {
			if(dbus_message_is_signal(msg, "org.freedesktop.portal.Request", "Response") &&
			   dbus_message_has_path(msg, request))
				return msg;
		}
	}

	return NULL;
}

uint8_t dhandler_open_pipewire_remote(struct ddata *data) {
	DBusError error;
	DBusMessage *msg, *reply;
	DBusMessageIter iter, asub;

	msg = method_call_new("OpenPipeWireRemote");
	if(!msg) {
		fprintf(stderr, "Fuck\n");
		return 1;
	}

	dbus_message_iter_init_append(msg, &iter);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH, &data->session);
	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &asub);
	dbus_message_iter_close_container(&iter, &asub);

	dbus_error_init(&error);
	reply = dbus_connection_send_with_reply_and_block(data->con, msg, -1, &error);
	if(!reply) {
		fprintf(stderr, "%s\n", error.message);
		return 1;
	}

	dbus_message_get_args(reply, &error, DBUS_TYPE_UNIX_FD, &data->pipewire_fd, DBUS_TYPE_INVALID);
	if(dbus_error_is_set(&error)) {
		fprintf(stderr, "%s\n", error.message);
		return 1;
	}

	return 0;
}

static uint32_t get_stream(DBusMessage *msg) {
	DBusMessageIter iter, asub;
	uint32_t res;
	int count, i;

	dbus_message_iter_init(msg, &iter);
	dbus_message_iter_get_basic(&iter, &res);

	if(res != 0)
		return 0;

	dbus_message_iter_next(&iter);
	count = dbus_message_iter_get_element_count(&iter);
	dbus_message_iter_recurse(&iter, &asub);
	for(i = 0; i < count; i++) {
		DBusMessageIter esub, vsub, sasub;
		char *key;
		int scount, si;

		dbus_message_iter_recurse(&asub, &esub);
		dbus_message_iter_get_basic(&esub, &key);

		if(strcmp(key, "streams")) {
			dbus_message_iter_next(&asub);
			continue;
		}
		
		dbus_message_iter_next(&esub);
		dbus_message_iter_recurse(&esub, &vsub);
		scount = dbus_message_iter_get_element_count(&vsub);
		dbus_message_iter_recurse(&vsub, &sasub);
		for(si = 0; i < scount; si++) {
			DBusMessageIter srsub;
			uint32_t pip_id;

			dbus_message_iter_recurse(&sasub, &srsub);
			dbus_message_iter_get_basic(&srsub, &pip_id);
			return pip_id;
		}
	}

	return 0;
}

uint8_t dhandler_start(struct ddata *data) {
	DBusError error;
	DBusMessage *msg, *reply, *response;
	DBusMessageIter iter, asub, esub, vsub;
	char *request;

	const char *parent_window = "";
	const char *handle_token = "handle_token";
	const char *video_bridge2 = "video_bridge2";

	msg = method_call_new("Start");
	if(!msg) {
		fprintf(stderr, "Fuck\n");
		return 1;
	}

	dbus_message_iter_init_append(msg, &iter);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH, &data->session);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &parent_window);
	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &asub);
	dbus_message_iter_open_container(&asub, DBUS_TYPE_DICT_ENTRY, NULL, &esub);
	dbus_message_iter_append_basic(&esub, DBUS_TYPE_STRING, &handle_token);
	dbus_message_iter_open_container(&esub, DBUS_TYPE_VARIANT, "s", &vsub);
	dbus_message_iter_append_basic(&vsub, DBUS_TYPE_STRING, &video_bridge2);
	dbus_message_iter_close_container(&esub, &vsub);
	dbus_message_iter_close_container(&asub, &esub);
	dbus_message_iter_close_container(&iter, &asub);

	dbus_error_init(&error);
	reply = dbus_connection_send_with_reply_and_block(data->con, msg, -1, &error);
	if(!reply) {
		fprintf(stderr, "%s\n", error.message);
		return 1;
	}

	dbus_message_get_args(reply, &error, DBUS_TYPE_OBJECT_PATH, &request, DBUS_TYPE_INVALID);
	response = wait_for_response(data->con, request);

	data->node_id = get_stream(response);
	return 0;
}

uint8_t dhandler_select_sources(struct ddata *data) {
	DBusError error;
	DBusMessage *msg, *reply;
	DBusMessageIter iter, asub, esub, vsub;
	char *request;

	const char *handle_token = "handle_token";
	const char *video_bridge1 = "video_bridge1";
	const char *types = "types";
	const uint32_t mask = 7;

	msg = method_call_new("SelectSources");
	if(!msg) {
		fprintf(stderr, "Fuck\n");
		return 1;
	}

	dbus_message_iter_init_append(msg, &iter);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH, &data->session);
	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &asub);
	dbus_message_iter_open_container(&asub, DBUS_TYPE_DICT_ENTRY, NULL, &esub);
	dbus_message_iter_append_basic(&esub, DBUS_TYPE_STRING, &handle_token);
	dbus_message_iter_open_container(&esub, DBUS_TYPE_VARIANT, "s", &vsub);
	dbus_message_iter_append_basic(&vsub, DBUS_TYPE_STRING, &video_bridge1);
	dbus_message_iter_close_container(&esub, &vsub);
	dbus_message_iter_close_container(&asub, &esub);
	dbus_message_iter_open_container(&asub, DBUS_TYPE_DICT_ENTRY, NULL, &esub);
	dbus_message_iter_append_basic(&esub, DBUS_TYPE_STRING, &types);
	dbus_message_iter_open_container(&esub, DBUS_TYPE_VARIANT, "u", &vsub);
	dbus_message_iter_append_basic(&vsub, DBUS_TYPE_UINT32, &mask);
	dbus_message_iter_close_container(&esub, &vsub);
	dbus_message_iter_close_container(&asub, &esub);
	dbus_message_iter_close_container(&iter, &asub);

	dbus_error_init(&error);
	reply = dbus_connection_send_with_reply_and_block(data->con, msg, -1, &error);
	if(!reply) {
		fprintf(stderr, "%s\n", error.message);
		return 1;
	}

	dbus_message_get_args(reply, &error, DBUS_TYPE_OBJECT_PATH, &request, DBUS_TYPE_INVALID);
	wait_for_response(data->con, request);

	return 0;
}

static char *get_session(DBusMessage *msg) {
	uint32_t res;
	int count, i;
	DBusMessageIter iter, asub;

	dbus_message_iter_init(msg, &iter);
	dbus_message_iter_get_basic(&iter, &res);

	if(res != 0)
		return NULL;

	dbus_message_iter_next(&iter);
	count = dbus_message_iter_get_element_count(&iter);
	dbus_message_iter_recurse(&iter, &asub);
	for(i = 0; i < count; i++) {
		char *key;
		DBusMessageIter esub;

		dbus_message_iter_recurse(&asub, &esub);
		dbus_message_iter_get_basic(&esub, &key);

		if(!strcmp(key, "session_handle")) {
			DBusMessageIter vsub;
			char *value;

			dbus_message_iter_next(&esub);
			dbus_message_iter_recurse(&esub, &vsub);
			dbus_message_iter_get_basic(&vsub, &value);
			return strdup(value);
		}
	}

	return NULL;
}

uint8_t dhandler_create_session(struct ddata *data) {
	DBusError error;
	DBusMessage *msg, *reply, *response;
	DBusMessageIter iter, asub, esub, vsub;
	char *request;

	const char *handle_token = "handle_token";
	const char *video_bridge0 = "video_bridge0";
	const char *session_handle_token = "session_handle_token";
	const char *video_bridge_session = "video_bridge_session";

	msg = method_call_new("CreateSession");
	if(!msg) {
		printf("Fuck\n");
		return 1;
	}

	dbus_message_iter_init_append(msg, &iter);
	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &asub);
	dbus_message_iter_open_container(&asub, DBUS_TYPE_DICT_ENTRY, NULL, &esub);
	dbus_message_iter_append_basic(&esub, DBUS_TYPE_STRING, &handle_token);
	dbus_message_iter_open_container(&esub, DBUS_TYPE_VARIANT, "s", &vsub);
	dbus_message_iter_append_basic(&vsub, DBUS_TYPE_STRING, &video_bridge0);
	dbus_message_iter_close_container(&esub, &vsub);
	dbus_message_iter_close_container(&asub, &esub);
	dbus_message_iter_open_container(&asub, DBUS_TYPE_DICT_ENTRY, NULL, &esub);
	dbus_message_iter_append_basic(&esub, DBUS_TYPE_STRING, &session_handle_token);
	dbus_message_iter_open_container(&esub, DBUS_TYPE_VARIANT, "s", &vsub);
	dbus_message_iter_append_basic(&vsub, DBUS_TYPE_STRING, &video_bridge_session);
	dbus_message_iter_close_container(&esub, &vsub);
	dbus_message_iter_close_container(&asub, &esub);
	dbus_message_iter_close_container(&iter, &asub);

	dbus_error_init(&error);
	reply = dbus_connection_send_with_reply_and_block(data->con, msg, -1, &error);
	if(!reply) {
		fprintf(stderr, "%s\n", error.message);
		return 1;
	}

	dbus_message_get_args(reply, &error, DBUS_TYPE_OBJECT_PATH, &request, DBUS_TYPE_INVALID);
	response = wait_for_response(data->con, request);

	data->session = get_session(response);

	return 0;
}

uint8_t dhandler_routine(struct ddata *data) {
	return !dbus_connection_read_write_dispatch(data->con, 1000);
}

void dhandler_destroy(struct ddata *data) {
	dbus_connection_unref(data->con);
	free(data->session);
	free(data);
}