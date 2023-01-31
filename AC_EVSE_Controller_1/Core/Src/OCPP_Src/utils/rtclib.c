/**************************************************************************/
/*!
 *  @file     RTClib.c
 *
 *  @mainpage Adafruit RTClib
 *
 *  @section intro Introduction
 *
 *  This is a fork of JeeLab's fantastic real time clock library for Arduino.
 *
 *  Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing
 *  products from Adafruit!
 *
 *  @section classes Available classes
 *
 *  This library provides the following classes:
 *
 *  - Classes for manipulating dates, times and durations:
 *    - DateTime represents a specific point in time; this is the data
 *      type used for setting and reading the supported RTCs
 *  - RTC emulated in software; do not expect much accuracy out of these:
 *    - RTC_Millis is based on `millis()`
 *
 *  @section license License
 *
 *  Original library by JeeLabs https://jeelabs.org/pub/docs/rtclib/, released to
 *  the public domain.
 *
 *  This version: MIT (see LICENSE)
 */
/**************************************************************************/

#include "rtclib.h"

//#include "port/port.h"

/**************************************************************************/
// utility code, some of this could be exposed in the DateTime API if needed
/**************************************************************************/

/**
  Number of days in each month, from January to November. December is not
  needed. Omitting it avoids an incompatibility with Paul Stoffregen's Time
  library. C.f. https://github.com/adafruit/RTClib/issues/114
*/
const uint8_t daysInMonth[]  = {31, 28, 31, 30, 31, 30,
                                       31, 31, 30, 31, 30};

/**************************************************************************/
/*!
    @brief  Given a date, return number of days since 2000/01/01,
            valid for 2000--2099
    @param y Year
    @param m Month
    @param d Day
    @return Number of days
*/
/**************************************************************************/
uint16_t date2days(uint16_t y, uint8_t m, uint8_t d) {
    if (y >= 2000U)
        y -= 2000U;
    uint16_t days = d;
    for (uint8_t i = 1; i < m; ++i)
        days += daysInMonth[i-1];
    if (m > 2 && y % 4 == 0)
        ++days;
    return days + 365 * y + (y + 3) / 4 - 1;
}

/**************************************************************************/
/*!
    @brief  Given a number of days, hours, minutes, and seconds, return the
   total seconds
    @param days Days
    @param h Hours
    @param m Minutes
    @param s Seconds
    @return Number of seconds total
*/
/**************************************************************************/
uint32_t time2ulong(uint16_t days, uint8_t h, uint8_t m, uint8_t s) {
    return ((days * 24UL + h) * 60 + m) * 60 + s;
}

uint32_t UnixTime(const rtc_t *rtc) {
    uint32_t t;
    uint16_t days = date2days(rtc->yOff, rtc->m, rtc->d);
    t = time2ulong(days, rtc->hh, rtc->mm, rtc->ss);
    t += SECONDS_FROM_1970_TO_2000; // seconds from 1970 to 2000
    return t;
}

void getRtcTime(RTC_HandleTypeDef* hrtc, rtc_t *rtc) {

	HAL_RTC_GetDate(hrtc, &sDate, RTC_FORMAT_BIN);
	HAL_RTC_GetTime(hrtc, &sTime, RTC_FORMAT_BIN);

	rtc->yOff = sDate.Year;
	rtc->m = sDate.Month;
	rtc->d = sDate.Date;
	rtc->hh = sTime.Hours;
	rtc->mm = sTime.Minutes;
	rtc->ss = sTime.Seconds;
	rtc->subseconds = sTime.SubSeconds;

}
