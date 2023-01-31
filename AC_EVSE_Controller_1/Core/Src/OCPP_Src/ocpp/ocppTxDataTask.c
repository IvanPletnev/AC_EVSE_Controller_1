
#include "main.h"
#include "ocpp.h"
#include "ocppTxDataTask.h"
#include "cmsis_os2.h"


extern UART_HandleTypeDef huart1;
extern osMessageQueueId_t uartTxHandle;

uint8_t uartTxDataQueueSend (uint8_t * buf, uint16_t size) {

	uartTxData_t data = {0};
	taskENTER_CRITICAL();
	memcpy((uint8_t*) data.buffer, (const uint8_t *) buf, size);
	data.len = size;
	taskEXIT_CRITICAL();

	if (osMessageQueuePut(uartTxHandle, &data , 0, 500) == osOK){
		return 1;
	} else {
		return 0;
	}
}

void ocppTxTask(void const * argument){

	uartTxData_t data = {0};

	for (;;){
		if (uart1Status == READY) {
			if (osMessageQueueGet(uartTxHandle, (uartTxData_t *) &data, 0, osWaitForever) == osOK){
				HAL_UART_Transmit_DMA(&huart1, (const uint8_t *)& data.buffer, data.len);
				uart1Status = BUSY;
			}
		} else {
			osDelay(1);
		}
	}

}

void ocppTask(void *argument){

	osDelay(1000);
	simcomInit();
	osDelay(100);
	ws_handshake_request_handler();
	wsOcppHandler.wsState = WS_HANDSHAKE_RECEIVE;

	for (;;) {

		if (wsOcppHandler.wsState == WS_IDLE) {
			Ocpp_IDLE();
			if (startTrans) {
				startTrans = 0;
				ocpp_task |= (1 << task_StartTransaction);
			}
			if (startAuth) {
				startAuth = 0;
				ocpp_task |= (1 << task_Authorize);
			}
		} else {
			osDelay(20);
		}
	}
}
