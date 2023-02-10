/*
 * sim7600.c
 *
 *  Created on: Nov 1, 2022
 *      Author: Admin
 */

#include "sim7600.h"
#include "main.h"
#include "cmsis_os.h"
#include "websocket.h"
#include "ocpp.h"
#include <stdlib.h>
#include "rtclib.h"

commandId_t commandId_reply = REPLY_NONE;

//uint8_t simcomRxBuffer[RX_BUFFER_SIZE] = {0};

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern osMessageQueueId_t simRespMessageHandle;
extern osThreadId parcerTaskHandle;
extern osMessageQueueId_t qSimcomRxHandle;;
extern int8_t testResponse;
extern RTC_TimeTypeDef sTime;
extern RTC_DateTypeDef sDate;
extern RTC_HandleTypeDef hrtc;

simcomRx_t simcomRxData = {0};

volatile simcom_t simcomHandler = {0};
extern volatile uint16_t pingPongCounter;



const simcom_cmd_t cmds[] = {
/*{command						len,		timeout,		retry,      expect_reply}*/
{ "AT\r\n", 					4, 			2000, 			30, 		REPLY_OK },
{ "AT+CPIN?\r\n", 				10, 		3000, 			20, 		REPLY_CPIN },
{ "AT+CSQ\r\n", 				8, 			500, 			60, 		REPLY_CSQ },
{ "AT+CGREG?\r\n", 				11, 		500, 			60,			REPLY_CGREG },
{ "AT+NETOPEN\r\n", 			12, 		1000, 			1, 			REPLY_NETOPEN },
{ "AT+CIPCLOSE=0\r\n", 			15, 		3000, 			1, 			REPLY_CIPCLOSE },
{ "AT+NETCLOSE\r\n", 			13, 		500, 			1, 			REPLY_NETCLOSE },
{ "AT+CFUN=0\r\n", 				11, 		5000, 			1, 			REPLY_INCFUN },
{ "AT+CFUN=1\r\n", 				11, 		3000,			1, 			REPLY_OUTCFUN },
{ "AT+CTZU?\r\n", 				10, 		2000, 			5, 			REPLY_TIME_UPDATE_AUTO },
{ "AT+CTZU=1\r\n", 				11, 		1000, 			5, 			REPLY_TIME_UPDATE_ENABLED },
{ "AT+CCLK?\r\n", 				10, 		1000, 			3, 			REPLY_DATE_TIME },
{ "AT+CIPMODE=1\r\n", 			14, 		500, 			10,			REPLY_CIPMODE },
{ "+++",			 			3,	 		2000, 			3, 			REPLY_TRANSPARENT_OFF },
{ "AT+IPR=460800\r\n",			15,	 		1000, 			3, 			REPLY_BAUDRATE }
};

uint8_t dateTimeStrToINt (const uint8_t * buf) {
	uint8_t digit1 = 0;
	uint8_t digit2 = 0;

	digit1 = *buf - 0x30;
	digit1 *= 10;
	digit2 = *(buf+1) - 0x30;
	return digit1 + digit2;
}

int8_t simcomExecuteCmd(simcom_cmd_t cmd) {

	uint32_t reply = REPLY_NONE;
	osStatus_t result;

	do {
		uartTxDataQueueSend((uint8_t*)cmd.data, cmd.len);
		printf ("\r\nCMD %s has sent. Expected response is --%d--\r\n", cmd.data, cmd.expect_retval);
		if ((result = osMessageQueueGet(simRespMessageHandle, &reply, 0, cmd.timeout)) == osOK) {
			if (reply == cmd.expect_retval) {
				_DEBUG(3, "----reply----%d\r\n", (uint8_t) reply);
					return reply;
			} else {
				osDelay(200);
			}
		} else if (result == osErrorTimeout) {
			reply = REPLY_TIMEOUT;
			printf ("\r\nCmd  %s timed out\r\n", cmd.data);
		}
	} while (--cmd.retry_count);
	return reply;
}

int8_t simcomSetServer(char *server, uint16_t port, uint16_t waitMs, int8_t retryCount) {

	uint8_t startConnectString[64];
	uint32_t reply = REPLY_NONE;
	uint16_t len = sprintf((char*) startConnectString, "AT+CIPOPEN=0,\"TCP\",\"%s\",%d\r\n", server, port);
	osStatus_t result;

	do {
		uartTxDataQueueSend(startConnectString, len);
		if ((result = osMessageQueueGet(simRespMessageHandle, &reply, 0, waitMs)) == osOK) {
			if (reply == REPLY_CIPOPEN_TRANSPARENT) {
				return reply;
			} else {
				osDelay(200);
			}
		} else if (result == osErrorTimeout) {
			reply = REPLY_TIMEOUT;
		}
	} while (--retryCount > 0);
	return reply;
}

int8_t simcomOpenNet(void) {

	int8_t reply = REPLY_NONE;
	reply = simcomExecuteCmd(cmds[NETOPEN]);

	if (reply != REPLY_NETOPEN) {
		if (reply == REPLY_ERROR) {
			_DEBUG(3, "+IP ERROR: Already connected\r\n");
			if (simcomExecuteCmd(cmds[NETCLOSE]) != REPLY_NETCLOSE) {
				_DEBUG(3, "NET IS CLOSED\r\n");
			}
			osDelay(2000);
			reply = simcomExecuteCmd(cmds[NETOPEN]);
			if (reply != REPLY_NETOPEN) {
				//переинициализировать SIM7600
			} else {
				_DEBUG(3, "NET IS OPEN\r\n");
			}
		}
	} else {
		puts ("NET IS OPEN\r\n");
	}
	return reply;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1) {
		uart1Status = READY;
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {

	if (huart->Instance == USART1) {
		simcomHandler.simcomRxData.simcomRxLen = Size;
		if (osMessageQueuePut (qSimcomRxHandle, (const void *)&simcomHandler.simcomRxData, 0, 0) == osOK) {

		}
		HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t*)simcomHandler.simcomRxData.simcomRxBuf, RX_BUFFER_SIZE);
	}
}

uint8_t simcomParcer(uint8_t *buffer, uint16_t Len, simcom_t * simcom) {

	uint8_t reply = REPLY_NONE;
	uint8_t dateFlag = 0;

	if (buffer[Len - 1] == '>') {

		reply = REPLY_SEND_START;

	} else if ((Len >= 4 && buffer[Len - 2] == '\r') && (buffer[Len - 1] == '\n')) {

		if (((strncmp((const char*)(buffer + Len - 9), "AT", 2)) == 0) && (strncmp((const char*)(buffer + Len - 4), "OK", 2) == 0)){

			reply = REPLY_OK;
		}

		else if ((Len >= 7) && (Len < 66) && (strncmp((const char*)(buffer + Len - 7), "ERROR", 5) == 0))
						{
			reply = REPLY_ERROR;
		}

		///CPIN
		if ((Len >= 31) && (strncmp((const char*)(buffer + Len - 20), "+CPIN:", 6) == 0)) {
			if (strstr((const char*)buffer, "READY") != NULL) {
				reply = REPLY_CPIN;
			}
			else {
				reply = REPLY_ERROR;
			}
		}

		if ((Len >= 20) && (strstr ((const char *)(buffer), "AT+IPR=460800") != NULL)) {
			reply = REPLY_BAUDRATE;
		}

		///CSQ
		if ((Len >= 27) && (strncmp((const char*)(buffer + Len - 19), "+CSQ:", 5) == 0)) {
			reply = REPLY_CSQ;
		}

		///CGREG
		if ((Len >= 31) && (strncmp((const char*)(buffer + Len - 19), "+CGREG:", 7) == 0)) {
			if ((strstr((const char*)buffer, "0,1") != NULL) || (strstr((const char*)buffer, "0,5") != NULL)) {
				reply = REPLY_CGREG;
			}
		}

		///NETOPEN
		if ((Len >= 9) && (Len < 16) && (strstr((const char*)buffer, "+NETOPEN:") != NULL)) {
			if (buffer[Len - 3] == '0')
				reply = REPLY_NETOPEN;
		}

		///NETCLOSE
		if ((Len >= 16) && (strstr((const char*)buffer, "+NETCLOSE: 0") != NULL)) {
			reply = REPLY_NETCLOSE;
		}

		///CFUN
		if (strstr((const char *) buffer, "+SIMCARD: NOT AVAILABLE")!=NULL )
		{
			reply = REPLY_INCFUN;
		}

		else if ( strstr((const char *)buffer, "PB DONE")!=NULL )
		{
			reply = REPLY_OUTCFUN;
		}

		///CTZU
		if ((Len >= 27) && (strstr ((const char*)buffer, "+CTZU: ") != NULL)) {
			if (buffer[Len - 9] == '1') {
				reply = REPLY_TIME_UPDATE_AUTO;
			} else {
				reply = REPLY_NO_TIME_UPDATE;
			}
		}

		///CTZU=1
		if ((strcmp ((const char*)buffer, "AT+CTZU=1") == 0) && (strncmp((const char*)(buffer + Len - 4), "OK", 2) == 0)) {
			reply = REPLY_TIME_UPDATE_ENABLED;
		}

		///CCLK

		if ((strstr ((const char *)buffer, "+CCLK: ") != NULL) && (strncmp((const char*)(buffer + Len - 4), "OK", 2) == 0)) {

			sDate.Year = dateTimeStrToINt (buffer + Len - 29);
			sDate.Month = dateTimeStrToINt(buffer + Len - 29 + 3);
			sTime.Hours = dateTimeStrToINt(buffer + Len - 29 + 9);
			if (sTime.Hours < GMT_OFFSET) {
				sTime.Hours += (24 - GMT_OFFSET);
				dateFlag = 1;
			} else {
				sTime.Hours -= GMT_OFFSET;
			}
			sDate.Date = dateTimeStrToINt(buffer + Len - 29 + 6);
			if (dateFlag) {
				sDate.Date -= 1;
				if (sDate.Date == 0) {
					sDate.Month -= 1;
					if (sDate.Month == 0) {
						sDate.Date = 31;
						sDate.Month = 12;
						sDate.Year -= 1;
					} else {
						sDate.Date = daysInMonthArray[sDate.Month];
					}
				}
			}
			sTime.Minutes = dateTimeStrToINt(buffer + Len - 29 +12);
			sTime.Seconds = dateTimeStrToINt(buffer + Len - 29 + 15);

			reply = REPLY_DATE_TIME;
		}

		///CIPOPEN
		if ((Len >= 17) && (Len < 66) && (strstr((const char*)buffer, "+CIPOPEN:") != NULL)) {
			if (buffer[Len - 3] == '0') {
				reply = REPLY_CIPOPEN;
			} else
				reply = REPLY_ERROR;
		}

		///CIPOPEN ----����ʧ��
		else if ((Len >= 66) && (strncmp((const char*)(buffer + Len - 24), "+CIPOPEN:", 9) == 0)) {
			if (buffer[Len - 12] == '4') {
				reply = REPLY_CIPOPEN_ERROR;
			} else
				reply = REPLY_ERROR;
		}

		///CIPOPEN TRANSPARENT
		else if ((Len >= 18) && (strncmp((const char*)(buffer+2), "CONNECT ", 8) == 0)) {
			reply = REPLY_CIPOPEN_TRANSPARENT;
		}

		///CIPMODE
		if ((Len >= 19) && (strstr ((const char*)buffer, "+CIPMODE=") != NULL)){
			if (strstr((const char*)buffer+Len-4, "OK") != NULL) {
				reply = REPLY_CIPMODE;
			}
			else {
				reply = REPLY_ERROR;
			}
		}

		///REPLY_CIPCLOSE
		if ((Len >= 18) && (strstr((const char*)buffer, "+CIPCLOSE") != NULL)) {

			if (buffer[Len - 3] == '0') {
				reply = REPLY_CIPCLOSE;
			}
		}

		///CIPSEND ݷ������״̬
		if (strstr((const char*)buffer, "+CIPSEND:") != NULL) {
			reply = REPLY_SEND_DONE;
		}
		/// +IPCLOSE
		if ((Len >= 13) && (strstr((const char*)buffer, "+IPCLOSE: 0,1") != NULL)) {
			reply = REPLY_CIPCLOSE;
		}
	}

	else if ((Len >= 40) && (buffer[Len - 1] == '\n')){

		reply = REPLY_REVER;
		if (strstr((const char*)buffer, "RECV FROM:") != NULL){

//			char *p = strstr((const char*)buffer, "+IPD23");
//			uint8_t data = (uint8_t*)p - &buffer[0];
//			_DEBUG(2, "--RECV---%s", &buffer[data + strlen("+IPD23\r\n")]); ///
		}
	}

	return reply;
}


uint8_t simcomInit (uint8_t state) {

	simcomHandler.initState = state;
	static uint8_t lockFlag = 0;

	switch (simcomHandler.initStatus) {
	case NOT_INITIALIZED:

		switch (simcomHandler.initState) {
		case 0:
			testResponse = simcomExecuteCmd(cmds[AT]);
			if (testResponse != REPLY_OK) {
				puts ("\r\nCMD AT FAILED\r\n");
				return INIT_SIM_FAIL;
			} else {
				osDelay(100);
				lockFlag = 0;
				printf ("Response: %d\r\n", testResponse);
				simcomHandler.initState = 4;
			}
//		case 2:
//			if (!lockFlag) {
//				lockFlag = 1;
//				testResponse= simcomExecuteCmd(cmds[BAUDRATE]);
//				simcomHandler.initState++;
//			}
//		case 3:
//			if (testResponse != REPLY_BAUDRATE) {
//				puts ("\r\nBAUDRATE CHANGE FAILED\r\n");
//				return INIT_SIM_FAIL;
//			} else {
//				printf ("\r\nBaudRate 460800. Response: %d\r\n", testResponse);
//				lockFlag = 0;
//				huart1.Init.BaudRate = 460800;
//				HAL_UART_Init(&huart1);
//				simcomHandler.initState++;
//			}
		case 4:
			if (!lockFlag) {
				lockFlag = 1;
				testResponse = simcomExecuteCmd(cmds[CTZU]);
				if (testResponse == REPLY_NO_TIME_UPDATE) {
					osDelay(100);
					testResponse = simcomExecuteCmd(cmds[CTZU_ENABLE]);
					if (testResponse == REPLY_TIME_UPDATE_ENABLED) {
						osDelay(100);
						testResponse = simcomExecuteCmd(cmds[INCFUN]);
						if (testResponse == REPLY_INCFUN) {
							osDelay(6000);
							testResponse = simcomExecuteCmd(cmds[OUTCFUN]);
							if (testResponse == REPLY_OUTCFUN) {
								simcomHandler.initState++;
								lockFlag = 0;
							}
						}
					}
				} else if (testResponse == REPLY_TIME_UPDATE_AUTO) {
					osDelay(100);
					simcomHandler.initState++;
					lockFlag = 0;
					puts ("Network time enabled \r\n");
				}
			}
		case 5:
			testResponse = simcomExecuteCmd(cmds[CCLK]);
			if (testResponse == REPLY_DATE_TIME) {
				HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
				HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
				osDelay(100);
				//simcomHandler.initState++;
			}
			HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
			osDelay(100);
			simcomHandler.initState++;
		case 6:
			testResponse = simcomExecuteCmd(cmds[CHECK_SIM]);
			if (testResponse != REPLY_CPIN) {
				puts ("\r\nCMD CPIN FAILED\r\n");
				return INIT_SIM_FAIL;
			} else if (testResponse == REPLY_CPIN) {
				osDelay(100);
				printf ("SIM card present. Response: %d\r\n", testResponse);
				simcomHandler.initState++;
			}
		case 7:
			testResponse = simcomExecuteCmd(cmds[CGREG]);
			if (testResponse != REPLY_CGREG) {
				puts ("CMD CGREG FAILED\r\n");
				return INIT_SIM_FAIL;
			} else if (testResponse == REPLY_CGREG) {
				printf ("CREG. Response: %d\r\n", testResponse);
				osDelay(100);
				simcomHandler.initState++;
			}
		case 8:
			testResponse = simcomExecuteCmd(cmds[CIPMODE]);
			if (testResponse != REPLY_CIPMODE) {
				puts ("CMD SET CIPMODE FAILED\r\n");
				return INIT_SIM_FAIL;
			} else {
				printf ("Transparent mode enabled. Response: %d\r\n", testResponse);
				osDelay(100);
				simcomHandler.initState++;;
			}
		case 9:
			testResponse = simcomOpenNet();
			if (testResponse != REPLY_NETOPEN) {
				puts ("CMD NETOPEN FAILED\r\n");
			} else {
				puts ("CMD NETOPEN DONE\r\n");
				simcomHandler.netStatus = NET_OPENED;
				osDelay(100);
				printf ("\r\nSocket has been open. Response: %d\r\n", testResponse);
				simcomHandler.initState++;
			}
		case 10:
			testResponse = simcomSetServer(SERVER_ADDR, SERVER_PORT, 2000, 10);
			if (testResponse != REPLY_CIPOPEN_TRANSPARENT) {
				puts ("CIPOPEN FAILED\r\n");
	//			return INIT_SIM_FAIL;
			} else if (testResponse == REPLY_CIPOPEN_TRANSPARENT){
				simcomHandler.transferMode = TRANSPARENT_MODE;
				simcomHandler.serverStatus = SERVER_CONNECTED;
				simcomHandler.error = ERROR_NONE;
				puts ("\r\n-------------Connected to server-------------\r\n");
				osDelay(100);
				simcomHandler.initStatus = INITIALIZED;
			}
		}
		break;
	case INITIALIZED:
		break;
	}
	return INIT_SIM_DONE;
}

void parcerTask(void const *argument) {

	volatile simcom_t *simcom;
	simcom = (simcom_t*) argument;
	simcomRx_t simcomData = { 0 };
	uint32_t tempResponse = 0;
	const uint8_t pong[6] = {0x8A, 0x80, 0x52, 0xFA, 0x1C, 0x6B};

	for (;;) {

		if (osMessageQueueGet(qSimcomRxHandle, &simcomData, 0, osWaitForever) == osOK) {

			if (simcom->transferMode == NOT_TRANSPARENT_MODE) {//SIM7600 COMMAND MODE
				tempResponse = simcomParcer(simcomData.simcomRxBuf, simcomData.simcomRxLen, (simcom_t*) &simcom);
				if (tempResponse != 0) {
					osMessageQueuePut(simRespMessageHandle, (uint32_t*)&tempResponse, 0, 300);
				}
			} else if (simcom->transferMode == TRANSPARENT_MODE) {//SIM7600 TRANSPARENT MODE
				if ((simcomData.simcomRxLen == 6) && (strncmp((const char*) (simcomData.simcomRxBuf), "\r\nOK\r\n", 6) == 0)) {//EXIT TRANSPARENT MODE (+++)
					tempResponse = REPLY_TRANSPARENT_OFF;
					osMessageQueuePut(simRespMessageHandle, (uint32_t*)&tempResponse, 0, 300);
					taskENTER_CRITICAL();
					simcom->transferMode = NOT_TRANSPARENT_MODE;
					taskEXIT_CRITICAL();
					puts ("\r\nNOT TRANSPARENT MODE\r\n");
				} else if ((simcomData.simcomRxLen == 10) && (strncmp((const char*) (simcomData.simcomRxBuf), "\r\nCLOSED\r\n", 6) == 0)){//CONNECTION CLOSED BY SERVER
					taskENTER_CRITICAL();
					simcom->transferMode = NOT_TRANSPARENT_MODE;
					simcom->serverStatus = SERVER_DISCONNECTED;
					simcom->error = CONN_CLOSED_BY_SERVER;
					wsOcppHandler.wsState = WS_HANDSHAKE_SEND;
					taskEXIT_CRITICAL();
					puts ("\r\n ----------------Connection CLOSED by server------------------\r\n");
				} else if (/*simcomData.simcomRxLen == 2 && */simcomData.simcomRxBuf[0] == 0x89) { //WEBSOCKET ping
					uartTxDataQueueSend((uint8_t*) pong, 6);
					puts ("\r\n ----------------Websocket PING------------------\r\n");
				} else if (simcomData.simcomRxBuf[0] == 0x8A) {
					puts ("\r\n ----------------Websocket PONG------------------\r\n");
				} else if (/*simcomData.simcomRxLen == 2 && */simcomData.simcomRxBuf[0] == 0x88) {//WEBSOCKET CLOSE
					puts ("\r\n ----------------Websocket CLOSED by server------------------\r\n");
				} else {
					switch (wsOcppHandler.wsState) {
					case WS_HANDSHAKE_SEND:
						break;
					case WS_HANDSHAKE_RECEIVE:
						if (ws_handshake_receive(simcomData.simcomRxBuf, simcomData.simcomRxLen)) {
							puts ("\r\n\r\n--------------Websocket connection established-----------------\r\n\r\n ");
							Ocpp_ESTABLISHED();
							taskENTER_CRITICAL();
							memset(simcomData.simcomRxBuf, 0, RX_BUFFER_SIZE);
							wsOcppHandler.wsState = WS_IDLE;
							taskEXIT_CRITICAL();
						}
						break;
					case WS_IDLE:
						ws_receive((char *)simcomData.simcomRxBuf, &wsOcppHandler);
						Ocpp_CLIENT_RECEIVE(wsOcppHandler.payload, wsOcppHandler.payload_len);
						break;
					}
				}
			}
		}
	}
}

