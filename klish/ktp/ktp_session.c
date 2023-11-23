#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <syslog.h>

#include <faux/str.h>
#include <klish/ktp_session.h>


typedef struct cb_s {
	void *fn;
	void *udata;
} cb_t;


struct ktp_session_s {
	ktp_session_state_e state;
	faux_async_t *async;
	faux_hdr_t *hdr; // Service var: engine will receive header and then msg
	bool_t done;
	faux_eloop_t *eloop; // External eloop object
	cb_t cb[KTP_SESSION_CB_MAX];
	faux_error_t *error; // Internal
	bool_t request_done;
	int cmd_retcode; // Internal
	bool_t cmd_retcode_available;
	ktp_status_e cmd_features;
	bool_t cmd_features_available;
	bool_t stop_on_answer; // Stop the loop when answer is received (for non-interactive mode)
	bool_t stdout_need_newline; // Does stdout has final line feed. If no then newline is needed
	bool_t stderr_need_newline; // Does stderr has final line feed. If no then newline is needed
	int last_stream; // Last active stream: stdout or stderr
};


static bool_t server_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data);
static bool_t ktp_session_read_cb(faux_async_t *async,
	faux_buf_t *buf, size_t len, void *user_data);


ktp_session_t *ktp_session_new(int sock, faux_eloop_t *eloop)
{
	ktp_session_t *ktp = NULL;

	if (sock < 0)
		return NULL;

	if (!eloop)
		return NULL;

	ktp = faux_zmalloc(sizeof(*ktp));
	assert(ktp);
	if (!ktp)
		return NULL;

	// Init
	ktp->state = KTP_SESSION_STATE_IDLE;
	ktp->done = BOOL_FALSE;
	ktp->eloop = eloop;
	ktp->stop_on_answer = BOOL_TRUE; // Non-interactive by default
	ktp->error = NULL;
	ktp->cmd_retcode = 0;
	ktp->cmd_retcode_available = BOOL_FALSE;
	ktp->request_done = BOOL_FALSE;
	ktp->cmd_features = KTP_STATUS_NONE;
	ktp->cmd_features_available = BOOL_FALSE;
	ktp->stdout_need_newline = BOOL_FALSE;
	ktp->stderr_need_newline = BOOL_FALSE;
	ktp->last_stream = STDOUT_FILENO;

	// Async object
	ktp->async = faux_async_new(sock);
	assert(ktp->async);
	// Receive message header first
	faux_async_set_read_limits(ktp->async,
		sizeof(faux_hdr_t), sizeof(faux_hdr_t));
	faux_async_set_read_cb(ktp->async, ktp_session_read_cb, ktp);
	ktp->hdr = NULL;
	faux_async_set_stall_cb(ktp->async, ktp_stall_cb, ktp->eloop);

	// Event loop handlers
	faux_eloop_add_fd(ktp->eloop, ktp_session_fd(ktp), POLLIN,
		server_ev, ktp);

	// Callbacks
	// Callbacks ktp->cb are zeroed by faux_zmalloc()

	return ktp;
}


void ktp_session_free(ktp_session_t *ktp)
{
	if (!ktp)
		return;

	// Remove socket from eloop but don't free eloop because it's external
	faux_eloop_del_fd(ktp->eloop, ktp_session_fd(ktp));
	faux_free(ktp->hdr);
	close(ktp_session_fd(ktp));
	faux_async_free(ktp->async);
	faux_free(ktp);
}


faux_eloop_t *ktp_session_eloop(const ktp_session_t *ktp)
{
	assert(ktp);
	if (!ktp)
		return NULL;

	return ktp->eloop;
}


bool_t ktp_session_done(const ktp_session_t *ktp)
{
	assert(ktp);
	if (!ktp)
		return BOOL_TRUE; // Done flag

	return ktp->done;
}


bool_t ktp_session_set_done(ktp_session_t *ktp, bool_t done)
{
	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;

	ktp->done = done;

	return BOOL_TRUE;
}


bool_t ktp_session_stop_on_answer(const ktp_session_t *ktp)
{
	assert(ktp);
	if (!ktp)
		return BOOL_TRUE; // Default

	return ktp->stop_on_answer;
}


bool_t ktp_session_set_stop_on_answer(ktp_session_t *ktp, bool_t stop_on_answer)
{
	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;

	ktp->stop_on_answer = stop_on_answer;

	return BOOL_TRUE;
}


ktp_session_state_e ktp_session_state(const ktp_session_t *ktp)
{
	assert(ktp);
	if (!ktp)
		return KTP_SESSION_STATE_ERROR;

	return ktp->state;
}


ktp_status_e ktp_session_cmd_features(const ktp_session_t *ktp)
{
	assert(ktp);
	if (!ktp)
		return KTP_STATUS_NONE;

	return ktp->cmd_features;
}


faux_error_t *ktp_session_error(const ktp_session_t *ktp)
{
	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;

	return ktp->error;
}


bool_t ktp_session_set_cb(ktp_session_t *ktp, ktp_session_cb_e cb_id,
	void *fn, void *udata)
{
	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;
	if (cb_id >= KTP_SESSION_CB_MAX)
		return BOOL_FALSE;

	ktp->cb[cb_id].fn = fn;
	ktp->cb[cb_id].udata = udata;

	return BOOL_TRUE;
}


bool_t ktp_session_connected(ktp_session_t *ktp)
{
	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;
	if (KTP_SESSION_STATE_DISCONNECTED == ktp->state)
		return BOOL_FALSE;

	return BOOL_TRUE;
}


int ktp_session_fd(const ktp_session_t *ktp)
{
	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;

	return faux_async_fd(ktp->async);
}


faux_async_t *ktp_session_async(const ktp_session_t *ktp)
{
	assert(ktp);
	if (!ktp)
		return NULL;

	return ktp->async;
}


static bool_t server_ev(faux_eloop_t *eloop, faux_eloop_type_e type,
	void *associated_data, void *user_data)
{
	faux_eloop_info_fd_t *info = (faux_eloop_info_fd_t *)associated_data;
	ktp_session_t *ktp = (ktp_session_t *)user_data;

	assert(ktp);

	// Write data
	if (info->revents & POLLOUT) {
		ssize_t len = 0;
		faux_eloop_exclude_fd_event(eloop, info->fd, POLLOUT);
		if ((len = faux_async_out_easy(ktp->async)) < 0) {
			// Someting went wrong
			faux_eloop_del_fd(eloop, info->fd);
			syslog(LOG_ERR, "Problem with async output");
			return BOOL_FALSE; // Stop event loop
		}
		// Execute external callback
		if (ktp->cb[KTP_SESSION_CB_STDIN].fn)
			((ktp_session_stdin_cb_fn)
				ktp->cb[KTP_SESSION_CB_STDIN].fn)(
				ktp, len, ktp->cb[KTP_SESSION_CB_STDIN].udata);
	}

	// Read data
	if (info->revents & POLLIN) {
		if (faux_async_in_easy(ktp->async) < 0) {
			// Someting went wrong
			faux_eloop_del_fd(eloop, info->fd);
			syslog(LOG_ERR, "Problem with async input");
			return BOOL_FALSE; // Stop event loop
		}
	}

	// EOF
	if (info->revents & POLLHUP) {
		faux_eloop_del_fd(eloop, info->fd);
		syslog(LOG_DEBUG, "Close connection %d", info->fd);
		return BOOL_FALSE; // Stop event loop
	}

	type = type; // Happy compiler

	if (ktp->request_done && ktp->stop_on_answer)
		return BOOL_FALSE; // Stop event loop on receiving answer
	if (ktp->done)
		return BOOL_FALSE; // Stop event loop on done flag (exit)

	return BOOL_TRUE;
}


static bool_t ktp_session_process_stdout(ktp_session_t *ktp, const faux_msg_t *msg)
{
	char *line = NULL;
	unsigned int len = 0;

	assert(ktp);
	assert(msg);

	if (!ktp->cb[KTP_SESSION_CB_STDOUT].fn)
		return BOOL_TRUE; // Just ignore stdout. It's not a bug

	if (!faux_msg_get_param_by_type(msg, KTP_PARAM_LINE, (void **)&line, &len))
		return BOOL_TRUE; // It's strange but not a bug

	if (len > 0) {
		if (line[len - 1] == '\n')
			ktp->stdout_need_newline = BOOL_FALSE;
		else
			ktp->stdout_need_newline = BOOL_TRUE;
		ktp->last_stream = STDOUT_FILENO;
	}

	return ((ktp_session_stdout_cb_fn)ktp->cb[KTP_SESSION_CB_STDOUT].fn)(
		ktp, line, len, ktp->cb[KTP_SESSION_CB_STDOUT].udata);
}


static bool_t ktp_session_process_stderr(ktp_session_t *ktp, const faux_msg_t *msg)
{
	char *line = NULL;
	unsigned int len = 0;

	assert(ktp);
	assert(msg);

	if (!ktp->cb[KTP_SESSION_CB_STDERR].fn)
		return BOOL_TRUE; // Just ignore message. It's not a bug

	if (!faux_msg_get_param_by_type(msg, KTP_PARAM_LINE,
			(void **)&line, &len))
		return BOOL_TRUE; // It's strange but not a bug

	if (len > 0) {
		if (line[len - 1] == '\n')
			ktp->stderr_need_newline = BOOL_FALSE;
		else
			ktp->stderr_need_newline = BOOL_TRUE;
		ktp->last_stream = STDERR_FILENO;
	}

	return ((ktp_session_stdout_cb_fn)ktp->cb[KTP_SESSION_CB_STDERR].fn)(
		ktp, line, len, ktp->cb[KTP_SESSION_CB_STDERR].udata);
}


static bool_t ktp_session_process_auth_ack(ktp_session_t *ktp, const faux_msg_t *msg)
{
	uint8_t *retcode8bit = NULL;
	ktp_status_e status = KTP_STATUS_NONE;
	char *error_str = NULL;

	assert(ktp);
	assert(msg);

	status = faux_msg_get_status(msg);

	if (faux_msg_get_param_by_type(msg, KTP_PARAM_RETCODE,
		(void **)&retcode8bit, NULL))
		ktp->cmd_retcode = (int)(*retcode8bit);
	error_str = faux_msg_get_str_param_by_type(msg, KTP_PARAM_ERROR);
	if (error_str) {
		faux_error_add(ktp->error, error_str);
		faux_str_free(error_str);
	}

	ktp->cmd_retcode_available = BOOL_TRUE; // Answer from server was received
	ktp->request_done = BOOL_TRUE;
	ktp->state = KTP_SESSION_STATE_IDLE;
	// Get exit flag from message
	if (KTP_STATUS_IS_EXIT(status))
		ktp->done = BOOL_TRUE;

	// Execute external callback
	if (ktp->cb[KTP_SESSION_CB_AUTH_ACK].fn)
		((ktp_session_event_cb_fn)
			ktp->cb[KTP_SESSION_CB_AUTH_ACK].fn)(
			ktp, msg,
			ktp->cb[KTP_SESSION_CB_AUTH_ACK].udata);

	return BOOL_TRUE;
}


static bool_t ktp_session_process_cmd_ack(ktp_session_t *ktp, const faux_msg_t *msg)
{
	uint8_t *retcode8bit = NULL;
	ktp_status_e status = KTP_STATUS_NONE;
	char *error_str = NULL;

	assert(ktp);
	assert(msg);

	status = faux_msg_get_status(msg);
	// cmd_ack with flag 'incompleted'
	if (KTP_STATUS_IS_INCOMPLETED(status)) {
		// Only first 'incompleted' cmd ack sets cmd features
		if (!ktp->cmd_features_available) {
			ktp->cmd_features_available = BOOL_TRUE;
			ktp->cmd_features = status &
				(KTP_STATUS_INTERACTIVE | KTP_STATUS_NEED_STDIN);
		}
		// Execute external callback
		if (ktp->cb[KTP_SESSION_CB_CMD_ACK_INCOMPLETED].fn)
			((ktp_session_event_cb_fn)
				ktp->cb[KTP_SESSION_CB_CMD_ACK_INCOMPLETED].fn)(
				ktp, msg,
				ktp->cb[KTP_SESSION_CB_CMD_ACK_INCOMPLETED].udata);
		return BOOL_TRUE;
	}

	// If retcode param is not present it means all is ok (retcode = 0).
	// Server will not send retcode in a case of empty command. Empty command
	// doesn't execute real actions
	if (faux_msg_get_param_by_type(msg, KTP_PARAM_RETCODE,
		(void **)&retcode8bit, NULL)) {
		ktp->cmd_retcode = (int)(*retcode8bit);
	} else {
		if (KTP_STATUS_IS_ERROR(status))
			ktp->cmd_retcode = -1;
		else
			ktp->cmd_retcode = 0;
	}
	error_str = faux_msg_get_str_param_by_type(msg, KTP_PARAM_ERROR);
	if (error_str) {
		faux_error_add(ktp->error, error_str);
		faux_str_free(error_str);
	}

	ktp->cmd_retcode_available = BOOL_TRUE; // Answer from server was received
	ktp->request_done = BOOL_TRUE;
	ktp->state = KTP_SESSION_STATE_IDLE;
	// Get exit flag from message
	if (KTP_STATUS_IS_EXIT(status))
		ktp_session_set_done(ktp, BOOL_TRUE);

	// Execute external callback
	if (ktp->cb[KTP_SESSION_CB_CMD_ACK].fn)
		((ktp_session_event_cb_fn)
			ktp->cb[KTP_SESSION_CB_CMD_ACK].fn)(
			ktp, msg,
			ktp->cb[KTP_SESSION_CB_CMD_ACK].udata);

	return BOOL_TRUE;
}


static bool_t ktp_session_process_completion_ack(ktp_session_t *ktp, const faux_msg_t *msg)
{
	assert(ktp);
	assert(msg);

	ktp->request_done = BOOL_TRUE;
	ktp->state = KTP_SESSION_STATE_IDLE;
	// Get exit flag from message
	if (KTP_STATUS_IS_EXIT(faux_msg_get_status(msg)))
		ktp->done = BOOL_TRUE;

	// Execute external callback
	if (ktp->cb[KTP_SESSION_CB_COMPLETION_ACK].fn)
		((ktp_session_event_cb_fn)
			ktp->cb[KTP_SESSION_CB_COMPLETION_ACK].fn)(
			ktp, msg,
			ktp->cb[KTP_SESSION_CB_COMPLETION_ACK].udata);

	return BOOL_TRUE;
}


static bool_t ktp_session_process_help_ack(ktp_session_t *ktp, const faux_msg_t *msg)
{
	assert(ktp);
	assert(msg);

	ktp->request_done = BOOL_TRUE;
	ktp->state = KTP_SESSION_STATE_IDLE;
	// Get exit flag from message
	if (KTP_STATUS_IS_EXIT(faux_msg_get_status(msg)))
		ktp->done = BOOL_TRUE;

	// Execute external callback
	if (ktp->cb[KTP_SESSION_CB_HELP_ACK].fn)
		((ktp_session_event_cb_fn)
			ktp->cb[KTP_SESSION_CB_HELP_ACK].fn)(
			ktp, msg,
			ktp->cb[KTP_SESSION_CB_HELP_ACK].udata);

	return BOOL_TRUE;
}


/*
static bool_t ktp_session_process_exit(ktp_session_t *ktp, const faux_msg_t *msg)
{
	assert(ktp);
	assert(msg);

	ktp_session_set_done(ktp, BOOL_TRUE);
	// Execute external callback
	if (ktp->cb[KTP_SESSION_CB_EXIT].fn)
		((ktp_session_event_cb_fn)
			ktp->cb[KTP_SESSION_CB_EXIT].fn)(
			ktp, msg,
			ktp->cb[KTP_SESSION_CB_EXIT].udata);

	return BOOL_TRUE;
}
*/

static bool_t ktp_session_dispatch(ktp_session_t *ktp, faux_msg_t *msg)
{
	uint16_t cmd = 0;
	bool_t rc = BOOL_TRUE;

	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;
	assert(msg);
	if (!msg)
		return BOOL_FALSE;

	cmd = faux_msg_get_cmd(msg);
	switch (cmd) {
	case KTP_AUTH_ACK:
		if (ktp->state != KTP_SESSION_STATE_UNAUTHORIZED) {
			syslog(LOG_WARNING, "Unexpected KTP_AUTH_ACK was received\n");
			break;
		}
		rc = ktp_session_process_auth_ack(ktp, msg);
		break;
	case KTP_CMD_ACK:
		if (ktp->state != KTP_SESSION_STATE_WAIT_FOR_CMD) {
			syslog(LOG_WARNING, "Unexpected KTP_CMD_ACK was received\n");
			break;
		}
		rc = ktp_session_process_cmd_ack(ktp, msg);
		break;
	case KTP_COMPLETION_ACK:
		if (ktp->state != KTP_SESSION_STATE_WAIT_FOR_COMPLETION) {
			syslog(LOG_WARNING, "Unexpected KTP_COMPLETION_ACK was received\n");
			break;
		}
		rc = ktp_session_process_completion_ack(ktp, msg);
		break;
	case KTP_HELP_ACK:
		if (ktp->state != KTP_SESSION_STATE_WAIT_FOR_HELP) {
			syslog(LOG_WARNING, "Unexpected KTP_HELP_ACK was received\n");
			break;
		}
		rc = ktp_session_process_help_ack(ktp, msg);
		break;
	case KTP_STDOUT:
		if (ktp->state != KTP_SESSION_STATE_WAIT_FOR_CMD) {
			syslog(LOG_WARNING, "Unexpected KTP_STDOUT was received\n");
			break;
		}
		rc = ktp_session_process_stdout(ktp, msg);
		break;
	case KTP_STDERR:
		if (ktp->state != KTP_SESSION_STATE_WAIT_FOR_CMD) {
			syslog(LOG_WARNING, "Unexpected KTP_STDERR was received\n");
			break;
		}
		rc = ktp_session_process_stderr(ktp, msg);
		break;
	default:
		syslog(LOG_WARNING, "Unsupported command: 0x%04x\n", cmd); // Ignore
		break;
	}

	return rc;
}


static bool_t ktp_session_read_cb(faux_async_t *async,
	faux_buf_t *buf, size_t len, void *user_data)
{
	ktp_session_t *ktp = (ktp_session_t *)user_data;
	faux_msg_t *completed_msg = NULL;
	char *data = NULL;

	assert(async);
	assert(buf);
	assert(ktp);

	// Linearize buffer
	data = malloc(len);
	faux_buf_read(buf, data, len);

	// Receive header
	if (!ktp->hdr) {
		size_t whole_len = 0;
		size_t msg_wo_hdr = 0;

		ktp->hdr = (faux_hdr_t *)data;
		// Check for broken header
		if (!ktp_check_header(ktp->hdr)) {
			faux_free(ktp->hdr);
			ktp->hdr = NULL;
			return BOOL_FALSE;
		}

		whole_len = faux_hdr_len(ktp->hdr);
		// msg_wo_hdr >= 0 because ktp_check_header() validates whole_len
		msg_wo_hdr = whole_len - sizeof(faux_hdr_t);
		// Plan to receive message body
		if (msg_wo_hdr > 0) {
			faux_async_set_read_limits(async,
				msg_wo_hdr, msg_wo_hdr);
			return BOOL_TRUE;
		}
		// Here message is completed (msg body has zero length)
		completed_msg = faux_msg_deserialize_parts(ktp->hdr, NULL, 0);

	// Receive message body
	} else {
		completed_msg = faux_msg_deserialize_parts(ktp->hdr, data, len);
		faux_free(data);
	}

	// Plan to receive msg header
	faux_async_set_read_limits(ktp->async,
		sizeof(faux_hdr_t), sizeof(faux_hdr_t));
	faux_free(ktp->hdr);
	ktp->hdr = NULL; // Ready to recv new header

	// Here message is completed
#ifdef DEBUG
//	faux_msg_debug(completed_msg);
#endif
	ktp_session_dispatch(ktp, completed_msg);
	faux_msg_free(completed_msg);

	return BOOL_TRUE;
}


static bool_t ktp_session_drop_state(ktp_session_t *ktp, faux_error_t *error)
{
	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;

	ktp->error = error;
	ktp->cmd_retcode = 0;
	ktp->cmd_retcode_available = BOOL_FALSE;
	ktp->request_done = BOOL_FALSE;
	ktp->cmd_features = KTP_STATUS_NONE;
	ktp->cmd_features_available = BOOL_FALSE;
	ktp->stdout_need_newline = BOOL_FALSE;
	ktp->stderr_need_newline = BOOL_FALSE;
	ktp->last_stream = STDOUT_FILENO;

	return BOOL_TRUE;
}


static bool_t ktp_session_req(ktp_session_t *ktp, ktp_cmd_e cmd,
	const char *line, size_t line_len, faux_error_t *error,
	bool_t dry_run, bool_t drop_state)
{
	faux_msg_t *req = NULL;
	ktp_status_e status = KTP_STATUS_NONE;

	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;

	// Set dry-run flag
	if (dry_run)
		status |= KTP_STATUS_DRY_RUN;

	req = ktp_msg_preform(cmd, status);
	if (line)
		faux_msg_add_param(req, KTP_PARAM_LINE, line, line_len);
	faux_msg_send_async(req, ktp->async);
	faux_msg_free(req);

	// Prepare for loop
	if (drop_state)
		ktp_session_drop_state(ktp, error);

	return BOOL_TRUE;
}


bool_t ktp_session_cmd(ktp_session_t *ktp, const char *line,
	faux_error_t *error, bool_t dry_run)
{
	if (!line)
		return BOOL_FALSE;

	if (!ktp_session_req(ktp, KTP_CMD, line, strlen(line),
		error, dry_run, BOOL_TRUE))
		return BOOL_FALSE;
	ktp->state = KTP_SESSION_STATE_WAIT_FOR_CMD;

	return BOOL_TRUE;
}


bool_t ktp_session_auth(ktp_session_t *ktp, faux_error_t *error)
{
	faux_msg_t *req = NULL;
	ktp_status_e status = KTP_STATUS_NONE;

	assert(ktp);
	if (!ktp)
		return BOOL_FALSE;

	// This request starts session. It must send some client's environment
	// to server
	if (isatty(STDIN_FILENO))
		status |= KTP_STATUS_TTY_STDIN;
	if (isatty(STDOUT_FILENO))
		status |= KTP_STATUS_TTY_STDOUT;
	if (isatty(STDERR_FILENO))
		status |= KTP_STATUS_TTY_STDERR;

	// Send request
	req = ktp_msg_preform(KTP_AUTH, status);
	faux_msg_send_async(req, ktp->async);
	faux_msg_free(req);

	// Prepare for loop
	ktp_session_drop_state(ktp, error);

	ktp->state = KTP_SESSION_STATE_UNAUTHORIZED;

	return BOOL_TRUE;
}


bool_t ktp_session_completion(ktp_session_t *ktp, const char *line, bool_t dry_run)
{
	if (!ktp_session_req(ktp, KTP_COMPLETION, line, strlen(line),
		NULL, dry_run, BOOL_TRUE))
		return BOOL_FALSE;
	ktp->state = KTP_SESSION_STATE_WAIT_FOR_COMPLETION;

	return BOOL_TRUE;
}


bool_t ktp_session_help(ktp_session_t *ktp, const char *line)
{
	if (!ktp_session_req(ktp, KTP_HELP, line, strlen(line),
		NULL, BOOL_TRUE, BOOL_TRUE))
		return BOOL_FALSE;
	ktp->state = KTP_SESSION_STATE_WAIT_FOR_HELP;

	return BOOL_TRUE;
}


bool_t ktp_session_stdin(ktp_session_t *ktp, const char *line, size_t line_len)
{
	if (!ktp_session_req(ktp, KTP_STDIN, line, line_len,
		NULL, BOOL_TRUE, BOOL_FALSE))
		return BOOL_FALSE;

	return BOOL_TRUE;
}


bool_t ktp_session_stdin_close(ktp_session_t *ktp)
{
	if (!ktp_session_req(ktp, KTP_STDIN_CLOSE, NULL, 0,
		NULL, BOOL_TRUE, BOOL_FALSE))
		return BOOL_FALSE;

	return BOOL_TRUE;
}


bool_t ktp_session_stdout_close(ktp_session_t *ktp)
{
	if (!ktp_session_req(ktp, KTP_STDOUT_CLOSE, NULL, 0,
		NULL, BOOL_TRUE, BOOL_FALSE))
		return BOOL_FALSE;

	return BOOL_TRUE;
}


bool_t ktp_session_stderr_close(ktp_session_t *ktp)
{
	if (!ktp_session_req(ktp, KTP_STDERR_CLOSE, NULL, 0,
		NULL, BOOL_TRUE, BOOL_FALSE))
		return BOOL_FALSE;

	return BOOL_TRUE;
}


bool_t ktp_session_retcode(ktp_session_t *ktp, int *retcode)
{
	if (!ktp)
		return BOOL_FALSE;

	if (ktp->cmd_retcode_available && retcode)
		*retcode = ktp->cmd_retcode;

	return ktp->cmd_retcode_available; // Sign of server answer
}


bool_t ktp_session_stdout_need_newline(ktp_session_t *ktp)
{
	if (!ktp)
		return BOOL_FALSE;

	return ktp->stdout_need_newline;
}


bool_t ktp_session_stderr_need_newline(ktp_session_t *ktp)
{
	if (!ktp)
		return BOOL_FALSE;

	return ktp->stderr_need_newline;
}


int ktp_session_last_stream(ktp_session_t *ktp)
{
	if (!ktp)
		return BOOL_FALSE;

	return ktp->last_stream;
}
