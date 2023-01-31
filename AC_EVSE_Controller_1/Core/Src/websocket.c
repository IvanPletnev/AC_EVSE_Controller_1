/*
 * websocket.c
 *
 *  Created on: 13 янв. 2023 г.
 *      Author: Admin
 */
#include "main.h"
#include "websocket.h"
#include <string.h>
#include <stdlib.h>
#include "cmsis_os.h"
#include "sim7600.h"
#include "utils.h"
#include "ocpp.h"

extern UART_HandleTypeDef huart1;

const char http_path[]       = "";
const char http_host[]       = "158.160.51.94";
const char http_port[]       = "9001";

#define WS_CHECKTABLE_COUNT 3
const char   str_websocket[]   = "websocket";
const char   str_Connection[]  = "Connection:";
const char     str_upgrade[]   = "upgrade";
const char *ws_checktable[WS_CHECKTABLE_COUNT] = {
    str_upgrade,
    str_websocket,
    str_Connection
};

const char     str_http_11[]   = "HTTP/1.1 101";
const char     str_http_10[]   = "HTTP/1.0 101";
const char str_SecWsAccept[]   = "Sec-WebSocket-Accept:";
const char       str_UUID[]    = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

wsHandler_t wsOcppHandler = {0};

extern osMessageQId uartTxHandle;


int ws_send(char *strdata){
	unsigned char mask[4];
	unsigned long long payload_len;
	unsigned char finNopcode;
	unsigned int payload_len_small;
	unsigned int payload_offset = 6;
	unsigned int len_size;
	int i;
	unsigned int frame_size;
	char *data;

	finNopcode = 0x81; //FIN and text opcode.
	for (i = 0; i < 4; i++) {
		mask[i] = rand () % 256;
	}
	payload_len = strlen(strdata);
	if(payload_len <= 125) {
		frame_size = 6 + payload_len;
		payload_len_small = payload_len;

	} else if(payload_len > 125 && payload_len <= 0xffff) {
		frame_size = 8 + payload_len;
		payload_len_small = 126;
		payload_offset += 2;
	} else if(payload_len > 0xffff && payload_len <= 0xffffffffffffffffLL) {
		frame_size = 14 + payload_len;
		payload_len_small = 127;
		payload_offset += 8;
	} else {
		return -1; //too large payload
	}
	data = (char *)pvPortMalloc(frame_size);
	memset(data, 0, frame_size);
	*data = finNopcode;
	*(data+1) = payload_len_small | 0x80; //payload length with mask bit on
	if(payload_len_small == 126) {
		payload_len &= 0xffff;
		len_size = 2;
		for(i = 0; i < len_size; i++) {
			*(data+2+i) = *((char *)&payload_len+(len_size-i-1));
		}
	}
	if(payload_len_small == 127) {
		payload_len &= 0xffffffffffffffffLL;
		len_size = 8;
		for(i = 0; i < len_size; i++) {
			*(data+2+i) = *((char *)&payload_len+(len_size-i-1));
		}
	}
	for(i=0;i<4;i++)
		*(data+(payload_offset-4)+i) = mask[i];

	memcpy(data+payload_offset, strdata, strlen(strdata));
	for(i=0;i<strlen(strdata);i++)
		*(data+payload_offset+i) ^= mask[i % 4] & 0xff;
	i = 0;

	uartTxDataQueueSend((uint8_t*)data, frame_size);
	secCounter = 0;

	vPortFree((char *)data);

	return 0;
}

uint8_t ws_receive(char * buffer, wsHandler_t* wsData) {

    uint8_t ws_receive_err = 1;
    if (buffer[0] == 0x81) {
        if ((buffer[1] & 0x80) == 0x00) {
            uint16_t payloadLen = (buffer[1] & 0x7F);
            uint8_t payload_offset = 2;

            if(payloadLen == 126) {
                payloadLen = buffer[2] << 8 | buffer[3];
                payload_offset += 2;
//                printf("payload_length=%u\n", payloadLen);
            }
            ws_receive_err = 0; // OK
            memcpy(wsData->payload, (buffer + payload_offset), payloadLen);
            wsData->payload_len = payloadLen;
        } else {
        	ws_receive_err = 2;
        } // FRAME MASKED
    }

    if (ws_receive_err) {
//        printf("\nWsRx.buff[0] %u\n", buffer[0]);
//        printf("\nWS_RECV ERR %u\n", ws_receive_err);
    }
    return 1;
}

uint8_t ws_handshake_request_handler(void){

	char request_headers[512];
	uint16_t len = 0;

	len = snprintf(request_headers, 1024, "GET %s HTTP/1.1\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nHost: %s:%d\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Protocol: ocpp1.6\r\nSec-WebSocket-Version: 13\r\n\r\n", "/KOS0012022RUqq", WS_SERVER_ADDR, SERVER_PORT);
	uartTxDataQueueSend((uint8_t*)request_headers, len);

	return 0;
}

uint8_t ws_handshake_receive(uint8_t* buf, uint16_t len) {

    uint8_t ws_upgrade_reply_err = 1;
    if ( len > 99 ) {
        if ((strstr((const char *)buf, str_http_11) != NULL) || (strstr((const char *)buf, str_http_10) != NULL)) {
            uint8_t check = 0;
            for (uint8_t x = 0; x < WS_CHECKTABLE_COUNT; ++x) {
                if (strcasestr((const char *)buf, ws_checktable[x]) != NULL) {
                	check++;
                }
                if (check == WS_CHECKTABLE_COUNT) {
                    if (strcasestr((const char *)buf, str_SecWsAccept) != NULL) {
//						ws_stream_print = ws_stream_ws_masking;
						ws_upgrade_reply_err = 0;
						return 1;
                    } else { ws_upgrade_reply_err = 5; } // bad WSACCEPT;
                }
            }
            if (check != WS_CHECKTABLE_COUNT) { ws_upgrade_reply_err = 4; } // check string
        } else { ws_upgrade_reply_err = 3; } // NO_HTTPx;
    } else { ws_upgrade_reply_err = 2; } // low size
    if (ws_upgrade_reply_err > 2) {
        printf("\nUPGRADE ERR %u\n", ws_upgrade_reply_err);
        // do reboot modem TODO
    }
    return 0;
}

//uint8_t wsProcess (uint8_t * buf, uint16_t len, wsHandler_t *wsHandler){
//	switch (wsHandler->wsState) {
//	case WS_HANDSHAKE_SEND:
//		ws_handshake_request_handler();
//		wsHandler->wsState = WS_HANDSHAKE_RECEIVE;
//		break;
//	case WS_HANDSHAKE_RECEIVE:
//		if (ws_handshake_receive(buf, len)) {
//			wsHandler->wsState = WS_IDLE;
//			break;
//	case WS_IDLE:
//		break;
//		}
//	}
//	return 1;
//}
