/*
 * sim7600.c
 *
 *  Created on: Nov 1, 2022
 *      Author: Admin
 */

#include "sim7600.h"
#include "main.h"

commandId_t commandId_reply = REPLY_NONE;

//uint8_t simcomRxBuffer[RX_BUFFER_SIZE] = {0};

extern UART_HandleTypeDef huart1;
extern osMessageQId simRespMessageHandle;
extern osThreadId parcerTaskHandle;

simcomRxType simcomRxData = {0};

uint8_t simcomInitStatus = 0;

simcom_cmd_t cmds[] = {
/*{command						len,		timeout,		retry,      expect_reply}*/
{ "AT\r\n", 					4, 			500, 			10, 		REPLY_OK },
{ "AT+CPIN?\r\n", 				10, 		500, 			20, 		REPLY_CPIN },
{ "AT+CSQ\r\n", 				8, 			500, 			60, 		REPLY_CSQ },
{ "AT+CGREG?\r\n", 				11, 		500, 			60,			REPLY_CGREG },
{ "AT+NETOPEN\r\n", 			12, 		500, 			1, 			REPLY_NETOPEN },
{ "AT+CIPCLOSE=0\r\n", 			15, 		500, 			1, 			REPLY_CIPCLOSE },
{ "AT+NETCLOSE\r\n", 			13, 		500, 			1, 			REPLY_NETCLOSE },
{ "AT+CFUN=0\r\n", 				11, 		5000, 			1, 			REPLY_INCFUN },
{ "AT+CFUN=1\r\n", 				11, 		0, 				1, 			REPLY_OUTCFUN }, };

int8_t simcomExecuteCmd(simcom_cmd_t cmd) {

	osEvent event;
	uint8_t reply = REPLY_NONE;
	_DEBUG(3, "line = %d\r\n", __LINE__);

	do {
		HAL_UART_Transmit_DMA(&huart1, (uint8_t*) cmd.data, cmd.len);
		event = osMessageGet(simRespMessageHandle, (uint32_t) cmd.timeout);
		if (event.status == osEventMessage) {

			if (cmd.expect_retval == REPLY_CIPCLOSE) {
				osDelay(5000);
			} else if (cmd.expect_retval == REPLY_NETCLOSE) {
				osDelay(8000);
			}
			reply = event.value.v;
			if (reply == cmd.expect_retval) {
				_DEBUG(3, "----reply----%d\r\n", reply);
				return reply;
			}
		} else if (event.status == osEventTimeout) {
			reply = REPLY_TIMEOUT;
			_DEBUG(3, "CMD has no response\r\n");
		}
	} while (--cmd.retry_count > 0);

	return reply;
}

int8_t simcomSetServer(uint8_t *server, uint16_t port, uint16_t waitMs,
		int8_t retryCount) {

	uint8_t startConnectString[64];
	uint8_t reply = REPLY_NONE;
	uint16_t len = sprintf((char*) startConnectString,
			"AT+CIPOPEN=0,\"TCP\",\"%s\",%d\r\n", server, port);
	osEvent event;

	do {
		HAL_UART_Transmit_DMA(&huart1, startConnectString, len);
		event = osMessageGet(simRespMessageHandle, waitMs);
		if (event.status == osEventMessage) {
			reply = event.value.v;
			if (reply == REPLY_CIPOPEN) {
				return reply;
			}
		} else if (event.status == osEventTimeout) {
			reply = REPLY_TIMEOUT;
		}
	} while (--retryCount > 0);
	return reply;
}

void simcomOpenNet(void) {

	if (simcomExecuteCmd(cmds[NETOPEN]) != REPLY_NETOPEN) {

		if (commandId_reply == REPLY_ERROR) {

			_DEBUG(3, "+IP ERROR: Network is already opened and needs to close\r\n");
			if (simcomExecuteCmd(cmds[NETCLOSE]) != REPLY_NETCLOSE) {
				_DEBUG(2, "NET IS CLOSED\r\n");
			}
			osDelay(2000);
			if (simcomExecuteCmd(cmds[NETOPEN]) != REPLY_NETOPEN) {
				//переинициализировать SIM7600
			}
		}
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {

	if (huart->Instance == USART1) {
		simcomRxData.simcomRxLen = Size;

		HAL_UARTEx_ReceiveToIdle_DMA(&huart1, simcomRxData.simcomRxBuf, RX_BUFFER_SIZE);
	}
}

uint8_t simcomParcer(uint8_t *buffer, uint16_t Len) {

	uint8_t reply = REPLY_NONE;

	if (buffer[Len - 1] == '>') {

		reply = REPLY_SEND_START;

	} else if (Len >= 4 && buffer[Len - 2] == '\r' && buffer[Len - 1] == '\n') {
		_DEBUG(3, "len : %d\r\n", Len);

		if ((strncmp((const char*)(buffer + Len - 9), "AT", 2)) == 0 && strncmp((const char*)(buffer + Len - 4), "OK", 2) == 0){

			reply = REPLY_OK;

		}

		else if (Len >= 7 && Len < 66 && strncmp((const char*)(buffer + Len - 7), "ERROR", 5) == 0)
						{
			reply = REPLY_ERROR;
		}

		///CPIN
		if (Len >= 31 && strncmp((const char*)(buffer + Len - 20), "+CPIN:", 6) == 0) {
			if (strstr((const char*)buffer, "READY") != NULL) {
				reply = REPLY_CPIN;
			}
			else {
				reply = REPLY_ERROR;
			}

			_DEBUG(2, "--- CPIN --- \r\n");
		}

		///CSQ
		if (Len >= 27 && strncmp((const char*)(buffer + Len - 19), "+CSQ:", 5) == 0) {
			reply = REPLY_CSQ;

			_DEBUG(2, "--- CSQ --- %s", buffer);
		}

		///CGREG
		if (Len >= 31 && strncmp((const char*)(buffer + Len - 19), "+CGREG:", 7) == 0) {
			if (strstr((const char*)buffer, "0,1") != NULL || strstr((const char*)buffer, "0,5") != NULL) {
				reply = REPLY_CGREG;
			}
			_DEBUG(2, "--- CGREG --- \r\n");
		}

		///NETOPEN
		if (Len >= 9 && Len < 16 && strstr((const char*)buffer, "+NETOPEN:") != NULL) {
			if (buffer[Len - 3] == '0')
				reply = REPLY_NETOPEN;
			else
				reply = REPLY_NETOPEN_ERROR;
			_DEBUG(2, "--- NETOPEN --- \r\n");
		}

		///NETCLOSE
		else if (Len >= 16 && strstr((const char*)buffer, "+NETCLOSE: 0") != NULL) {
			printf("---- NETCLOSE --- %c\r\n", buffer[Len - 3]);
			reply = REPLY_NETCLOSE;
		}

		///CIPOPEN
		if (Len >= 17 && Len < 66 && (strstr((const char*)buffer, "+CIPOPEN:") != NULL)) {
			if (buffer[Len - 3] == '0') {
				_DEBUG(2, "---- CIPOPEN --- \r\n");
				reply = REPLY_CIPOPEN;
			} else
				reply = REPLY_ERROR;
		}

		///CIPOPEN ----����ʧ��
		else if (Len >= 66 && strncmp((const char*)(buffer + Len - 24), "+CIPOPEN:", 9) == 0) {
			if (buffer[Len - 12] == '4') {
				reply = REPLY_CIPOPEN_ERROR;
				_DEBUG(2, "---- CIPOPEN_ERROR --- %c\r\n",
						buffer[Len - 24]);
			} else
				reply = REPLY_ERROR;
		}

		///REPLY_CIPCLOSE
		if (Len >= 18 && (strstr((const char*)buffer, "+CIPCLOSE") != NULL)) {

			if (buffer[Len - 3] == '0') {
				_DEBUG(2, "---+CIPCLOSE:---\r\n");
				reply = REPLY_CIPCLOSE;
			}
		}

		///CIPSEND ݷ������״̬
		if (strstr((const char*)buffer, "+CIPSEND:") != NULL) {
			_DEBUG(2, "CIPSEND\r\n");
			reply = REPLY_SEND_DONE;
		}

		///
		if (Len >= 13 && strstr((const char*)buffer, "+IPCLOSE: 0,1") != NULL) {
			_DEBUG(3, "REPLY_CIPCLOSE\r\n");
			reply = REPLY_CIPCLOSE;
		}

	}

	else if (Len >= 40 && buffer[Len - 1] == '\n'){

		reply = REPLY_REVER;
		if (strstr((const char*)buffer, "RECV FROM:") != NULL){

			char *p = strstr((const char*)buffer, "+IPD23");

			uint8_t data = (uint8_t*)p - &buffer[0];

			_DEBUG(2, "--RECV---%s", &buffer[data + strlen("+IPD23\r\n")]); ///
		}
	}

	return reply;
}

uint8_t simcomInit (void) {

	if (simcomExecuteCmd(cmds[AT]) != REPLY_OK) {
		puts("AT not response\r\n");
		osDelay(5000);
		return INIT_SIM_FAIL;
	} else {
		puts ("AT is works!\r\n");
	}

	if (simcomExecuteCmd(cmds[CHECK_SIM]) != REPLY_CPIN) {
		osDelay(1000);
		puts ("CPIN error, CFUN = 0\r\n");
		return INIT_SIM_FAIL;
	}

	if (simcomExecuteCmd(cmds[CSQ]) != REPLY_CSQ){
		puts("Signal quality ERROR. Trying to reconnect\r\n");
		simcomExecuteCmd(cmds[INCFUN]);
		osDelay(6000);
		simcomExecuteCmd(cmds[OUTCFUN]);
	}

	if (simcomExecuteCmd(cmds[CGREG]) != REPLY_CGREG){
		puts ("Not registerd\r\n");
		return INIT_SIM_FAIL;
	}
	return INIT_SIM_DONE;
}


void parcer(void const * argument){

	osEvent event;

	event = osSignalWait(0x01, osWaitForever);
	if (event.status == osEventSignal) {
		if (event.value.v == 0x01){

		}
	}
}

