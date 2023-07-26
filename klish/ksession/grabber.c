/** @file kexec.c
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <faux/list.h>
#include <faux/buf.h>
#include <faux/eloop.h>


typedef struct {
	int fd_in;
	int fd_out;
	faux_buf_t *buf;
} grabber_stream_t;

typedef enum {
	IN,
	OUT
} direction_t;


grabber_stream_t *grabber_stream_new(int fd_in, int fd_out)
{
	grabber_stream_t *stream = NULL;

	stream = malloc(sizeof(*stream));
	assert(stream);

	stream->fd_in = fd_in;
	stream->fd_out = fd_out;
	stream->buf = faux_buf_new(FAUX_BUF_UNLIMITED);

	return stream;
}


void grabber_stream_free(grabber_stream_t *stream)
{
	if (!stream)
		return;

	faux_buf_free(stream->buf);
	faux_free(stream);
}


static bool_t grabber_stop_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	// Happy compiler
	eloop = eloop;
	type = type;
	associated_data = associated_data;
	user_data = user_data;

	return BOOL_FALSE; // Stop Event Loop
}


static bool_t grabber_fd_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	faux_eloop_info_fd_t *info = (faux_eloop_info_fd_t *)associated_data;
	faux_list_t *stream_list = (faux_list_t *)user_data;
	grabber_stream_t *stream = NULL;
	grabber_stream_t *cur_stream = NULL;
	direction_t direction = IN;
	faux_list_node_t *iter = NULL;
	type = type; // Happy compiler

	iter = faux_list_head(stream_list);
	while((cur_stream = (grabber_stream_t *)faux_list_each(&iter))) {
		if (info->fd == cur_stream->fd_in) {
			direction = IN;
			stream = cur_stream;
			break;
		}
		if (info->fd == cur_stream->fd_out) {
			direction = OUT;
			stream = cur_stream;
			break;
		}
	}
	if (!stream)
		return BOOL_FALSE; // Problems

	// Read data (must locates before write data code)
	if (info->revents & POLLIN) {
		ssize_t r = 0;
		do {
			ssize_t len = 0;
			void *data = NULL;
			size_t readed = 0;
			len = faux_buf_dwrite_lock_easy(stream->buf, &data);
			if (len <= 0)
				break;
			r = read(stream->fd_in, data, len);
			readed = r < 0 ? 0 : r;
			faux_buf_dwrite_unlock_easy(stream->buf, readed);
		} while (r > 0);
	}

	// Write data
	if (faux_buf_len(stream->buf) > 0) {
		ssize_t r = 0;
		ssize_t sent = 0;
		ssize_t len = 0;
		do {
			void *data = NULL;
			len = faux_buf_dread_lock_easy(stream->buf, &data);
			if (len <= 0)
				break;
			r = write(stream->fd_out, data, len);
			sent = r < 0 ? 0 : r;
			faux_buf_dread_unlock_easy(stream->buf, sent);
		} while (sent == len);
	}
	// Check if there additional data to send
	if (faux_buf_len(stream->buf) == 0)
		faux_eloop_exclude_fd_event(eloop, stream->fd_out, POLLOUT);
	else
		faux_eloop_include_fd_event(eloop, stream->fd_out, POLLOUT);

	// EOF
	if (info->revents & POLLHUP) {
		faux_eloop_del_fd(eloop, info->fd);
		if (IN == direction)
			stream->fd_in = -1;
		else
			stream->fd_out = -1;
	}

	iter = faux_list_head(stream_list);
	while((cur_stream = (grabber_stream_t *)faux_list_each(&iter))) {
		if (cur_stream->fd_in != -1)
			return BOOL_TRUE;
	}

	return BOOL_FALSE;
}


void grabber(int fds[][2])
{
	faux_eloop_t *eloop = NULL;
	int i = 0;
	faux_list_t *stream_list = NULL;

	stream_list = faux_list_new(FAUX_LIST_UNSORTED, FAUX_LIST_NONUNIQUE,
		NULL, NULL, (void (*)(void *))grabber_stream_free);

	eloop = faux_eloop_new(NULL);
	faux_eloop_add_signal(eloop, SIGINT, grabber_stop_ev, NULL);
	faux_eloop_add_signal(eloop, SIGTERM, grabber_stop_ev, NULL);
	faux_eloop_add_signal(eloop, SIGQUIT, grabber_stop_ev, NULL);
	faux_eloop_add_signal(eloop, SIGPIPE, grabber_stop_ev, NULL);

	i = 0;
	while (fds[i][0] != -1) {
		grabber_stream_t *stream = grabber_stream_new(fds[i][0], fds[i][1]);
		int fflags = 0;

		faux_list_add(stream_list, stream);

		fflags = fcntl(stream->fd_in, F_GETFL);
		fcntl(stream->fd_in, F_SETFL, fflags | O_NONBLOCK);
		fflags = fcntl(stream->fd_out, F_GETFL);
		fcntl(stream->fd_out, F_SETFL, fflags | O_NONBLOCK);

		faux_eloop_add_fd(eloop, stream->fd_in, POLLIN, grabber_fd_ev, stream_list);
		faux_eloop_add_fd(eloop, stream->fd_out, 0, grabber_fd_ev, stream_list);

		i++;
	}

	faux_eloop_loop(eloop);
	faux_eloop_free(eloop);

	faux_list_free(stream_list);
}
