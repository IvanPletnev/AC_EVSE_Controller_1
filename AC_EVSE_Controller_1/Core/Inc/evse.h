/*
 * evse.h
 *
 *  Created on: Feb 28, 2023
 *      Author: Admin
 */

#ifndef INC_EVSE_H_
#define INC_EVSE_H_
#include "main.h"
#include "cmsis_os.h"

#define ADC_BUF_SIZE		4
#define ADC_SAMPLE_PERIOD	9

typedef enum _pwmState {
	PWM_ENABLE,
	PWM_DISABLE
}pwmState_t;

typedef struct _adcInputStruct {
	uint16_t adcBufLow[ADC_BUF_SIZE];
	uint16_t adcBufHigh[ADC_BUF_SIZE];
	pwmState_t pwmState;
}adcInput_t;

typedef struct _adcAverage {
	int16_t adcBufLow [4];
	int16_t adcBufHigh[4];
}adcAverage_t;

extern volatile adcInput_t adcInputData;

#endif /* INC_EVSE_H_ */
