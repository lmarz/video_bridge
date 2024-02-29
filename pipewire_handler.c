/* SPDX-License-Identifier: EUPL-1.2 */
#include <stdio.h>

#include <spa/debug/types.h>
#include <spa/param/video/format-utils.h>
#include <spa/pod/builder.h>
#include <spa/utils/result.h>

#include "pipewire_handler.h"

static void stream_param_changed(void *dat, uint32_t id, const struct spa_pod *param) {
	struct pdata *data = (struct pdata *) dat;
	struct spa_video_info video_info;
	struct spa_video_info_raw *raw;

	if(param == NULL || id != SPA_PARAM_Format)
		return;


	spa_format_parse(param, &video_info.media_type, &video_info.media_subtype);

	if(video_info.media_type != SPA_MEDIA_TYPE_video || video_info.media_subtype != SPA_MEDIA_SUBTYPE_raw) {
		fprintf(stderr, "Wrong media type: %s:%s\n", spa_debug_type_find_name(spa_type_media_type, video_info.media_type), spa_debug_type_find_name(spa_type_media_subtype, video_info.media_subtype));
		return;
	}

	spa_format_video_raw_parse(param, &video_info.info.raw);
	raw = &video_info.info.raw;

	if(raw->format != SPA_VIDEO_FORMAT_BGRx) {
		fprintf(stderr, "Wrong format: %s\n", spa_debug_type_find_name(spa_type_video_format, raw->format));
		return;
	}

	xhandler_resize(data->xdata, raw->size.width, raw->size.height);
}

static void stream_process(void *dat) {
	struct pdata *data = (struct pdata *) dat;
	struct pw_buffer *b;
	struct spa_buffer *buf;

	b = pw_stream_dequeue_buffer(data->stream);
	if(!b) {
		pw_log_warn("out of buffers: %m");
		return;
	}

	buf = b->buffer;
	if(buf->datas[0].data == NULL)
		return;

	xhandler_draw(data->xdata, buf->datas[0].data, buf->datas[0].chunk->size);

	pw_stream_queue_buffer(data->stream, b);
}

static const struct pw_stream_events stream_events = {
	PW_VERSION_STREAM_EVENTS,
	.param_changed = stream_param_changed,
	.process = stream_process
};

static void init_stream(struct pdata *data, const char *serial) {
	struct pw_properties *props;
	uint8_t buffer[1024];
	struct spa_pod_builder builder;
	const struct spa_pod *params[1];

	props = pw_properties_new(PW_KEY_TARGET_OBJECT, serial, NULL);

	data->stream = pw_stream_new(data->core, "screen-capture", props);
	pw_stream_add_listener(data->stream, &data->stream_listener, &stream_events, data);

	builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
	params[0] = spa_pod_builder_add_object(&builder,
				SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
				SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
				SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
				SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(2, SPA_VIDEO_FORMAT_RGBA, SPA_VIDEO_FORMAT_BGRx));

	pw_stream_connect(data->stream, SPA_DIRECTION_INPUT, PW_ID_ANY, PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS, params, 1);
}

static void registry_global(void *dat, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const struct spa_dict *props) {
	struct pdata *data = (struct pdata *) dat;
	const struct spa_dict_item *serial;

	(void) permissions;
	(void) type;
	(void) version;

	if(id != data->node_id)
		return;

	serial = spa_dict_lookup_item(props, PW_KEY_OBJECT_SERIAL);
	if(!serial || !serial->value) {
		fprintf(stderr, "Pipewire Node %u doesn't have serial. How?\n", id);
		return;
	}

	init_stream(data, serial->value);
}

static const struct pw_registry_events registry_events = {
	PW_VERSION_REGISTRY_EVENTS,
	.global = registry_global
};

struct pdata *phandler_init(int fd, uint32_t node_id, struct xdata *xdata) {
	struct pdata *data;

	data = calloc(sizeof(struct pdata), 1);

	data->node_id = node_id;
	data->xdata = xdata;

	data->loop = pw_thread_loop_new("pipewire thread", NULL);
	pw_thread_loop_lock(data->loop);
	pw_thread_loop_start(data->loop);
	data->context = pw_context_new(pw_thread_loop_get_loop(data->loop), NULL, 0);
	data->core = pw_context_connect_fd(data->context, fd, NULL, 0);
	data->registry = pw_core_get_registry(data->core, PW_VERSION_REGISTRY, 0);

	spa_zero(data->registry_listener);
	pw_registry_add_listener(data->registry, &data->registry_listener, &registry_events, data);

	pw_thread_loop_unlock(data->loop);

	return data;
}

void phandler_destroy(struct pdata *data) {
	pw_thread_loop_stop(data->loop);
	if(data->stream)
		pw_stream_destroy(data->stream);
	pw_proxy_destroy((struct pw_proxy *)data->registry);
	pw_core_disconnect(data->core);
	pw_context_destroy(data->context);
	pw_thread_loop_destroy(data->loop);
	pw_deinit();
	free(data);
}