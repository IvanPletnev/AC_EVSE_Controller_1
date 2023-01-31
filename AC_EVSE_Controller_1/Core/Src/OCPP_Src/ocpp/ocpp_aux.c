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
#include "main.h"
#include "ocpp.h"
#include "ocpp_strings.h"
#include <stdbool.h>
#include "ocpp_data.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"

uint16_t timerCnt;
extern TIM_HandleTypeDef htim10;

extern RTC_TimeTypeDef sTime;
extern RTC_DateTypeDef sDate;
extern RTC_HandleTypeDef hrtc;

void MakeUUID (uint8_t* buf) {

    uuid_bytes_t out;
    uint16_t num = 1;
    struct uuid_RFC4122 uu;
    timerCnt = __HAL_TIM_GET_COUNTER(&htim10);
    srand(timerCnt);
    __uuid_generate_random(out, &num); //calling function to generate GUID
    uuid_unpack(out, &uu);
    // 00000000-0000-0000-0309-000000000000
    sprintf((char *)buf, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", // UUID = 36 B
        (unsigned int) uu.time_low,
        uu.time_mid,
        uu.time_hi_and_version,
        (uint8_t)(uu.clock_seq >> 8),
        (uint8_t)(uu.clock_seq & 0xFF),
        uu.node[0],
        uu.node[1],
        uu.node[2],
        uu.node[3],
        uu.node[4],
        uu.node[5]
    );
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
const char __ocpp_print_keyP_val_0[]  = "\"";
const char __ocpp_print_keyP_val_1[]  = "\":\"";

void ocpp_print_keyP_val(const uint8_t *key, const uint8_t *val, bool comma) {

//	ringBufferInit(&ringBuffer);

    if (val) {
        put (&ringBuffer, (const uint8_t *)__ocpp_print_keyP_val_0, strlen (__ocpp_print_keyP_val_0));
        put (&ringBuffer, key, strlen ((const char*)key));
        put (&ringBuffer, (const uint8_t *) __ocpp_print_keyP_val_1, strlen (__ocpp_print_keyP_val_1));
        put (&ringBuffer, val, strlen ((const char*) val));
        put (&ringBuffer, (const uint8_t *)__ocpp_print_keyP_val_0, strlen(__ocpp_print_keyP_val_0));
        if (comma) {
        	put (&ringBuffer, (const uint8_t *)",", strlen (","));
        }
    }
}

#pragma GCC diagnostic pop

void __uuid_generate_random(uuid_bytes_t out, uint16_t *num) {

    uint16_t n;
    if (!num || !*num) n = 1;
    else n = *num;
    struct uuid_RFC4122 uu;
    uuid_bytes_t buf;
    for (uint16_t i = 0; i < n; i++) {
        for (uint8_t x = 0; x < sizeof(buf); ++x) {
        	buf[x] = rand() % 256;
        }
        uuid_unpack(buf, &uu);
        uu.clock_seq = (uu.clock_seq & 0x3FFF) | 0x8000;
        uu.time_hi_and_version = (uu.time_hi_and_version & 0x0FFF) | 0x4000;
        uuid_pack(&uu, out);
        out += sizeof(uuid_bytes_t);
    }
}

void uuid_unpack(const uuid_bytes_t in, struct uuid_RFC4122 *uu) {
    const uint8_t   *ptr = in;
    uint32_t        tmp;
    tmp = *ptr++;
    tmp = (tmp << 8) | *ptr++;
    tmp = (tmp << 8) | *ptr++;
    tmp = (tmp << 8) | *ptr++;
    uu->time_low = tmp;
    tmp = *ptr++;
    tmp = (tmp << 8) | *ptr++;
    uu->time_mid = tmp;
    tmp = *ptr++;
    tmp = (tmp << 8) | *ptr++;
    uu->time_hi_and_version = tmp;
    tmp = *ptr++;
    tmp = (tmp << 8) | *ptr++;
    uu->clock_seq = tmp;
    memcpy(uu->node, ptr, 6);
}
void uuid_pack(const struct uuid_RFC4122 *uu, uuid_bytes_t ptr) {

    uint32_t   tmp;
    uint8_t   *out = ptr;
    tmp = uu->time_low;     
    out[3] = (uint8_t)tmp; tmp >>= 8;
    out[2] = (uint8_t)tmp; tmp >>= 8;
    out[1] = (uint8_t)tmp; tmp >>= 8;
    out[0] = (uint8_t)tmp;
    tmp = uu->time_mid;
    out[5] = (uint8_t)tmp; tmp >>= 8;
    out[4] = (uint8_t)tmp;
    tmp = uu->time_hi_and_version;
    out[7] = (uint8_t) tmp; tmp >>= 8;
    out[6] = (uint8_t) tmp;
    tmp = uu->clock_seq;
    out[9] = (uint8_t) tmp; tmp >>= 8;
    out[8] = (uint8_t) tmp;
    memcpy(out + 10, uu->node, 6);
}


#define in_range(c, lo, up)  ((uint8_t)c >= lo && (uint8_t)c <= up)
#define isdigit(c)           in_range(c, '0', '9')


#include "rtclib.h"

uint8_t __calc_10(uint8_t a, uint8_t b) {
    return ((a-'0')*10 + (b-'0'));
}

// 2022-09-03T21:15:03.198Z
uint32_t Load_UnixTime(const uint8_t *tmp_c) {
    uint32_t ret = 0;
    if (strlen((const char *)tmp_c) > 18) {

        if (isdigit(tmp_c[0])  ||
            isdigit(tmp_c[1])  ||
            isdigit(tmp_c[2])  ||
            isdigit(tmp_c[3])  ||
             '-' == tmp_c[4]   ||
            isdigit(tmp_c[5])  || 
            isdigit(tmp_c[6])  ||
             '-' == tmp_c[7]   ||
            isdigit(tmp_c[8])  ||
            isdigit(tmp_c[9])  ||
             'T' == tmp_c[10]  ||
            isdigit(tmp_c[11]) ||
            isdigit(tmp_c[12]) ||
             ':' == tmp_c[13]  ||
            isdigit(tmp_c[14]) ||
            isdigit(tmp_c[15]) ||
             ':' == tmp_c[16]  ||
            isdigit(tmp_c[17]) ||
            isdigit(tmp_c[18])
        ) {
            rtc_t rtc;
            rtc.yOff = __calc_10(tmp_c[2] , tmp_c[3] );
            rtc.m    = __calc_10(tmp_c[5] , tmp_c[6] );
            rtc.d    = __calc_10(tmp_c[8] , tmp_c[9]);
            rtc.hh   = __calc_10(tmp_c[11], tmp_c[12]);
            rtc.mm   = __calc_10(tmp_c[14], tmp_c[15]);
            rtc.ss   = __calc_10(tmp_c[17], tmp_c[18]);
            sDate.Year = rtc.yOff;
            sDate.Month = rtc.m;
            sDate.Date = rtc.d;
            sTime.Hours = rtc.hh;
            sTime.Minutes = rtc.mm;
            sTime.Seconds = rtc.ss;
			HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
			HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
//			printf ("\r\nDate: %02d.%02d.%02d, Time: %02d:%02d:%02d\r\n", sDate.Year, sDate.Month, sDate.Date, sTime.Hours, sTime.Minutes, sTime.Seconds);

            ret = UnixTime(&rtc);
        }
    }
    return ret;
}

