/*
 * Copyright (c) 2011 Bas Westerbaan <bas@westerbaan.name>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/**
 * @file audio_format_gst.c
 * @brief Experimental wrapper for the GStreamer decodebin
 *
 * This plugin sets up a simple GStreamer pipeline:
 *
 *   fdsrc => decodebin => audioconvert => appsink
 *
 * We feed fdsrc fileno(audio_file->fp) and pull buffers from appsink.
 */

/*
 * TODO
 *  - Reuse pipeline.  This will decrease the inter-track gap.
 */

#include "stdinc.h"

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include "audio_file.h"
#include "audio_format.h"
#include "audio_output.h"

/**
 * @brief Private GST data stored in the audio file structure.
 */
struct gst_drv_data {
	/*
	 * @brief The main gstreamer pipeline
	 */
	GstElement* pipeline;

	/*
	 * @brief The business end of the pipeline
	 */
	GstElement* appsink;

	/*
	 * @brief The current buffer
	 */
	GstBuffer* gbuf;

	/*
	 * @brief The offset in the current buffer
	 */
	size_t gbuf_o;
};

/**
 * @brief Pulls a buffer from the pipeline and store it in the drv_data
 *
 * This function will block when no data is available and unblock on
 * a end of stream or when the pipeline is set to GST_STATE_NULL
 */
static int
gst_pull_buffer(struct audio_file *fd, struct gst_drv_data *data)
{
	GstAppSink* sink = GST_APP_SINK(data->appsink);
	GstBuffer* gbuf = NULL;
	GstStructure* capss = NULL;
	gint channels;
	gint srate;

	g_assert(!data->gbuf);

	/* try to pull a buffer */
	gbuf = gst_app_sink_pull_buffer(sink);

	if (!gbuf) {
		if (gst_app_sink_is_eos(sink))
			return (-1);
		g_assert_not_reached();
	}

	g_assert(gbuf->size % sizeof(int16_t) == 0);

	/* get the format of the buffer */
	g_assert(gst_caps_get_size(gbuf->caps) == 1);
	capss = gst_caps_get_structure(gbuf->caps, 0);
	g_assert(capss);

	/* set the format in fd */
	gst_structure_get_int(capss, "channels", &channels);
	fd->channels = channels;
	gst_structure_get_int(capss, "rate", &srate);
	fd->srate = srate;

	/* Store in drv_data */
	data->gbuf = gbuf;
	data->gbuf_o = 0;

	/* Check for a timestamp */
	if (gbuf->timestamp != GST_CLOCK_TIME_NONE)
		fd->time_cur = gbuf->timestamp / 1000000000;

	return (0);
}

/**
 * @brief Called when there is an error on the pipeline
 */
static gboolean
on_bus_error(GstBus* bus, GstMessage* msg, void* user_data)
{
	struct audio_file* fd = user_data;
	struct gst_drv_data *data = fd->drv_data;

	g_assert(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR);

	/* Stop the pipeline */
	gst_element_set_state(data->pipeline, GST_STATE_NULL);

	return FALSE;
}

/**
 * @brief Called when there is a tag message on the pipeline
 */
static gboolean
on_bus_tag(GstBus* bus, GstMessage* msg, void* user_data)
{
	struct audio_file* fd = user_data;
	GstTagList* tags;
	char* artist = NULL;
	char* title = NULL;
#ifdef BUILD_SCROBBLER
	char* album = NULL;
#endif /* BUILD_SCROBBLER */
	guint64 duration;

	g_assert(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_TAG);

	gst_message_parse_tag(msg, &tags);

	/* Set artist, title (and album) from tags --- if available  */
	if (gst_tag_list_get_string(tags, GST_TAG_ARTIST, &artist)) {
		g_free(fd->artist);
		fd->artist = artist;
	}
	if (gst_tag_list_get_string(tags, GST_TAG_TITLE, &title)) {
		g_free(fd->title);
		fd->title = title;
	}
#ifdef BUILD_SCROBBLER
	if (gst_tag_list_get_string(tags, GST_TAG_ALBUM, &album)) {
		g_free(fd->album);
		fd->album = album;
	}
#endif /* BUILD_SCROBBLER */

	/* Set duration from tags, if not set already */
	if (gst_tag_list_get_uint64(tags, GST_TAG_DURATION, &duration)) {
		if (fd->time_len == 0)
			fd->time_len = duration / 1000000000;
	}

	gst_tag_list_free(tags);

	return TRUE;
}

/**
 * @brief Called when there is a duration message on the pipeline
 */
static gboolean
on_bus_duration(GstBus* bus, GstMessage* msg, void* user_data)
{
	struct audio_file* fd = user_data;
	struct gst_drv_data *data = fd->drv_data;
	gint64 duration;
	GstFormat format;

	g_assert(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_DURATION);

	gst_message_parse_duration(msg, &format, &duration);

	/* The duration might be in a format we don't care about
	 * (bytes); or it might be GST_CLOCK_TIME_NONE which
	 * means we've got to query it for ourselves */
	if ((GstClockTime)duration == GST_CLOCK_TIME_NONE ||
			format != GST_FORMAT_TIME) {
		gboolean successful;
		format = GST_FORMAT_TIME;
		successful = gst_element_query_duration(data->pipeline,
					&format, &duration);
		if (!successful || format != GST_FORMAT_TIME ||
				(GstClockTime)duration == GST_CLOCK_TIME_NONE)
			return TRUE;
	}

	fd->time_len = duration / 1000000000;

	return TRUE;
}

/*
 * Public API
 */

int
gst_open(struct audio_file *fd, const char *ext)
{
	struct gst_drv_data *data = NULL;
	GstElement* pipeline = NULL;
	GstElement* appsink = NULL;
	GString* pipeline_desc;
	GError* err = NULL;
	GstBus* bus = NULL;

	/* Set up the pipeline */
	pipeline_desc = g_string_sized_new(128);
	g_string_printf(pipeline_desc,
			"fdsrc fd=%d ! "        /* read from the fd */
			"decodebin ! "          /* decode it */
			"audioconvert ! "       /* and  convert to: */
			"audio/x-raw-int,"      /* raw pcm */
			"width=16,depth=16,"    /* 16 bit amplitude */
			"signed=True,"          /* signed integers */
			"endianness=1234 ! "    /* in the right endianness */
			"appsink name=sink sync=False",
				fileno(fd->fp));
	pipeline = gst_parse_launch(pipeline_desc->str, &err);
	g_string_free(pipeline_desc, TRUE);
	if (err) {
		g_warning("gst_parse_launch: %s", err->message);
		goto error;
	}

	/* Get and configure the appsink */
	appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
	if (!appsink) {
		g_warning("gst_bin_get_by_name failed");
		goto error;
	}

	/* If we don't limit the amount of buffers, appsink
	 * will probably put the whole raw audiofile in memory
	 * as queued buffers. */
	gst_app_sink_set_max_buffers(GST_APP_SINK(appsink), 8);

	/* Save the state */
	data = g_slice_new(struct gst_drv_data);
	fd->drv_data = data;
	data->pipeline = pipeline;
	data->appsink = appsink;
	data->gbuf = NULL;
	data->gbuf_o = 0;

	/* Set up a message watch on the bus of the pipeline
	 * (for tags, duration etc) */
	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_signal_watch(bus);
	g_signal_connect(bus, "message::duration",
			G_CALLBACK(on_bus_duration), fd);
	g_signal_connect(bus, "message::error",
			G_CALLBACK(on_bus_error), fd);
	g_signal_connect(bus, "message::tag",
			G_CALLBACK(on_bus_tag), fd);
	gst_object_unref(GST_OBJECT(bus));

	/* Start the pipeline! */
	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	/* Pull the first buffer
	 * NOTE we assume that on_bus_tag will be called before gst_pull_buffer
	 *      returns. */
	if (gst_pull_buffer(fd, data) == -1)
		goto error;

	return (0);
error:
	if (data) {
		/* If we've already set up data, gst_close will do */
		gst_close(fd);
		return (-1);
	}
	if (err)
		g_error_free(err);
	if (appsink)
		gst_object_unref(GST_OBJECT(appsink));
	if (pipeline) {
		gst_object_unref(GST_OBJECT(pipeline));
	}
	if (data)
		g_slice_free(struct gst_drv_data, data);
	return (-1);
}

void
gst_close(struct audio_file *fd)
{
	struct gst_drv_data *data = fd->drv_data;

	if (data->pipeline) {
		/* Check whether the state is already GST_STATE_NULL */
		GstState state = GST_STATE_VOID_PENDING;
		GstState pending = GST_STATE_VOID_PENDING;

		gst_element_get_state(data->pipeline, &state, &pending,
					GST_CLOCK_TIME_NONE);

		if (state != GST_STATE_NULL) {
			/* stop the pipeline */
			gst_element_set_state(data->pipeline, GST_STATE_NULL);
		}

		/* free it */
		gst_object_unref(GST_OBJECT(data->pipeline));
		data->pipeline = NULL;
	}
	if (data->appsink) {
		gst_object_unref(GST_OBJECT(data->appsink));
		data->appsink = NULL;
	}
	if (data->gbuf) {
		gst_buffer_unref(data->gbuf);
		data->gbuf = NULL;
	}

	g_slice_free(struct gst_drv_data, data);
}

size_t
gst_read(struct audio_file *fd, int16_t *buf, size_t len)
{
	struct gst_drv_data *data = fd->drv_data;
	size_t written = 0;

	do {
		size_t to_copy = 0;

		if (!data->gbuf) {
			if (gst_pull_buffer(fd, data) == -1)
				return 0;
		}

		to_copy = MIN(len - written, data->gbuf->size / sizeof(int16_t)
						- data->gbuf_o);

		memcpy(&(buf[written]),
		       &(((int16_t*)data->gbuf->data)[data->gbuf_o]),
		       to_copy * sizeof(int16_t));
		written += to_copy;
		data->gbuf_o += to_copy;

		/* Is the gbuf depleted? */
		if (data->gbuf->size / sizeof(int16_t)
					== data->gbuf_o) {
			gst_buffer_unref(data->gbuf);
			data->gbuf = NULL;
		}
	} while (written < len);

	return written;
}

void
gst_seek(struct audio_file *fd, int len, int rel)
{
	struct gst_drv_data *data = fd->drv_data;

	if (rel) {
		/* Relative seek */
		len += fd->time_cur;
	}

	/* Calculate the new relative position */
	len = CLAMP(len, 0, (int)fd->time_len);

	gst_element_seek_simple(data->pipeline,
				GST_FORMAT_TIME,
				GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
				((gint64)len) * 1000000000);
}
