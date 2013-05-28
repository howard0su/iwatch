#ifndef _OBEX_H_
#define _OBEX_H_

#define OBEX_OP_CONNECT 	0x00
#define OBEX_OP_DISCONNECT 	0x01
#define OBEX_OP_PUT			0x02
#define OBEX_OP_GET			0x03

#define OBEX_OP_LAST_FLAG   0x80

#define OBEX_RESPCODE_OK	200


typedef struct connection_obex
{
	uint8_t opcode;
	uint16_t length;
	uint8_t version;
	uint8_t flags;
	uint16_t max_packet_length;
	uint8_t data[0];
};

typedef struct operation_obex
{
	uint8_t opcode;
	uint16_t length;
	uint8_t data[0];
};

#endif