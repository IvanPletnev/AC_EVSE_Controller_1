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
#define 	SERVER_ADDR				"158.160.51.94"
#define 	SERVER_PORT				8008

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

typedef uint8_t commandId_t;

//#define RXBUFFERSIZE   128 //�����С

//#define UART_RECEIVE_LEN  			128  	//�����������ֽ��� 128

#define 	RX_BUFFER_SIZE			128
#define 	USART_DMA_SENDING 		1//发送未完成
#define 	USART_DMA_SENDOVER 		0//发送完成

#define MAIL_SIZE	4

typedef struct{

	uint8_t	simcomRxFlag;
	uint8_t	simcomRxBuf[RX_BUFFER_SIZE];     //���ջ���,���USART_REC_LEN���ֽ�.
	uint16_t simcomRxLen;       //�������ݳ���

}simcomRxType;

typedef enum {
	NOT_INITIALIZED,
	INITIALIZED
}simcomInitStatus;

typedef struct _simcom_t {
	simcomInitStatus initStatus;
}simcom_t;



extern commandId_t commandId_reply;

extern simcom_t simcomHandler;


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
	CTZU
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
extern simcomRxType simcomRxData;

uint8_t simcomInit (void);
int8_t simcomExecuteCmd(simcom_cmd_t cmd);


#endif /* INC_SIM7600_H_ */
