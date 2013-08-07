#ifndef _OBEX_H_
#define _OBEX_H_

#define OBEX_OP_CONNECT 	0x00
#define OBEX_OP_DISCONNECT 	0x01
#define OBEX_OP_PUT			0x02
#define OBEX_OP_GET			0x03

#define OBEX_OP_LAST_FLAG   0x80

#define OBEX_RESPCODE_OK	200

struct obex_state
{
	int state;
	uint32_t connection;
	uint32_t target;
};

#define OBEX_CB_SEND 1
#define OBEX_CB_NEWCONN 2
#define OBEX_CB_DISCONNECT 3
#define OBEX_CB_PUTOBJECT 4
#define OBEX_CB_GETOBJECT 5


struct obex
{
	struct obex_state *state;
	void (*callback)(int code, void* lparam, uint16_t rparam);
};

#pragma pack(push, 1)
typedef struct _connection_obex
{
	uint8_t opcode;
	uint16_t length;
	uint8_t version;
	uint8_t flags;
	uint16_t max_packet_length;
	uint8_t data[0];
}connection_obex;

typedef struct _operation_obex
{
	uint8_t opcode;
	uint16_t length;
	uint8_t data[0];
}operation_obex;
#pragma pack(pop)
#define OBEX_HEADER_COUNT 0xC0
#define OBEX_HEADER_NAME 0x01
#define OBEX_HEADER_TYPE 0x42
#define OBEX_HEADER_LENGTH 0xC3
#define OBEX_HEADER_TARGET 0x46
#define OBEX_HEADER_BODY   0x48
#define OBEX_HEADER_ENDBODY 0x49
#define OBEX_HEADER_CONNID  0xCB
#define OBEX_HEADER_APPPARMS 0x4C
#define OBEX_HEADER_AUTHCHALLENGE 0x4D
#define OBEX_HEADER_AUTHRESPONSE  0x4E

void obex_init(const struct obex*);
void obex_handle(const struct obex* state, const uint8_t* packet, uint16_t length);
void obex_connect(const struct obex* obex, const uint8_t *target, uint8_t target_length);
uint8_t* obex_create_request(const struct obex* obex, int opcode, uint8_t* buf);
void obex_send(const struct obex* obex, uint8_t* buf, uint16_t length);

uint8_t* obex_header_add_text(uint8_t *buf, int code, const char* text);
uint8_t* obex_header_add_bytes(uint8_t *buf, int code, const uint8_t *data, int length);
uint8_t *obex_header_add_byte(uint8_t *buf, int code, uint8_t data);
uint8_t *obex_header_add_uint32(uint8_t *buf, int code, uint32_t data);


#endif