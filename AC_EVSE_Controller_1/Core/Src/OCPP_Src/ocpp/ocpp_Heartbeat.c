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

//#include "port/port.h"
#include "ocpp_defines.h"
#include "ocpp_data.h"
#include "ocpp_types.h"
#include "ocpp.h"
#include "ocpp_strings.h"
#include <string.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wformat-security"

volatile c_id_string37_t ID_Heartbeat;

void Heartbeat(OCPP_CALL_ARGS) {
	uint8_t len = 0;
	uint8_t tempBuf[128];

    if (call == ocpp_call_req) {
    	ringBufferInit(&ringBuffer);
    	memset((uint8_t*)&ringBuffer.buffer, 0, RINGBUFFER_SIZE);
        MakeUUID((uint8_t *)&ID_Heartbeat);
        len = sprintf ((char *)tempBuf, PSTR_Req_Head, (char *)&ID_Heartbeat);
        put (&ringBuffer, tempBuf, len);
        put (&ringBuffer, (const uint8_t *)PSTR_Heartbeat, strlen (PSTR_Heartbeat));
        put (&ringBuffer, (const uint8_t *)PSTR_Obj_Start, strlen (PSTR_Obj_Start));
        put (&ringBuffer, (const uint8_t *)PSTR_Obj_End, strlen (PSTR_Obj_End));
        puts ((const char*)ringBuffer.buffer);
        puts("\r\n");

        ws_send((char *)ringBuffer.buffer);

    } else if (call == ocpp_call_reply) {

        if (strcmp((const char *)buf + tok[2].start, (const char *)&ID_Heartbeat) == 0) {
            uint32_t new_time = 0;
            for (uint8_t ax = 0; ax < tok[3].size; ++ax) {
                uint8_t ikey = ax*2 + 4;
                uint8_t ival = ikey + 1;
                set_zero(buf, &tok[ikey]);
                set_zero(buf, &tok[ival]);
                const uint8_t *key_ptr = buf + tok[ikey].start;
                const uint8_t *val_ptr = buf + tok[ival].start;
                if (tok[ikey].type == JSMN_STRING) {
                    if (strcmp((const char *) key_ptr, "currentTime") == 0) {
                        new_time = Load_UnixTime(val_ptr);
                    }
                }
            }
//            if (new_time) { unixtime_set(new_time); }
        }
    }
}

