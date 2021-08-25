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
	KTP_EXIT = 'x',
	KTP_AUTH = 'a',
	KTP_AUTH_ACK = 'A',
	KTP_KEEPALIVE = 'k',
} ktp_cmd_e;


typedef enum {
	KTP_PARAM_NULL = '\0',
	KTP_PARAM_LINE = 'L',
	KTP_PARAM_ERROR = 'E',
} ktp_param_e;


// Status field. Bitmap
#define KTP_STATUS_NONE  (uint32_t)0x00000000
#define KTP_STATUS_ERROR (uint32_t)0x00000001

#define KTP_STATUS_IS_ERROR(status) (status & KTP_STATUS_ERROR)


#endif // _klish_ktp_h
