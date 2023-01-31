/*
 * MIT License
 *
 * Copyright (c) 2022 Sergey Kostyanoy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "ocpp_defines.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute__((__packed__)) {
	char c[20];
} c_string20_t;

typedef struct __attribute__((__packed__)) {
	char c[25];
} c_string25_t;

typedef struct __attribute__((__packed__)) {
	char c[50];
} c_string50_t;

typedef struct __attribute__((__packed__)) {
	char c[255];
} c_string255_t;

typedef struct __attribute__((__packed__)) {
    char c[37];
} c_id_string37_t;

// UUID structure defined as mentioned in RFC4122
struct uuid_RFC4122 {
    uint32_t     time_low;
    uint16_t     time_mid;
    uint16_t     time_hi_and_version;
    uint16_t     clock_seq;
    uint8_t      node[6];
};

typedef struct __attribute__((__packed__)) _uartTxData {
	uint8_t buffer[1024];
	uint16_t len;
	uint8_t source;
}uartTxData_t;

typedef uint8_t uuid_bytes_t[16];

typedef enum _uartStatus {
	READY,
	BUSY
}uartStatus_t;

typedef enum ChargePointStatus{ 	//Total 9
	Available,						//0
	Preparing,						//1
	Charging,						//2
	SuspendedEV,					//3
	SuspendedEVSE,					//4
	Finishing,						//5
	Reserved,						//6
	Unavailable,					//7
	Faulted							//8
}ChargePointStatus_t;

typedef enum ChargePointErrorCode{	//Total 16
	ConnectorLockFailure,			//0
	EVCommunicationError,			//1
	GroundFailure,					//2
	HighTemperature,				//3
	InternalError,					//4
	LocalListConflict,				//5
	NoError,						//6
	OtherError,						//7
	OverCurrentFailure,				//8
	OverVoltage,					//9
	PowerMeterFailure,				//10
	PowerSwitchFailure,				//11
	ReaderFailure,					//12
	ResetFailure,					//13
	UnderVoltage,					//14
	WeakSignal						//15
}ChargePointErrorCode_t;

typedef enum AvailabilityStatus {
	Accepted,
	Rejected,
	Scheduled
}AvailabilityStatus_t;

typedef enum UnlockStatus {
	Unlocked,
	UnlockFailed,
	NotSupported
}UnlockStatus_t;


typedef enum {
    CP_Unknown,		//0
    CP_Accepted,	//1
    CP_Pending,		//2
    CP_Rejected		//3
} ChargePoint_RegistrationStatus_t;

typedef enum {
    Auth_Unknown,		//0
    Auth_Accepted,		//1
    Auth_Blocked,		//2
    Auth_Expired,		//3
    Auth_Invalid,		//4
    Auth_ConcurrentTx	//5
} Tag_AuthorizationStatus_t;

typedef struct _TransactionData {
	uint32_t transactionId;
	uint8_t connectorId;
	uint8_t idTagString[20];
}TransactionData_t;

//typedef enum TriggerMessageStatus {
//	Accepted,
//	Rejected,
//	NotImplemented
//}TriggerMessageStatus_t;
