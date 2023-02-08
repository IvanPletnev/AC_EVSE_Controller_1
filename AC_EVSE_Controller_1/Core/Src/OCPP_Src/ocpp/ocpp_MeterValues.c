
#include "ocpp.h"
#include "mjson.h"



void MeterValues (OCPP_CALL_ARGS) {

	if (call == ocpp_call_req) {

		char time[25] = {0};
		char payload[PAYLOAD_LEN];
		char request[REQUEST_LEN];
		rtc_t rtc;
		uint8_t connector = 0;
		c_id_string37_t ID_MeterValues;

		getRtcTime(&hrtc, &rtc);
		sprintf((char*)time, "20%02u-%02u-%02uT%02u:%02u:%02u.%03luZ",
				rtc.yOff, rtc.m, rtc.d, rtc.hh, rtc.mm, rtc.ss, rtc.subseconds/10);
		MakeUUID ((uint8_t *)&ID_MeterValues);
		connector = getCurrentConnector();

		mjson_snprintf (
				payload, PAYLOAD_LEN,
				"{%Q:%u,%Q:%u,%Q:[{%Q:%Q,%Q:[{%Q:\"%u\",%Q:%Q},{%Q:\"%u\",%Q:%Q},{%Q:\"%u\",%Q:%Q},{%Q:\"%u\",%Q:%Q}]}]}",
				"connectorId",connector,
				"transactionId", transactionData[connector].transactionId,
				"meterValue",
				"timestamp", time,
				"sampledValue",
				"value", powerData.energy,
				"unit", "Wh",
				"value", powerData.power,
				"unit", "W",
				"value", powerData.current,
				"unit", "A",
				"value", powerData.voltage,
				"unit", "V"
			);

		mjson_snprintf (
			request, REQUEST_LEN,
			"[%u,%Q,%Q,%s]",
			2,
			(char*)&ID_MeterValues,
			"MeterValues",
			payload
		);
//		puts (request);
//		puts("\r\n");
		ws_send((char *)request);

	} else if (call == ocpp_call_reply) {
		puts ("meterValues_conf\r\n");
	}
}
