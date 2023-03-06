/*
 * evse.c
 *
 *  Created on: Feb 28, 2023
 *      Author: Admin
 */

#include "evse.h"
#include "utils.h"
#include "stm32f4xx_hal.h"

volatile adcInput_t adcInputData;
adcAverage_t adcAverage = {0};
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim8;

extern osMessageQueueId_t adcQHandle;
uint16_t evseVoltage[4] = {0};

uint8_t pwmPercent = 0;

void pwmOn (void) {
	__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 1000);
}

void setPwmValue (uint8_t percent) {

	uint16_t pwm = 0;

	pwm = 10 * percent;
	if (pwm >=1000) {
		HAL_TIM_Base_Start_IT(&htim8);
	} else {
		HAL_TIM_Base_Stop_IT(&htim8);
	}
	__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwm);
}

void evseTask (void const *argument) {

	adcInput_t adcData;
	for(;;) {
		setPwmValue(pwmPercent);
		if (osMessageQueueGet(adcQHandle, &adcData, 0, 50) == osOK) {
			adcAverage.adcBufHigh[0] = filtering(adcData.adcBufHigh[0], &filter[0]);
			adcAverage.adcBufHigh[1] = filtering(adcData.adcBufHigh[1], &filter[1]);
			adcAverage.adcBufHigh[2] = filtering(adcData.adcBufHigh[2], &filter[2]);
			adcAverage.adcBufHigh[3] = filtering(adcData.adcBufHigh[3], &filter[3]);
			adcAverage.adcBufLow[0] = filtering(adcData.adcBufLow[0], &filter[4]);
			adcAverage.adcBufLow[1] = filtering(adcData.adcBufLow[1], &filter[5]);
			adcAverage.adcBufLow[2] = filtering(adcData.adcBufLow[2], &filter[6]);
			adcAverage.adcBufLow[3] = filtering(adcData.adcBufLow[3], &filter[7]);
			for (uint8_t i = 0; i < 4; i++) {
				evseVoltage[i] = (adcAverage.adcBufHigh[i] * 6.041 - 11450 + 45) / 10;
			}
		}
	}
}
