/*
 * websocket.h
 *
 *  Created on: 13 янв. 2023 г.
 *      Author: Admin
 */

#ifndef INC_WEBSOCKET_H_
#define INC_WEBSOCKET_H_

typedef enum {
	WS_HANDSHAKE_SEND,
	WS_HANDSHAKE_RECEIVE,
	WS_IDLE
}wsState_t;

typedef struct _ws_t {
	uint8_t payload[1024];
	uint16_t payload_len;
	wsState_t wsState;
	uint8_t pingPongFlag;
}wsHandler_t;

extern wsHandler_t wsOcppHandler;

uint8_t ws_handshake_request_handler(void);
uint8_t ws_handshake_receive(uint8_t* buf, uint16_t len);
uint8_t ws_receive(char * buffer, wsHandler_t* wsData);
uint8_t wsProcess (uint8_t * buf, uint16_t len, wsHandler_t *wsHandler);
int ws_send(char *strdata);
int wsSendPing (const char *strdata);

#endif /* INC_WEBSOCKET_H_ */
