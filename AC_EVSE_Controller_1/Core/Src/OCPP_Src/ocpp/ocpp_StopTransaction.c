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

#include "ocpp_defines.h"
//#include "port/port.h"
#include "ocpp_data.h"
#include "ocpp_types.h"
#include "ocpp.h"
#include "ocpp_strings.h"
#include <string.h>
#include <stdlib.h>
#include "rtclib.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wformat-security"

volatile c_id_string37_t ID_StopTransaction;


// connectorId      integer     connectorId > 0 1..1 Required. This identifies which
//                                                   connector of the Charge Point is used. 
// idTag            IdToken     1..1  Required. This contains the identifier for which a transaction has to be started. 
// meterStart       integer     1..1  Required. This contains the meter value in Wh for the connector at start of the transaction. 
// reservationId    integer     0..1  Optional. This contains the id of the reservation that terminates as a result of this transaction. 
// timestamp        dateTime    1..1  Required. This contains the date and time on which the transaction is started. 

void StopTransaction(OCPP_CALL_ARGS) {
	uint8_t tempBuf[128] = {0};
	uint8_t len = 0;
	uint8_t connector = 0;
    
    if (call == ocpp_call_req) {

    	ringBufferInit(&ringBuffer);
    	memset((uint8_t*)&ringBuffer.buffer, 0, RINGBUFFER_SIZE);
        MakeUUID((uint8_t *)&ID_StopTransaction);
        len = sprintf((char *)tempBuf, PSTR_Req_Head, (const char *)&ID_StopTransaction);
        put (&ringBuffer, (const uint8_t *)tempBuf, len);
        put (&ringBuffer, (const uint8_t *)PSTR_StopTransaction, strlen(PSTR_StopTransaction));
        put (&ringBuffer, (const uint8_t *)PSTR_Obj_Start, strlen(PSTR_Obj_Start));
        connector = getCurrentConnector();
        len = sprintf ((char *)tempBuf, "\"transactionId\":%lu,", transactionData[connector].transactionId);
        put (&ringBuffer, (const uint8_t *)tempBuf, len);
        len = sprintf ((char *)tempBuf, "\"reason\":\"%s\",","Local");
        put (&ringBuffer, (const uint8_t *)tempBuf, len);
       // fprintf_P(ws_stream, PSTR("\"transactionData\":{},"));  Optional.
        len = sprintf ((char *)tempBuf, "\"meterStop\":%d,", 360);
        put (&ringBuffer, (const uint8_t *)tempBuf, len);
        // fprintf_P(ws_stream, PSTR("\"reservationId\":%d,"), 777);
        ocpp_print_keyP_val((const uint8_t *)"idTag", buf, true);
        
        rtc_t rtc;
        getRtcTime(&hrtc, &rtc);

        len = sprintf((char *)tempBuf, "\"timestamp\":\"20%02u-%02u-%02uT%02u:%02u:%02u.%03luZ\"",
                  rtc.yOff, rtc.m, rtc.d, rtc.hh, rtc.mm, rtc.ss, rtc.subseconds);
        put (&ringBuffer, (const uint8_t *)tempBuf, len);
        put (&ringBuffer, (const uint8_t *)PSTR_Obj_End, strlen(PSTR_Obj_End));
        
        ringBuffer.buffer[ringBuffer.bytes_avail] = '\0';
        puts ((const char*) ringBuffer.buffer);
        puts("\r\n");
        ws_send((char *)ringBuffer.buffer);
        setConnectorStatus(Finishing, connector);
    }
}
