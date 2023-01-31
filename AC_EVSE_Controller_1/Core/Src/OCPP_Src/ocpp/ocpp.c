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

#include "../port/port.h"
#include "../utils/jsmn.h"
#include "ocpp_types.h"
#include "ocpp_strings.h"
#include "ocpp_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ocpp.h"

volatile uartStatus_t uart1Status = READY;
volatile ChargePointStatus_t cpStatus = Available;
volatile ChargePointStatus_t connectorStatus1 = Available;
volatile ChargePointStatus_t connectorStatus2 = Available;

volatile ChargePointStatus_t connectorStatus1Last = Available;
volatile ChargePointStatus_t connectorStatus2Last = Available;

volatile ChargePointErrorCode_t cpErrorCode = NoError;
volatile uint8_t connectorID = 0;
volatile uint8_t *ocppRequestId;
volatile AvailabilityStatus_t availStatus = Accepted;
volatile uint8_t idTagString[20];
//volatile TriggerMessageStatus_t trigMessageStatus = Accepted;

extern UART_HandleTypeDef huart1;
extern osMessageQueueId_t uartTxHandle;

volatile uint8_t startTrans = 0;
volatile uint8_t startAuth = 0;

volatile TransactionData_t transactionData[3] = {0};

void setConnectorStatus (ChargePointStatus_t status, uint8_t connector) {
	switch (connector) {
	case 0:
		connectorStatus1 = connectorStatus2 = status;
		cpStatus = status;
		break;
	case 1:
		connectorStatus1 = status;
		break;
	case 2:
		connectorStatus2 = status;
		break;
	default:
		connectorStatus1 = Unavailable;
		connectorStatus2 = Unavailable;
	}
	cpStatus = status;
}

ChargePointStatus_t getChargePointStatus (void) {
	return cpStatus;
}

ChargePointStatus_t getConnectorStatus (uint8_t Connector) {

	switch (Connector) {
	case 0:
		return cpStatus;
	case 1:
		return connectorStatus1;
	case 2:
		return connectorStatus2;
	}
	return Unavailable;
}

void setChargePointErrorCode (ChargePointErrorCode_t error){
	cpErrorCode = error;
}

ChargePointErrorCode_t getChargePointErrorCode (void) {
	return cpErrorCode;
}

void setCurrentConnector (uint8_t connector) {
	connectorID = connector;
}

uint8_t getCurrentConnector (void) {
	return connectorID;
}

void set_zero(const uint8_t *js, jsmntok_t *tok) {
    *((uint8_t *)js + tok->end - tok->start + tok->start) = '\0';
}

void Ocpp_CLIENT_CONNECTION_ERROR() {
    printf("\nOCPP_CLIENT_CONNECTION_ERROR\n");
}

void setAvailabilityStatus (AvailabilityStatus_t status) {
	availStatus = status;
}

AvailabilityStatus_t getAvailabilityStatus (void) {
	return availStatus;
}

// Authorize
// BootNotification
// DataTransfer
// DiagnosticsStatusNotification
// FirmwareStatusNotification
// Heartbeat
// MeterValues
// StartTransaction
// StatusNotification
// StopTransaction

void CancelReservation(OCPP_CALL_ARGS) {
    if (call == ocpp_call_conf) {
        printf("\nconf CancelReservation\n");
    }
}


// [2,"f7d37671-a7cf-488c-a449-657b83af0c7f","ChangeAvailability",{"connectorId":0,"type":"Operative"}]
// [2,"9cf6a258-b5a8-46db-a28e-603a102a9669","ChangeAvailability",{"connectorId":0,"type":"Inoperative"}]
void ChangeAvailability(OCPP_CALL_ARGS) {

	uint8_t tempBuf[128];
	int len = 0;
	AvailabilityStatus_t aStatus;
	if (call == ocpp_call_conf) {

		for (uint8_t ax = 0; ax < tok[4].size; ++ax) {

			uint8_t ikey = ax * 2 + 5;
			uint8_t ival = ikey + 1;
			set_zero(buf, &tok[ikey]);
			set_zero(buf, &tok[ival]);
			const uint8_t *key_ptr = buf + tok[ikey].start;
			const uint8_t *val_ptr = buf + tok[ival].start;

			if (tok[ikey].type == JSMN_STRING
					&& tok[ival].type == JSMN_PRIMITIVE) {

				if (strcmp((const char*) key_ptr, "connectorId") == 0) {
					connectorID = atoi((const char*) val_ptr);
					printf("**connector_id = %d\r\n", connectorID);
				}
			}
			if (tok[ikey].type == JSMN_STRING
					&& tok[ival].type == JSMN_STRING) {
				if (strcmp((const char*) key_ptr, "type") == 0) {
					if (strcmp((const char*) val_ptr, "Inoperative") == 0) {
						setConnectorStatus(Unavailable, connectorID);
					}
					if (strcmp((const char*) val_ptr, "Operative") == 0) {
						setConnectorStatus(Available, connectorID);
					}
				}
			}
		}
		ringBufferInit(&ringBuffer);
		memset((uint8_t*) &ringBuffer.buffer, 0, RINGBUFFER_SIZE);
		len = sprintf((char*) tempBuf, PSTR_Response_Head_simple,
				(const char*) ocppRequestId);
		put(&ringBuffer, (const uint8_t*) tempBuf, len);
//    	put (&ringBuffer, (const uint8_t *) PSTR_ChangeAvailability, strlen(PSTR_ChangeAvailability));
		put(&ringBuffer, (const uint8_t*) PSTR_Obj_Start_Simple,
				strlen(PSTR_Obj_Start_Simple));
		aStatus = getAvailabilityStatus();
		len = sprintf((char*) tempBuf, "\"status\":\"%s\"",
				AvailabilityStatuses[aStatus]);
		put(&ringBuffer, (const uint8_t*) tempBuf, len);
		put(&ringBuffer, (const uint8_t*) PSTR_Obj_End, strlen(PSTR_Obj_End));
//		puts((const char*) ringBuffer.buffer);
//		puts("\r\n");

		ws_send((char*) ringBuffer.buffer);
		osDelay(10);
		ocpp_task |= (1 << task_StatusNotification);

//		printf("\nconf ChangeAvailability\n");
	}
}

// [2,"ef39ae51-f802-4ab4-823a-2d9c301052f9","ChangeConfiguration",{"key":"AllowOfflineTxForUnknownId","value":""}]
// [2,"9e2b9cda-5baa-4cde-840b-2c310eac273d","ChangeConfiguration",{"key":"CustomConfigurationKey","value":"Value"}]
void ChangeConfiguration(OCPP_CALL_ARGS) {
    if (call == ocpp_call_conf) {
        printf("\nconf ChangeConfiguration\n");
    }
}

// [2,"f57c8d7e-cf6b-4f68-8797-88b2f34cd687","ClearCache",{}]
void ClearCache(OCPP_CALL_ARGS) {
    if (call == ocpp_call_conf) {
        printf("handler ClearCache\n");
    }
}

// [2,"1a9321ca-5b2a-4a43-a308-03c9f27c9664","ClearChargingProfile",{"stackLevel":0}]
void ClearChargingProfile(OCPP_CALL_ARGS) {
    if (call == ocpp_call_conf) {
        printf("\nconf ClearChargingProfile\n");
    }
}

// [2,"feeed34d-0943-4e9a-bc27-d3bc326e0d24","DataTransfer",{"vendorId":"Vendor ID","messageId":"Message ID","data":"Data TEXT"}]
void DataTransfer(OCPP_CALL_ARGS) {
    if (call == ocpp_call_conf) {
        printf("\nconf DataTransfer\n");
    }
}

// [2,"a842a770-4c98-46bb-8994-9ca9f4f73ee9","GetCompositeSchedule",{"connectorId":0,"duration":123,"chargingRateUnit":"W"}]
// [2,"796618b4-3435-4fb1-918a-426fec5d1df5","GetCompositeSchedule",{"connectorId":0,"duration":789,"chargingRateUnit":"A"}]
void GetCompositeSchedule(OCPP_CALL_ARGS) {
    if (call == ocpp_call_conf) {
        printf("\nconf GetCompositeSchedule\n");
    }
}

// [2,"1324400b-28ff-40ac-99ee-f57af8ad725f","GetConfiguration",{"key":["BlinkRepeat"]}]
// [2,"7d589fb2-a71f-42a6-ad64-12e3eb94a010","GetConfiguration",{"key":[
//     "AllowOfflineTxForUnknownId",   "AuthorizationCacheEnabled",
//     "AuthorizeRemoteTxRequests",    "BlinkRepeat",
//     "ChargeProfileMaxStackLevel",   "ChargingScheduleAllowedChargingRateUnit",
//     "ChargingScheduleMaxPeriods",   "ClockAlignedDataInterval",
//     "ConnectionTimeOut",            "ConnectorPhaseRotation",
//     "ConnectorPhaseRotationMaxLength",  "ConnectorSwitch3to1PhaseSupported",
//     "GetConfigurationMaxKeys",      "HeartbeatInterval",
//     "LightIntensity",   "LocalAuthListEnabled", "LocalAuthListMaxLength",
//     "LocalAuthorizeOffline",    "LocalPreAuthorize",    "MaxChargingProfilesInstalled",
//     "MaxEnergyOnInvalidId",     "MeterValueSampleInterval", "MeterValuesAlignedData",
//     "MeterValuesAlignedDataMaxLength",  "MeterValuesSampledData",   "MeterValuesSampledDataMaxLength",
//     "MeterValuesSignatureContexts", "MinimumStatusDuration",    "NumberOfConnectors",
//     "ReserveConnectorZeroSupported",    "ResetRetries", "SendLocalListMaxLength",
//     "StopTransactionOnEVSideDisconnect",    "StopTransactionOnInvalidId",
//     "StopTransactionSignatureContexts", "StopTransactionSignatureFormat",
//     "StopTxnAlignedData",   "StopTxnAlignedDataMaxLength",
//     "StopTxnSampledData",   "StopTxnSampled",   "MaxChargingProfilesInstalled",
//     "MaxDataMaxLength",     "SupportedFeatureProfiles", "SupportedFeatureProfilesMaxLength",
//     "SupportedFileTransferProtocols",   "TransactionMessageAttempts",
//     "TransactionMessageRetryInterval",  "UnlockConnectorOnEVSideDisconnect",
//     "WebSocketPingInterval",    "CUSTOM_KEY"
// ]}]
void GetConfiguration(OCPP_CALL_ARGS) {
    if (call == ocpp_call_conf) {
        printf("\nconf GetConfiguration\n");
    }
}

void GetDiagnostics(OCPP_CALL_ARGS) {
    if (call == ocpp_call_conf) {
        printf("\nconf GetDiagnostics\n");
    }
}

// [2,"42966b41-c6ab-4523-a659-4c239ee63eb5","GetLocalListVersion",{}]
void GetLocalListVersion(OCPP_CALL_ARGS) {
    if (call == ocpp_call_conf) {
        printf("\nconf GetLocalListVersion\n");
    }
}

// [2,"da8ed36a-554d-4e5a-ae62-678ef464cc6b","RemoteStartTransaction",{"idTag":"B4A63CDF"}]
void RemoteStartTransaction(OCPP_CALL_ARGS) {

    if (call == ocpp_call_conf) {
    	uint8_t tempBuf[128];
    	int len = 0;

		for (uint8_t ax = 0; ax < tok[4].size; ++ax) {

			uint8_t ikey = ax * 2 + 5;
			uint8_t ival = ikey + 1;
			set_zero(buf, &tok[ikey]);
			set_zero(buf, &tok[ival]);
			const uint8_t *key_ptr = buf + tok[ikey].start;
			const uint8_t *val_ptr = buf + tok[ival].start;

			if (tok[ikey].type == JSMN_STRING && tok[ival].type == JSMN_PRIMITIVE) {

				if (strcmp((const char*) key_ptr, "connectorId") == 0) {
					connectorID = atoi((const char*) val_ptr);
					transactionData[connectorID].connectorId = connectorID;
					printf("**connector_id = %d\r\n", connectorID);
				}
			}

			if (tok[ikey].type == JSMN_STRING && tok[ival].type == JSMN_STRING) {

				if (strcmp((const char*) key_ptr, "idTag") == 0) {
					strcpy((char *) transactionData[connectorID].idTagString, (const char *) val_ptr);
				}
			}
		}

		ringBufferInit(&ringBuffer);
		memset((uint8_t*) &ringBuffer.buffer, 0, RINGBUFFER_SIZE);
		len = sprintf((char*) tempBuf, PSTR_Response_Head_simple, (const char*) ocppRequestId);
		put(&ringBuffer, (const uint8_t*) tempBuf, len);
		put(&ringBuffer, (const uint8_t*) PSTR_Obj_Start_Simple, strlen(PSTR_Obj_Start_Simple));
		len = sprintf((char*) tempBuf, "\"status\":\"%s\"", "Accepted");
		put(&ringBuffer, (const uint8_t*) tempBuf, len);
		put(&ringBuffer, (const uint8_t*) PSTR_Obj_End, strlen(PSTR_Obj_End));
		puts((const char*) ringBuffer.buffer);
		puts("\r\n");

		ws_send((char*) ringBuffer.buffer);
		ocpp_task |= (1 << task_StartTransaction);
		osDelay(10);
        printf("\nconf RemoteStartTransaction\n");

    }
}

void RemoteStopTransaction(OCPP_CALL_ARGS) {

    if (call == ocpp_call_conf) {
    	uint8_t tempBuf[128];
    	int len = 0;
    	uint32_t transId = 0;
    	uint8_t i = 0;

		uint8_t ikey = 5;
		uint8_t ival = ikey + 1;
		set_zero(buf, &tok[ikey]);
		set_zero(buf, &tok[ival]);
		const uint8_t *key_ptr = buf + tok[ikey].start;
		const uint8_t *val_ptr = buf + tok[ival].start;

		if (tok[ikey].type == JSMN_STRING && tok[ival].type == JSMN_PRIMITIVE) {

			if (strcmp((const char*) key_ptr, "transactionId") == 0) {
				transId = atol((const char*) val_ptr);
				for (i = 1; i < 3; i++) {
					if (transId == transactionData[i].transactionId) {
						setCurrentConnector(i);
						break;
					}
				}
			}
		}
		ringBufferInit(&ringBuffer);
		memset((uint8_t*) &ringBuffer.buffer, 0, RINGBUFFER_SIZE);
		len = sprintf((char*) tempBuf, PSTR_Response_Head_simple, (const char*) ocppRequestId);
		put(&ringBuffer, (const uint8_t*) tempBuf, len);
		put(&ringBuffer, (const uint8_t*) PSTR_Obj_Start_Simple, strlen(PSTR_Obj_Start_Simple));
		len = sprintf((char*) tempBuf, "\"status\":\"%s\"", "Accepted");
		put(&ringBuffer, (const uint8_t*) tempBuf, len);
		put(&ringBuffer, (const uint8_t*) PSTR_Obj_End, strlen(PSTR_Obj_End));
		puts((const char*) ringBuffer.buffer);
		puts("\r\n");

		ws_send((char*) ringBuffer.buffer);
		ocpp_task |= (1 << task_StopTransaction);
		osDelay(10);

        printf("\nconf RemoteStopTransaction\n");
    }
}

void ReserveNow(OCPP_CALL_ARGS) {
    if (call == ocpp_call_conf) {
        printf("\nconf ReserveNow\n");
    }
}

// [2,"e481beb2-6560-4243-8d17-9b372792ef71","Reset",{"type":"Hard"}]
// [2,"bbe4e5e0-b75d-42e1-a394-9f4e3edb5312","Reset",{"type":"Soft"}]
void Reset(OCPP_CALL_ARGS) {

	uint8_t tempBuf[128];
	int len = 0;

    if (call == ocpp_call_conf) {

		for (uint8_t ax = 0; ax < tok[4].size; ++ax) {

			uint8_t ikey = ax * 2 + 5;
			uint8_t ival = ikey + 1;
			set_zero(buf, &tok[ikey]);
			set_zero(buf, &tok[ival]);
			const uint8_t *key_ptr = buf + tok[ikey].start;
			const uint8_t *val_ptr = buf + tok[ival].start;

			if (tok[ikey].type == JSMN_STRING
					&& tok[ival].type == JSMN_STRING) {

				if (strcmp((const char*) key_ptr, "type") == 0) {
					if (strcmp((const char*) val_ptr, "Hard") == 0){
						puts ("HARD reset \r\n");
					} else if (strcmp((const char*) val_ptr, "Soft") == 0){
						puts ("SOFT reset \r\n");
					}
				}
			}
		}

		ringBufferInit(&ringBuffer);
		memset((uint8_t*) &ringBuffer.buffer, 0, RINGBUFFER_SIZE);
		len = sprintf((char*) tempBuf, PSTR_Response_Head_simple, (const char*) ocppRequestId);
		put(&ringBuffer, (const uint8_t*) tempBuf, len);
		put(&ringBuffer, (const uint8_t*) PSTR_Obj_Start_Simple, strlen(PSTR_Obj_Start_Simple));
		len = sprintf((char*) tempBuf, "\"status\":\"%s\"", "Accepted");
		put(&ringBuffer, (const uint8_t*) tempBuf, len);
		put(&ringBuffer, (const uint8_t*) PSTR_Obj_End, strlen(PSTR_Obj_End));
//		puts((const char*) ringBuffer.buffer);
//		puts("\r\n");

		ws_send((char*) ringBuffer.buffer);
		osDelay(10);
        printf("\nconf Reset\n");
    }
}

// [2,"8cc74b49-932c-4b62-9a93-f4cc77644d25","SendLocalList",{"listVersion":0,"localAuthorizationList":[],"updateType":"Full"}]
void SendLocalList(OCPP_CALL_ARGS) {
    if (call == ocpp_call_conf) {
        printf("\nconf SendLocalList\n");
    }
}

void SetChargingProfile(OCPP_CALL_ARGS) {
    if (call == ocpp_call_conf) {
        printf("\nconf SetChargingProfile\n");
    }
}

// [2,"0655d76a-e575-446c-865f-b1f822b139e8","TriggerMessage",{"requestedMessage":"MeterValues"}]
// [2,"51a74dd4-5c43-4e36-94b8-f4bdae7529a2","TriggerMessage",{"requestedMessage":"FirmwareStatusNotification","connectorId":1}]
void TriggerMessage(OCPP_CALL_ARGS) {

    if (call == ocpp_call_conf) {

		for (uint8_t ax = 0; ax < tok[4].size; ++ax) {

			uint8_t ikey = ax * 2 + 5;
			uint8_t ival = ikey + 1;
			set_zero(buf, &tok[ikey]);
			set_zero(buf, &tok[ival]);
			const uint8_t *key_ptr = buf + tok[ikey].start;
			const uint8_t *val_ptr = buf + tok[ival].start;

			if (tok[ikey].type == JSMN_STRING
					&& tok[ival].type == JSMN_STRING) {

				if (strcmp((const char*) key_ptr, "requestedMessage") == 0) {

                    if (strcmp((const char *)key_ptr, "BootNotification") == 0) {
                        ocpp_task |= (1 << task_BootNotification);
                    } else if (strcmp((const char *)val_ptr, "StatuSNotification") == 0) {
                    	ocpp_task |= (1 << task_StatusNotification);
					} else if (strcmp((const char *)val_ptr, "MeterValues") == 0) {
						ocpp_task |= (1 << task_MeterValues);
					} else if (strcmp((const char *)val_ptr, "DiagnosticsStatusNotification") == 0) {
						ocpp_task |= (1 << task_DiagnosticsStatusNotification);
					} else if (strcmp((const char *)val_ptr, "FirmwareStatusNotification") == 0) {
						ocpp_task |= (1 << task_DiagnosticsStatusNotification);
					}
                        printf ("Requested message = [%s]\r\n", val_ptr);

				} else if (strcmp((const char*) key_ptr, "connectorId") == 0) {
					connectorID = atoi((const char*) val_ptr);
					printf("**connector_id = %d\r\n", connectorID);
				}
			}
		}
         printf("\nconf TriggerMessage\n");
    }
}

void UnlockConnector(OCPP_CALL_ARGS) {
	uint8_t tempBuf[128];
	int len = 0;

    if (call == ocpp_call_conf) {

		for (uint8_t ax = 0; ax < tok[4].size; ++ax) {

			uint8_t ikey = ax * 2 + 5;
			uint8_t ival = ikey + 1;
			set_zero(buf, &tok[ikey]);
			set_zero(buf, &tok[ival]);
			const uint8_t *key_ptr = buf + tok[ikey].start;
			const uint8_t *val_ptr = buf + tok[ival].start;

			if (tok[ikey].type == JSMN_STRING
					&& tok[ival].type == JSMN_PRIMITIVE) {

				if (strcmp((const char*) key_ptr, "connectorId") == 0) {
					connectorID = atoi((const char*) val_ptr);
					printf("**connector_id = %d\r\n", connectorID);
				}
			}
		}

		ringBufferInit(&ringBuffer);
		memset((uint8_t*) &ringBuffer.buffer, 0, RINGBUFFER_SIZE);
		len = sprintf((char*) tempBuf, PSTR_Response_Head_simple, (const char*) ocppRequestId);
		put(&ringBuffer, (const uint8_t*) tempBuf, len);
		put(&ringBuffer, (const uint8_t*) PSTR_Obj_Start_Simple, strlen(PSTR_Obj_Start_Simple));
		len = sprintf((char*) tempBuf, "\"status\":\"%s\"", UnlockStatuses[Unlocked]);
		put(&ringBuffer, (const uint8_t*) tempBuf, len);
		put(&ringBuffer, (const uint8_t*) PSTR_Obj_End, strlen(PSTR_Obj_End));

		ws_send((char*) ringBuffer.buffer);

		osDelay(10);
//        printf("\nconf UnlockConnector\n");
    }
}

void UpdateFirmware(OCPP_CALL_ARGS) {
    LOGGING( printf("\nUpdateFirmware"); )
    if (call == ocpp_call_conf) {
         printf(".conf\n");
    }
    else if (call == ocpp_call_reply) {
        printf("[reply]\n");
    }

}

// ------------------------------------------------------------------------
// Authorize
// BootNotification
// DataTransfer
// DiagnosticsStatusNotification
// FirmwareStatusNotification
// Heartbeat
// MeterValues
// StartTransaction
// StatusNotification
// StopTransaction

void DiagnosticsStatusNotification(OCPP_CALL_ARGS) {
    LOGGING( printf("\nDiagnosticsStatusNotification\n"); )
    if (call == ocpp_call_req) {
        
    } else if (call == ocpp_call_reply) {
        
    }
}

void FirmwareStatusNotification(OCPP_CALL_ARGS) {
    LOGGING( printf("\nFirmwareStatusNotification\n"); )
    if (call == ocpp_call_req) {
        
    } else if (call == ocpp_call_reply) {
        
    }
}

void MeterValues(OCPP_CALL_ARGS) {
    LOGGING( printf("\nMeterValues\n"); )
    if (call == ocpp_call_conf) {
    }
}

//-------------------------------------------------------------------------

// ocpp_conf_pair_t procs[] = {
//     { PSTR_CancelReservation,      CancelReservation },
//     { PSTR_ChangeAvailability,     ChangeAvailability },
//     { PSTR_ChangeConfiguration,    ChangeConfiguration },
//     { PSTR_ClearCache,             ClearCache },
//     { PSTR_ClearChargingProfile,   ClearChargingProfile },
//     { PSTR_DataTransfer,           DataTransfer },
//     { PSTR_GetCompositeSchedule,   GetCompositeSchedule },
//     { PSTR_GetConfiguration,       GetConfiguration },
//     { PSTR_GetDiagnostics,         GetDiagnostics },
//     { PSTR_GetLocalListVersion,    GetLocalListVersion },
//     { PSTR_RemoteStartTransaction, RemoteStartTransaction },
//     { PSTR_RemoteStopTransaction,  RemoteStopTransaction },
//     { PSTR_ReserveNow,             ReserveNow },
//     { PSTR_Reset,                  Reset },
//     { PSTR_SendLocalList,          SendLocalList },
//     { PSTR_SetChargingProfile,     SetChargingProfile },
//     { PSTR_TriggerMessage,         TriggerMessage },
//     { PSTR_UnlockConnector,        UnlockConnector },
//     { PSTR_UpdateFirmware,         UpdateFirmware },
//     // -----------------------------------------------
//     { PSTR_Authorize               , Authorize },
//     { PSTR_BootNotification        , BootNotification },
//     { PSTR_DiagnosticsStatusNotification ,DiagnosticsStatusNotification },
//     { PSTR_FirmwareStatusNotification    ,FirmwareStatusNotification },
//     { PSTR_Heartbeat               , Heartbeat },
//     { PSTR_MeterValues             , MeterValues },
//     { PSTR_StartTransaction        , StartTransaction },
//     { PSTR_StatusNotification      , StatusNotification },
//     { PSTR_StopTransaction         , StopTransaction },
//     {0, 0}
// };

// [4,"7b437670-e231-4a82-bab7-b6042bf2d896","FormationViolation","The payload for action could not be deserialized",{"errorMsg":"Cannot deserialize value of type `java.util.ArrayList<ocpp.cs._2015._10.MeterValue>` from Object val..."}]

void Ocpp_CLIENT_RECEIVE(const uint8_t *buf, const uint16_t len) {
//    LOGGING(
        printf("\r\nOcpp_CLIENT_RECEIVE(%u)\r\n", len);
        printf("%.*s\r\n\r\n", len, buf);
//    )

//    HexDump(buf, len);
    
    jsmn_init(&p);
    int r = jsmn_parse(&p, (const char *) buf, len, tok, TOKENS);

    if (r < 0) {
        printf("JSON_ERROR\n");
    } else {

        if (tok[0].type == JSMN_ARRAY) {

            int8_t      Call_Type   = 0;

            if (tok[1].type == JSMN_PRIMITIVE) {
                set_zero(buf, &tok[1]);
                Call_Type = atoi((const char *)buf + tok[1].start);
            }

            const uint8_t *Call_ID     = NULL;
            if (tok[2].type == JSMN_STRING) {
                    set_zero(buf, &tok[2]);
                    Call_ID = buf + tok[2].start;
                    ocppRequestId = (uint8_t *)Call_ID;
            }

            if (Call_Type == 2) { // call from CS

                if (tok[3].type == JSMN_STRING) {
                    set_zero(buf, &tok[3]);
                    
                    const char *str_in = (const char *)buf + tok[3].start;
                    if (strcmp( str_in, PSTR_CancelReservation) == 0) {
                        CancelReservation(ocpp_call_conf, buf);
                    } else if (strcmp( str_in, PSTR_ChangeAvailability) == 0) {
                        ChangeAvailability(ocpp_call_conf, buf);
                    } else if (strcmp( str_in, PSTR_ChangeConfiguration) == 0) {
                        ChangeConfiguration(ocpp_call_conf, buf);
                    } else if (strcmp( str_in, PSTR_ClearCache) == 0) {
                        ClearCache(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_ClearChargingProfile) == 0) {
                        ClearChargingProfile(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_DataTransfer) == 0) {
                        DataTransfer(ocpp_call_conf, buf);
                    } else if (strcmp( str_in, PSTR_GetCompositeSchedule) == 0) {
                        GetCompositeSchedule(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_GetConfiguration) == 0) {
                        GetConfiguration(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_GetDiagnostics) == 0) {
                        GetDiagnostics(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_GetLocalListVersion) == 0) {
                        GetLocalListVersion(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_RemoteStartTransaction) == 0) {
                        RemoteStartTransaction(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_RemoteStopTransaction) == 0) {
                        RemoteStopTransaction(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_ReserveNow) == 0) {
                        ReserveNow(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_Reset) == 0) {
                        Reset(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_SendLocalList) == 0) {
                        SendLocalList(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_SetChargingProfile) == 0) {
                        SetChargingProfile(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_TriggerMessage) == 0) {
                        TriggerMessage(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_UnlockConnector) == 0) {
                        UnlockConnector(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_UpdateFirmware) == 0) {
                        UpdateFirmware(ocpp_call_conf, buf); 
                    /////////////////////////
                    } else if (strcmp( str_in, PSTR_Authorize ) == 0) {
                        Authorize(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_BootNotification ) == 0) {
                        BootNotification(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_DiagnosticsStatusNotification) == 0) {
                        DiagnosticsStatusNotification(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_FirmwareStatusNotification ) == 0) {
                        FirmwareStatusNotification(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_Heartbeat ) == 0) {
                        Heartbeat(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_MeterValues ) == 0) {
                        MeterValues(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_StartTransaction ) == 0) {
                        StartTransaction(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_StatusNotification ) == 0) {
                        StatusNotification(ocpp_call_conf, buf); 
                    } else if (strcmp( str_in, PSTR_StopTransaction ) == 0) {
                        StopTransaction(ocpp_call_conf, buf);
                    }
                }
            } else if (Call_Type == 3) { // req result
                // if (tok[3].type == JSMN_OBJECT) {
                StartTransaction(ocpp_call_reply, buf);
                BootNotification(ocpp_call_reply, buf); // tok, buf + tok[2].start
                Heartbeat(ocpp_call_reply, buf);
                Authorize(ocpp_call_reply, buf);
                StatusNotification(ocpp_call_reply, buf);
            } else if (Call_Type == 4) { // error
                set_zero(buf, &tok[3]);
                set_zero(buf, &tok[4]);
                set_zero(buf, &tok[7]);
//                const uint8_t *m1 = buf + tok[3].start;
//                const uint8_t *m2 = buf + tok[4].start;
//                const uint8_t *m3 = buf + tok[7].start;
//                printf("\n[%s] ERROR %s: %s\n%s\n\n", Call_ID, m1, m2, m3);
                // [4,"7b437670-e231-4a82-bab7-b6042bf2d896",
                // "FormationViolation",
                // "The payload for action could not be deserialized",
                // {"errorMsg":"Cannot deserialize value of type `java.util.ArrayList<ocpp.cs._2015._10.MeterValue>` from Object val..."}]
            }

        } else {
//        	printf("JSON_NO_ARRAY\n");
        }
    }
}

volatile uint16_t ocpp_task = 0;

void Ocpp_ESTABLISHED() {
//    printf("\nOCPP ESTABLISHED\n");
    ocpp_task |= (1 << task_BootNotification);
    ocpp_task |= (1 << task_StatusNotification);
    ocpp_task |= (1 << task_Heartbeat);
}

//void ___Ocpp_IDLE_timer();

// void Ocpp_IDLE() {}

void Ocpp_IDLE() {

//    printf("~");
    if (ocpp_task & 1 << (task_BootNotification)) {
        ocpp_task &= ~(1 << task_BootNotification);

        BootNotification(ocpp_call_req, NULL);

    } else if (ocpp_task & 1 << (task_Heartbeat)) {
        ocpp_task &= ~(1 << task_Heartbeat);

        Heartbeat(ocpp_call_req, NULL);

    } else if (ocpp_task & 1 << (task_Authorize)) {
        ocpp_task &= ~(1 << task_Authorize);

        Authorize(ocpp_call_req, (const uint8_t *) idTagString);

    } else if (ocpp_task & 1 << (task_MeterValues)) {
        ocpp_task &= ~(1 << task_MeterValues);

    } else if (ocpp_task & 1 << (task_StartTransaction)) {
        ocpp_task &= ~(1 << task_StartTransaction);

        StartTransaction(ocpp_call_req, (const uint8_t *) transactionData[getCurrentConnector()].idTagString);

    } else if (ocpp_task & 1 << (task_StatusNotification)) {
        ocpp_task &= ~(1 << task_StatusNotification);

        StatusNotification(ocpp_call_req, NULL);

    } else if (ocpp_task & 1 << (task_StopTransaction)) {
        ocpp_task &= ~(1 << task_StopTransaction);

        StopTransaction(ocpp_call_req, (const uint8_t *)transactionData[getCurrentConnector()].idTagString);

    } else if (ocpp_task & 1 << (task_DiagnosticsStatusNotification)) {
        ocpp_task &= ~(1 << task_DiagnosticsStatusNotification);

    } else if (ocpp_task & 1 << (task_FirmwareStatusNotification)) {
        ocpp_task &= ~(1 << task_FirmwareStatusNotification);

    } else {
//        ___Ocpp_IDLE_timer();
    }

    if (connectorStatus1 != connectorStatus1Last) {
    	setCurrentConnector(1);
    	StatusNotification(ocpp_call_req, NULL);
    	connectorStatus1Last = connectorStatus1;
    }

    if (connectorStatus2 != connectorStatus2Last) {
    	setCurrentConnector(2);
    	StatusNotification(ocpp_call_req, NULL);
    	connectorStatus2Last = connectorStatus2;
    }

    osDelay(20);
}

//static uint32_t ts_StatusNotification = 0;
//static uint32_t ts_Authorize = 0;
//static uint32_t prev_ut = 0;

//void ___Ocpp_IDLE_timer() {
//
//    if (uptime_sec() % 30 == 0) {
//        ocpp_task |= (1 << task_Heartbeat);
//    }
//
//    if (uptime_sec() - ts_StatusNotification > 240) {
//        ts_StatusNotification = uptime_sec();
//        ocpp_task |= (1 << task_StatusNotification);
//    }
//
//    if (uptime_sec() - ts_Authorize > 120) {
//        ts_Authorize = uptime_sec();
//        if (Last_transaction_id == 0) {
//            ocpp_task |= (1 << task_StartTransaction);
//        } else {
//            ocpp_task |= (1 << task_StopTransaction);
//        }
//    }
//    if (prev_ut != uptime_sec()) {
//        prev_ut = uptime_sec();
//        // printf(" uptime_sec=%lu\n"), prev_ut);
//    }
//
//}










// void Ocpp_init() {
// }



/*
 * 
 * 
 * 
 * req_BootNotification();
 * 
 * printf("ocpp_req_message_tail=%d\n", ocpp_req_message_tail);
 * 
 * printf("\n%s\n", ocpp_req_message);
 * 
 * ocpp_req_message_tail = 0;
 * 
 * 
 * 
 * return 0;
 * 
 * //For reset purposes, we store original argc and argv values & pointers.
 * original_argc = argc;
 * original_argv = argv;
 * 
 * int error = CP_initialize();
 * if (error > 0) {
 *    currentChargePointState         =   _CP_STATUS_FAULTED;
 *    currentChargePointErrorStatus   =   _CP_ERROR_INTERNALERROR;
 *    return error;
 * }
 * 
 * lwsl_user("LWS minimal ws client\n");
 * signal(SIGINT, sigint_handler);
 * 
 * struct lws_context_creation_info info;
 * memset(&info, 0, sizeof info);
 * lws_cmdline_option_handle_builtin(argc, argv, &info);
 * 
 * 
 * info.fd_limit_per_thread = 1 + 1 + 1;
 * info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
 * info.port = CONTEXT_PORT_NO_LISTEN; // we do not run any server
 // info.protocols = protocols;
 // 
 // ssl_connection &= ~LCCSCF_USE_SSL;
 // ssl_connection |= LCCSCF_ALLOW_SELFSIGNED;
 // ssl_connection |= LCCSCF_ALLOW_INSECURE;
 // ssl_connection |= LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
 // ssl_connection |= LCCSCF_ALLOW_EXPIRED;
 // 
 // context = lws_create_context(&info);
 // if (!context) {
 //     lwsl_err("lws init failed\n");
 //     return 1;
 // }
 // 
 //     Creamos un thread que manda un heartbeat cada 'heartbeat' microsegundos
 // pthread_t pidheartbeat;
 // struct pthread_routine_tool tool;
 // tool.wsi = mco.wsi;
 // tool.context = context;
 // 
 // pthread_create(&pidheartbeat, NULL, sendHeartbeat, &tool);
 // pthread_detach(pidheartbeat);
 // 
 // sendToDisplay("Initializing...");
 // 
 // 
 // 
 // int n = 0;
 // schedule the first client connection attempt to happen immediately
 // lws_sul_schedule(context, 0, &mco.sul, connect_client, 1);
 // while (n >= 0 && !interrupted) { n = lws_service(context, 0); }
 // lws_context_destroy(context);
 // lwsl_user("Completed\n");
 // return 0;*/
 
 
