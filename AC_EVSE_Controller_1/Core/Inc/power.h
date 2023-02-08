/*
 * power.h
 *
 *  Created on: Feb 7, 2023
 *      Author: Admin
 */

#include "ocpp.h"

#ifndef INC_POWER_H_
#define INC_POWER_H_



typedef struct __attribute__((__packed__)) _powerVal {
	uint16_t current;
	uint16_t voltage;
	uint32_t energy;
	uint16_t power;
}powerVal_t;


extern powerVal_t powerData;


#endif /* INC_POWER_H_ */
