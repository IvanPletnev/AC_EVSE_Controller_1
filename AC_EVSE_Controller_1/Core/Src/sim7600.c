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
extern QueueHandle_t qSimcomHandle;
extern int8_t testResponse;
extern RTC_TimeTypeDef sTime;
extern RTC_DateTypeDef sDate;
extern RTC_HandleTypeDef hrtc;

simcom_t simcomHandler = {0};

simcomRxType simcomRxData = {0};

const simcom_cmd_t cmds[] = {
/*{command						len,		timeout,		retry,      expect_reply}*/
{ "AT\r\n", 					4, 			500, 			10, 		REPLY_OK },
{ "AT+CPIN?\r\n", 				10, 		500, 			20, 		REPLY_CPIN },
{ "AT+CSQ\r\n", 				8, 			500, 			60, 		REPLY_CSQ },
{ "AT+CGREG?\r\n", 				11, 		500, 			60,			REPLY_CGREG },
{ "AT+NETOPEN\r\n", 			12, 		1000, 			1, 			REPLY_NETOPEN },
{ "AT+CIPCLOSE=0\r\n", 			15, 		500, 			1, 			REPLY_CIPCLOSE },
{ "AT+NETCLOSE\r\n", 			13, 		500, 			1, 			REPLY_NETCLOSE },
{ "AT+CFUN=0\r\n", 				11, 		5000, 			1, 			REPLY_INCFUN },
{ "AT+CFUN=1\r\n", 				11, 		3000,			1, 			REPLY_OUTCFUN },
{ "AT+CTZU?\r\n", 				10, 		500, 			1, 			REPLY_TIME_UPDATE_AUTO },
{ "AT+CTZU=1\r\n", 				11, 		500, 			1, 			REPLY_TIME_UPDATE_ENABLED },
{ "AT+CCLK?\r\n", 				10, 		500, 			1, 			REPLY_DATE_TIME }};

uint8_t dateTimeStrToINt (const uint8_t * buf) {
	uint8_t digit1 = 0;
	uint8_t digit2 = 0;

	digit1 = *buf - 0x30;
	digit1 *= 10;
	digit2 = *(buf+1) - 0x30;
	return digit1 + digit2;
}

int8_t simcomExecuteCmd(simcom_cmd_t cmd) {

	osEvent event;
	uint8_t reply = REPLY_NONE;

	do {
		HAL_UART_Transmit_DMA(&huart1, (const uint8_t *)cmd.data, cmd.len);
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
			_DEBUG(3, "CMD timed out\r\n");
		}
	} while (--cmd.retry_count);
	return reply;
}

int8_t simcomSetServer(char *server, uint16_t port, uint16_t waitMs, int8_t retryCount) {

	uint8_t startConnectString[64];
	uint8_t reply = REPLY_NONE;
	uint16_t len = sprintf((char*) startConnectString, "AT+CIPOPEN=0,\"TCP\",\"%s\",%d\r\n", server, port);
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
		puts ("NET IS OPEN");
	}
	return reply;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {

	BaseType_t xHigherPriorityTaskWoken = 0;

	if (huart->Instance == USART1) {
		simcomRxData.simcomRxLen = Size;
		if (xQueueSendFromISR(qSimcomHandle, &simcomRxData, &xHigherPriorityTaskWoken) != pdTRUE) {
		}
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
			puts ("Parcer: REPLY_OK\r\n");
		}

		else if (Len >= 7 && Len < 66 && strncmp((const char*)(buffer + Len - 7), "ERROR", 5) == 0)
						{
			reply = REPLY_ERROR;
			puts ("Parcer: REPLY_ERROR\r\n");
		}

		///CPIN
		if (Len >= 31 && strncmp((const char*)(buffer + Len - 20), "+CPIN:", 6) == 0) {
			if (strstr((const char*)buffer, "READY") != NULL) {
				reply = REPLY_CPIN;
				puts ("Parcer: REPLY_CPIN\r\n");
			}
			else {
				reply = REPLY_ERROR;
			}
			_DEBUG(2, "--- CPIN --- \r\n");
		}

		///CSQ
		if (Len >= 27 && strncmp((const char*)(buffer + Len - 19), "+CSQ:", 5) == 0) {
			reply = REPLY_CSQ;
			puts ("Parcer: REPLY_CSQ\r\n");
			_DEBUG(2, "--- CSQ --- %s", buffer);
		}

		///CGREG
		if (Len >= 31 && strncmp((const char*)(buffer + Len - 19), "+CGREG:", 7) == 0) {
			if (strstr((const char*)buffer, "0,1") != NULL || strstr((const char*)buffer, "0,5") != NULL) {
				reply = REPLY_CGREG;
				puts ("Parcer: REPLY_CGREG\r\n");
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


		///CFUN
		if ( strstr((const char *) buffer, "+SIMCARD: NOT AVAILABLE")!=NULL )
		{
			reply = REPLY_INCFUN;
		}

		else if ( strstr((const char *)buffer, "PB DONE")!=NULL )
		{
			reply = REPLY_OUTCFUN;
		}

		///CTZU
		if (Len >= 27 && strstr ((const char*)buffer, "+CTZU: ") != NULL) {
			if (buffer[Len - 9] == '1') {
				reply = REPLY_TIME_UPDATE_AUTO;
			} else {
				reply = REPLY_NO_TIME_UPDATE;
			}
		}

		///CTZU=1
		if (strcmp ((const char*)buffer, "AT+CTZU=1") == 0 && strncmp((const char*)(buffer + Len - 4), "OK", 2) == 0) {
			reply = REPLY_TIME_UPDATE_ENABLED;
		}

		///CCLK

		if (strstr ((const char *)buffer, "+CCLK: ") != NULL && strncmp((const char*)(buffer + Len - 4), "OK", 2) == 0) {

			sDate.Year = dateTimeStrToINt (buffer + Len - 29);
			sDate.Month = dateTimeStrToINt(buffer + Len - 29 + 3);
			sDate.Date = dateTimeStrToINt(buffer + Len - 29 + 6);
			sTime.Hours = dateTimeStrToINt(buffer + Len - 29 + 9);
			sTime.Minutes = dateTimeStrToINt(buffer + Len - 29 +12);
			sTime.Seconds = dateTimeStrToINt(buffer + Len - 29 + 15);

			reply = REPLY_DATE_TIME;
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
		/// +IPCLOSE
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

	switch (simcomHandler.initStatus) {
	case NOT_INITIALIZED:
		testResponse = simcomExecuteCmd(cmds[AT]);
		if (testResponse != REPLY_OK) {
			puts ("CMD AT FAILED\r\n");
			return INIT_SIM_FAIL;
		}
		osDelay(100);
		printf ("Response: %d\r\n", testResponse);

		testResponse = simcomExecuteCmd(cmds[CTZU]);
		if (testResponse == REPLY_NO_TIME_UPDATE) {
			osDelay(100);
			testResponse = simcomExecuteCmd(cmds[CTZU_ENABLE]);
			osDelay(100);
			testResponse = simcomExecuteCmd(cmds[INCFUN]);
			if (testResponse == REPLY_INCFUN) {
				osDelay(6000);
				testResponse = simcomExecuteCmd(cmds[OUTCFUN]);
			}
		}
		osDelay(100);

		testResponse = simcomExecuteCmd(cmds[CCLK]);
		if (testResponse == REPLY_DATE_TIME) {
			HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
			HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
			osDelay(100);
		}

		HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

		testResponse = simcomExecuteCmd(cmds[CHECK_SIM]);
		if (testResponse != REPLY_CPIN) {
			puts ("CMD CPIN FAILED\r\n");
			return INIT_SIM_FAIL;
		}
		osDelay(100);
		printf ("Response: %d\r\n", testResponse);

		testResponse = simcomExecuteCmd(cmds[CSQ]);
		if (testResponse != REPLY_CSQ) {
			puts ("CMD CSQ FAILED\r\n");
			return INIT_SIM_FAIL;
		}
		osDelay(100);
		printf ("Response: %d\r\n", testResponse);

		testResponse = simcomExecuteCmd(cmds[CGREG]);
		if (testResponse != REPLY_CGREG) {
			puts ("CMD CGREG FAILED\r\n");
			return INIT_SIM_FAIL;
		}
		osDelay(100);
		printf ("Response: %d\r\n", testResponse);

		testResponse = simcomOpenNet();
		if (testResponse != REPLY_NETOPEN) {
			puts ("CMD NETOPEN FAILED\r\n");
//			return INIT_SIM_FAIL;
		} else {
			puts ("CMD NETOPEN DONE\r\n");
		}
		osDelay(100);
		printf ("Response: %d\r\n", testResponse);

		testResponse = simcomSetServer(SERVER_ADDR, SERVER_PORT, 500, 5);
		if (testResponse != REPLY_CIPOPEN) {
			puts ("CIPOPEN FAILED\r\n");
//			return INIT_SIM_FAIL;
		} else {
			puts ("----------Connected to server-------------\r\n");
		}
		osDelay(100);
		printf ("Response: %d\r\n", testResponse);



//		puts ("\r\nSIM7600 Initialization done\r\n");

		simcomHandler.initStatus = INITIALIZED;
		break;
	case INITIALIZED:
		break;
	}
	return INIT_SIM_DONE;
}


void parcerTask(void const * argument){

	simcomRxType simcomData = {0};
	uint8_t tempResponse = 0;

	for (;;){
		if (xQueueReceive(qSimcomHandle, (simcomRxType*) &simcomData, osWaitForever) == pdPASS) {

			tempResponse = simcomParcer(simcomData.simcomRxBuf, simcomData.simcomRxLen);
			if (tempResponse != 0) {
				osMessagePut(simRespMessageHandle, (uint32_t) tempResponse, 100);
			}
		}
	}
}

