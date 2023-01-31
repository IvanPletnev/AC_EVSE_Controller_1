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
#include "main.h"
//#include "port/port.h"
#include "ocpp_defines.h"
#include "ocpp_data.h"
#include "ocpp_types.h"
#include "ocpp.h"
#include "ocpp_strings.h"
#include <string.h>
#include <stdlib.h>
#include "websocket.h"
//#include "ws16_gsm/conn_sim800.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wformat-security"

volatile c_id_string37_t ID_BootNotification;


void BootNotification(OCPP_CALL_ARGS) {
	uint8_t tempBuf[1024] = {0};
	uint8_t len = 0;

    if (call == ocpp_call_req) {
    	ringBufferInit(&ringBuffer);
    	memset((uint8_t*)&ringBuffer.buffer, 0, RINGBUFFER_SIZE);
        MakeUUID((uint8_t *)&ID_BootNotification);
        len = sprintf ((char*) tempBuf, PSTR_Req_Head, (char *)&ID_BootNotification);
        put (&ringBuffer, tempBuf, len);
        put (&ringBuffer,(uint8_t*) PSTR_BootNotification, strlen (PSTR_BootNotification));
        put (&ringBuffer,(uint8_t*) PSTR_Obj_Start, strlen (PSTR_Obj_Start));

        ocpp_print_keyP_val((const uint8_t *)PSTR_chargePointModel, (const uint8_t *)&chargePointModel, true);
        ocpp_print_keyP_val((const uint8_t *)PSTR_chargePointVendor, (const uint8_t *)&chargePointVendor, true);
#if defined(CHARGE_POINT_SERIAL_NUMBER)
        ocpp_print_keyP_val((const uint8_t *)PSTR_chargePointSerialNumber, (const uint8_t *)&chargePointSerialNumber, true);
#endif
#if defined(CHARGE_BOX_SERIAL_NUMBER)
//        ocpp_print_keyP_val((const uint8_t *)PSTR_chargeBoxSerialNumber, (const uint8_t *)&chargeBoxSerialNumber, true);
#endif
#if defined(FIRMWARE_VERSION)
        ocpp_print_keyP_val((const uint8_t *)PSTR_firmwareVersion, (const uint8_t *)&firmwareVersion, false);
#endif
        
#if defined(ICCID)
//        ocpp_print_keyP_val((const uint8_t *)PSTR_iccid, (const uint8_t *)&iccid, true);
#else
        ocpp_print_keyP_val((const uint8_t *)PSTR_iccid, (const uint8_t *)&_Gsm_Modem.info.s_GSN, true);
#endif

#if defined(IMSI)
//        ocpp_print_keyP_val((const uint8_t *)PSTR_imsi, (const uint8_t *)&imsi, true);
#else
        ocpp_print_keyP_val((const uint8_t *)PSTR_imsi, (const uint8_t *)&_Gsm_Modem.info.s_CIMI, true);
#endif

#if defined(METER_SERIAL_NUMBER)
//        ocpp_print_keyP_val((const uint8_t *)PSTR_meterSerialNumber,  (const uint8_t *)&meterSerialNumber, true);
#endif
#if defined(METER_TYPE)
//        ocpp_print_keyP_val((const uint8_t *)PSTR_meterType, (const uint8_t *)&meterType, true);
#endif

        put (&ringBuffer, (const uint8_t *) PSTR_Obj_End, strlen (PSTR_Obj_End));
        ringBuffer.buffer[ringBuffer.bytes_avail] = '\0';

        ws_send((char *)ringBuffer.buffer);


    } else if (call == ocpp_call_reply) {

        if (strcmp((const char *)buf + tok[2].start, (const char *)&ID_BootNotification) == 0) {

            uint32_t new_time = 0;
            uint16_t new_interval = 0;
            ChargePoint_RegistrationStatus_t new_CP_RS = CP_Unknown;

            for (uint8_t ax = 0; ax < tok[3].size; ++ax) {
                uint8_t ikey = ax*2 + 4;
                uint8_t ival = ikey + 1;
                set_zero(buf, &tok[ikey]);
                set_zero(buf, &tok[ival]);
                const uint8_t *key_ptr = buf + tok[ikey].start;
                const uint8_t *val_ptr = buf + tok[ival].start;
                if (tok[ikey].type == JSMN_STRING) {
                    if (strcmp((const char *)key_ptr, "status") == 0) {
                        new_CP_RS = CP_Unknown;
                        if (strcmp((const char *)val_ptr, "Accepted") == 0) {
                            new_CP_RS = CP_Accepted;
                        } else if (strcmp((const char *)val_ptr, "Pending") == 0) {
                            new_CP_RS = CP_Pending;
                        } else if (strcmp((const char *)val_ptr, "Rejected") == 0) {
                            new_CP_RS = CP_Rejected;
                        }
                        printf ("found status=%s [%d]\n", val_ptr, new_CP_RS);
                    }
                    if (strcmp((const char *)key_ptr, "currentTime") == 0) {
                        new_time = Load_UnixTime(val_ptr);
                    }
                    if (strcmp((const char *)key_ptr, "interval") == 0) {
                        new_interval = atoi((const char *)val_ptr);
                    }
                }
            }
// new_CP_RS
            if (new_CP_RS == CP_Accepted) {
//                if (new_time)       { unixtime_set(new_time); }
                if (new_interval)   { Heartbeat_Interval = new_interval; }

                printf("found interval=%d\n", new_interval);
                printf("found currentTime=%lu\n", new_time);
            }
            ChargePoint_RegistrationStatus = new_CP_RS;
        }
    }
}




// This contains the field definition of the BootNotification.req PDU sent by the Charge Point to the Central System
//
// BootNotification.req
//
// Field Name                   Field Type          Card.    Description
// chargeBoxSerialNumber        CiString25Type      0..1    Optional. This contains a value that identifies the serial number of the Charge
//                                                          Box inside the Charge Point. Deprecated, will be removed in future version
// chargePointModel             CiString20Type      1..1    Required. This contains a value that identifies the model of the ChargePoint.
// chargePointSerialNumber      CiString25Type      0..1    Optional. This contains a value that identifies the serial number of the Charge Point.
// chargePointVendor            CiString20Type      1..1    Required. This contains a value that identifies the vendor of the ChargePoint.
// firmwareVersion              CiString50Type      0..1    Optional. This contains the firmware version of the Charge Point.
// iccid                        CiString20Type      0..1    Optional. This contains the ICCID of the modem’s SIM card.
// imsi                         CiString20Type      0..1    Optional. This contains the IMSI of the modem’s SIM card.
// meterSerialNumber            CiString25Type      0..1    Optional. This contains the serial number of the main power meter of the Charge Point.
// meterType                    CiString25Type      0..1    Optional. This contains the type of the main power meter of the Charge Point.



// This contains the field definition of the BootNotification.conf PDU sent by the Central System
// to the Charge Point in response to a BootNotification.req PDU. See also Boot Notification
//
// BootNotification.conf
//
// currentTime  dateTime            1..1    Required. This contains the Central System’s current time.
// interval     integer             1..1    Required. When RegistrationStatus is Accepted, this contains the heartbeat interval in seconds.
//                                          If the Central System returns something other than Accepted, the value of the interval field
//                                          indicates the minimum wait time before sending a next BootNotification request.
// status       RegistrationStatus  1..1    Required. This contains whether the Charge Point has been registered within the System Central.
//                                          RegistrationStatus = Accepted | Pending | Rejected

