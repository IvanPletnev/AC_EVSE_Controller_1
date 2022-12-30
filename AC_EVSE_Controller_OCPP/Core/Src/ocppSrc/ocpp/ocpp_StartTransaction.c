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
#include "port/port.h"
#include "ocpp_data.h"
#include "ocpp_types.h"
#include "ocpp.h"
#include "ocpp_strings.h"
#include <string.h>
#include <stdlib.h>
#include "utils/rtclib.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wformat-security"

volatile c_id_string37_t ID_StartTransaction;


// connectorId      integer     connectorId > 0 1..1 Required. This identifies which
//                                                   connector of the Charge Point is used. 
// idTag            IdToken     1..1  Required. This contains the identifier for which a transaction has to be started. 
// meterStart       integer     1..1  Required. This contains the meter value in Wh for the connector at start of the transaction. 
// reservationId    integer     0..1  Optional. This contains the id of the reservation that terminates as a result of this transaction. 
// timestamp        dateTime    1..1  Required. This contains the date and time on which the transaction is started. 

void StartTransaction(OCPP_CALL_ARGS) {
    uint8_t tempBuf[128] = {0};
    uint8_t len = 0;

    if (call == ocpp_call_req) {
    	ringBufferInit(&ringBuffer);
        MakeUUID((uint8_t *)&ID_StartTransaction);
        len = sprintf ((char *) tempBuf, PSTR_Req_Head, (char *)&ID_StartTransaction);
        put (&ringBuffer, (const uint8_t *) tempBuf, len);
        put (&ringBuffer, (const uint8_t *)PSTR_StartTransaction, strlen(PSTR_StartTransaction));
        put (&ringBuffer, (const uint8_t *)PSTR_Obj_Start, strlen(PSTR_Obj_Start));
        len = sprintf((char *)tempBuf, "\"connectorId\":%d,", 1);
        put (&ringBuffer, (const uint8_t *) tempBuf, len);
        sprintf((char *)tempBuf, "\"meterStart\":%d,", 333);
        put (&ringBuffer, (const uint8_t *) tempBuf, len);
        // fprintf_P(ws_stream, "\"reservationId\":%d,"), 777);
        ocpp_print_keyP_val((const uint8_t *)"idTag", buf, true);
        
        rtc_t rtc;
        getRtcTime(&hrtc, &rtc);
        //         20%02u-&u-%uT21:15:03.198Z
        // 20%02u-%02u-%02uT%02u:%02u:%02u.000Z
        len = sprintf((char *)tempBuf, "\"timestamp\":\"20%02u-%02u-%02uT%02u:%02u:%02u.%03luZ\"",
                  rtc.yOff, rtc.m, rtc.d, rtc.hh, rtc.mm, rtc.ss, rtc.subseconds);
        put (&ringBuffer, (const uint8_t *) tempBuf, len);
        put (&ringBuffer, (const uint8_t *)PSTR_Obj_End, strlen(PSTR_Obj_End));
        
        HAL_UART_Transmit_DMA(&huart1, ringBuffer.buffer, ringBuffer.bytes_avail);

    } else if (call == ocpp_call_reply) {
        
        // [3,"0f505b35-aa81-420c-aa8e-c479d27f448a",{"transactionId":92,"idTagInfo":{"status":"Accepted","expiryDate":"2022-09-04T15:36:58.265Z"}}]
        if (strcmp((char *)buf + tok[2].start, (const char *)&ID_StartTransaction) == 0) {
            
            int32_t new_transaction_id = 0;
            
            for (uint8_t ax = 0; ax < tok[3].size; ++ax) {
                
                uint8_t ikey = ax*2 + 4;
                uint8_t ival = ikey + 1;
                set_zero(buf, &tok[ikey]);
                set_zero(buf, &tok[ival]);
                const uint8_t *key_ptr = buf + tok[ikey].start;
                const uint8_t *val_ptr = buf + tok[ival].start;
                
                if (tok[ikey].type == JSMN_STRING && tok[ival].type == JSMN_PRIMITIVE) {
                    
                    if (strcmp((const char *)key_ptr, "transactionId") == 0) {
                        new_transaction_id = atol((const char *)val_ptr);
                        printf("**new_transaction_id = %lu\n", new_transaction_id);
                    }
                }
                
                if (tok[ikey].type == JSMN_STRING && tok[ival].type == JSMN_OBJECT) {
                    
                    if (strcmp((const char *)key_ptr, "idTagInfo") == 0) {
//                         new_transaction_id = atoi(val_ptr);
                        Tag_AuthorizationStatus_t   new_AuthStatus = Auth_Unknown;
                        uint32_t                    new_expiryDate = 0;
                        for (uint8_t ay = 0; ay < tok[ival].size; ++ay) {
                            uint8_t jkey = ay*2 + ival + 1; // ikey
                            uint8_t jval = jkey + 1;
                            set_zero(buf, &tok[jkey]);
                            set_zero(buf, &tok[jval]);
                            const uint8_t *skey_ptr = buf + tok[jkey].start;
                            const uint8_t *sval_ptr = buf + tok[jval].start;
                            if (tok[jkey].type == JSMN_STRING && tok[jval].type == JSMN_STRING) {
                                
                                if (strcmp((const char *)skey_ptr, "status") == 0) {
                                    if (strcmp((const char *)sval_ptr, "Accepted") == 0) {
                                        new_AuthStatus = Auth_Accepted;
                                    }
                                    if (strcmp((const char *)sval_ptr, "Blocked") == 0) {
                                        new_AuthStatus = Auth_Blocked;
                                    }
                                    if (strcmp((const char *)sval_ptr, "Expired") == 0) {
                                        new_AuthStatus = Auth_Expired;
                                    }
                                    if (strcmp((const char *)sval_ptr, "Invalid") == 0) {
                                        new_AuthStatus = Auth_Invalid;
                                    }
                                    if (strcmp((const char *)sval_ptr, "ConcurrentTx") == 0) {
                                        new_AuthStatus = Auth_ConcurrentTx;
                                    }
                                }
                                
                                if (strcmp((const char *)skey_ptr, "expiryDate") == 0) {
                                    new_expiryDate = Load_UnixTime(sval_ptr);
                                }
                            }
                        }
                        if (new_expiryDate && new_AuthStatus != Auth_Unknown) {
                            printf(">> new_expiryDate=%lu new_AuthStatus=%lu\n" , new_expiryDate , (unsigned long int)new_AuthStatus);
                            Last_transaction_id = new_transaction_id;
                        }
                    }
                }
                //             if (new_CP_RS == CP_Accepted) {
                //                 if (new_time)       { unixtime_set(new_time); }
                //                 if (new_interval)   { Heartbeat_Interval = new_interval; }
                //                 printf("found interval=%d\n", new_interval);
                //                 printf("found currentTime=%u\n", new_time);
                //             }
            }
        }
    }
    // idTagInfo      IdTagInfo      1..1     Required. This contains information about authorization status, expiry and parent id. 
    // transactionId      integer      1..1     Required. This contains the transaction id supplied by the Central System. 
}
