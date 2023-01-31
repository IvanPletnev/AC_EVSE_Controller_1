/*
 * sim7600.h
 *
 *  Created on: Nov 1, 2022
 *      Author: Admin
 */

#ifndef INC_SIM7600_H_
#define INC_SIM7600_H_

#include "debug.h"
#include "cmsis_os.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "FreeRTOS.h"

#define 	INIT_SIM_DONE			0
#define 	INIT_SIM_FAIL			1
#define 	SERVER_ADDR				"mos.ocpp.test.intermb.ru"
#define 	WS_SERVER_ADDR			"mos.ocpp.test.intermb.ru"
#define 	SERVER_PORT				80

#define REPLY_NONE						0
#define REPLY_OK						1
#define REPLY_ERROR						2
#define REPLY_CPIN						3
#define REPLY_CSQ						4
#define REPLY_CGREG						5
#define REPLY_NETOPEN					6
#define REPLY_CIPCLOSE					7
#define REPLY_NETCLOSE					8
#define REPLY_INCFUN					9
#define REPLY_OUTCFUN					10

#define REPLY_NETOPEN_ERROR				11
#define REPLY_CIPOPEN_ERROR				12
#define REPLY_CIPOPEN					13
#define REPLY_SEND_START				14
#define REPLY_SEND_DONE					15
#define REPLY_SENDERROR					16
#define REPLY_REVER						17
#define REPLY_TIMEOUT					18
#define REPLY_NO_TIME_UPDATE			19
#define REPLY_TIME_UPDATE_AUTO			20
#define REPLY_TIME_UPDATE_ENABLED		21
#define REPLY_DATE_TIME					22
#define REPLY_CIPMODE					23
#define REPLY_CIPMODE_OFF				24
#define REPLY_CIPOPEN_TRANSPARENT		25
#define REPLY_TRANSPARENT_OFF			26

typedef uint8_t commandId_t;

//#define RXBUFFERSIZE   128 //�����С

//#define UART_RECEIVE_LEN  			128  	//�����������ֽ��� 128

#define 	RX_BUFFER_SIZE			1024
#define 	USART_DMA_SENDING 		1//发送未完成
#define 	USART_DMA_SENDOVER 		0//发送完成

#define MAIL_SIZE	4

typedef struct{

	uint8_t	simcomRxFlag;
	uint8_t	simcomRxBuf[RX_BUFFER_SIZE];     //���ջ���,���USART_REC_LEN���ֽ�.
	uint16_t simcomRxLen;       //�������ݳ���

}simcomRx_t;

typedef enum {
	NOT_INITIALIZED,
	INITIALIZED,
}simcomInitStatus;
typedef enum {
	NOT_TRANSPARENT_MODE,
	TRANSPARENT_MODE
}transferMode_t;

typedef enum {
	NET_CLOSED,
	NET_OPENED,
}simcomNetStatus;

typedef enum {
	SERVER_DISCONNECTED,
	SERVER_CONNECTED
}simcomServerStatus;

typedef struct _simcom_t {

	simcomInitStatus initStatus;
	simcomNetStatus netStatus;
	simcomServerStatus serverStatus;
	simcomRx_t simcomRxData;
	transferMode_t transferMode;
	uint8_t initState;

}simcom_t;



extern commandId_t commandId_reply;

extern volatile simcom_t simcomHandler;


enum {
	AT,
	CHECK_SIM,
	CSQ,
	CGREG,
	NETOPEN,
	CIPCLOSE,
	NETCLOSE,
	INCFUN,
	OUTCFUN,
	CTZU,
	CTZU_ENABLE,
	CCLK,
	CIPMODE
};


typedef struct simcom_cmd_struct
{
	char *data;
	uint16_t len;
	uint32_t timeout;
	int8_t retry_count;
	int8_t expect_retval;

}simcom_cmd_t;


extern const simcom_cmd_t cmds[];
//extern uint8_t simcomRxBuffer[RX_BUFFER_SIZE];
extern simcomRx_t simcomRxData;
extern uint32_t lTime;

uint8_t simcomInit (void);
int8_t simcomExecuteCmd(simcom_cmd_t cmd);


#endif /* INC_SIM7600_H_ */
