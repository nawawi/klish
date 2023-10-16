/** @file ktp.h
 *
 * @brief Klish Transfer Protocol
 */

#ifndef _klish_ktp_h
#define _klish_ktp_h

#include <faux/msg.h>

#define KTP_MAGIC 0x4b545020
#define KTP_MAJOR 0x01
#define KTP_MINOR 0x00

typedef enum {
	KTP_NULL = '\0',
	KTP_STDIN = 'i',
	KTP_STDOUT = 'o',
	KTP_STDERR = 'e',
	KTP_CMD = 'c',
	KTP_CMD_ACK = 'C',
	KTP_COMPLETION = 'v',
	KTP_COMPLETION_ACK = 'V',
	KTP_HELP = 'h',
	KTP_HELP_ACK = 'H',
	KTP_NOTIFICATION = 'n',
	KTP_AUTH = 'a',
	KTP_AUTH_ACK = 'A',
	KTP_KEEPALIVE = 'k',
	KTP_STDIN_CLOSE = 'I',
	KTP_STDOUT_CLOSE = 'O',
	KTP_STDERR_CLOSE = 'E',
} ktp_cmd_e;


typedef enum {
	KTP_PARAM_NULL = '\0',
	KTP_PARAM_LINE = 'L',
	KTP_PARAM_PREFIX = 'P', // Same as line but differ by meaning
	KTP_PARAM_PROMPT = '$', // Same as line but differ by meaning
	KTP_PARAM_HOTKEY = 'H', // <key>'\0'<cmd>
	KTP_PARAM_WINCH = 'W', // <width><space><height>
	KTP_PARAM_ERROR = 'E',
	KTP_PARAM_RETCODE = 'R',
} ktp_param_e;


// Status field. Bitmap
typedef enum {
	KTP_STATUS_NONE =		(uint32_t)0x00000000,
	KTP_STATUS_ERROR =		(uint32_t)0x00000001,
	KTP_STATUS_INCOMPLETED =	(uint32_t)0x00000002,
	KTP_STATUS_TTY_STDIN =		(uint32_t)0x00000100, // Client's stdin is tty
	KTP_STATUS_TTY_STDOUT =		(uint32_t)0x00000200, // Client's stdout is tty
	KTP_STATUS_TTY_STDERR =		(uint32_t)0x00000400, // Client's stderr is tty
	KTP_STATUS_NEED_STDIN =		(uint32_t)0x00001000, // Server's cmd need stdin
	KTP_STATUS_INTERACTIVE =	(uint32_t)0x00002000, // Server's stdout is for tty
	KTP_STATUS_DRY_RUN =		(uint32_t)0x00010000,
	KTP_STATUS_EXIT =		(uint32_t)0x80000000,
} ktp_status_e;

#define KTP_STATUS_IS_ERROR(status) (status & KTP_STATUS_ERROR)
#define KTP_STATUS_IS_INCOMPLETED(status) (status & KTP_STATUS_INCOMPLETED)
#define KTP_STATUS_IS_TTY_STDIN(status) (status & KTP_STATUS_TTY_STDIN)
#define KTP_STATUS_IS_TTY_STDOUT(status) (status & KTP_STATUS_TTY_STDOUT)
#define KTP_STATUS_IS_TTY_STDERR(status) (status & KTP_STATUS_TTY_STDERR)
#define KTP_STATUS_IS_NEED_STDIN(status) (status & KTP_STATUS_NEED_STDIN)
#define KTP_STATUS_IS_INTERACTIVE(status) (status & KTP_STATUS_INTERACTIVE)
#define KTP_STATUS_IS_DRY_RUN(status) (status & KTP_STATUS_DRY_RUN)
#define KTP_STATUS_IS_EXIT(status) (status & KTP_STATUS_EXIT)


#endif // _klish_ktp_h
