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
#include "ocpp_types.h"
#include "ocpp_strings.h"

#pragma GCC diagnostic ignored "-Wunused-const-variable"

// CS -> CP
const char PSTR_CancelReservation       []   = "CancelReservation";
const char PSTR_ChangeAvailability      []   = "ChangeAvailability";
const char PSTR_ChangeConfiguration     []   = "ChangeConfiguration";
const char PSTR_ClearCache              []   = "ClearCache";
const char PSTR_ClearChargingProfile    []   = "ClearChargingProfile";
const char PSTR_DataTransfer            []   = "DataTransfer";
const char PSTR_GetCompositeSchedule    []   = "GetCompositeSchedule";
const char PSTR_GetConfiguration        []   = "GetConfiguration";
const char PSTR_GetDiagnostics          []   = "GetDiagnostics";
const char PSTR_GetLocalListVersion     []   = "GetLocalListVersion";
const char PSTR_RemoteStartTransaction  []   = "RemoteStartTransaction";
const char PSTR_RemoteStopTransaction   []   = "RemoteStopTransaction";
const char PSTR_ReserveNow              []   = "ReserveNow";
const char PSTR_Reset                   []   = "Reset";
const char PSTR_SendLocalList           []   = "SendLocalList";
const char PSTR_SetChargingProfile      []   = "SetChargingProfile";
const char PSTR_TriggerMessage          []   = "TriggerMessage";
const char PSTR_UnlockConnector         []   = "UnlockConnector";
const char PSTR_UpdateFirmware          []   = "UpdateFirmware";

// CP -> CS
const char PSTR_Authorize               []   = "Authorize";
const char PSTR_BootNotification        []   = "BootNotification";
const char PSTR_DiagnosticsStatusNotification   []   = "DiagnosticsStatusNotification";
const char PSTR_FirmwareStatusNotification      []   = "FirmwareStatusNotification";
const char PSTR_Heartbeat               []   = "Heartbeat";
const char PSTR_MeterValues             []   = "MeterValues";
const char PSTR_StartTransaction        []   = "StartTransaction";
const char PSTR_StatusNotification      []   = "StatusNotification";
const char PSTR_StopTransaction         []   = "StopTransaction";

const char *ChargePointErrorCodes[] = {
		"ConnectorLockFailure",
		"EVCommunicationError",
		"GroundFailure",
		"HighTemperature",
		"InternalError",
		"LocalListConflict",
		"NoError",
		"OtherError",
		"OverCurrentFailure",
		"OverVoltage",
		"PowerMeterFailure",
		"PowerSwitchFailure",
		"ReaderFailure",
		"ResetFailure",
		"UnderVoltage",
		"WeakSignal"
};

const char *ChargePointStatuses[] = {
		"Available",
		"Preparing",
		"Charging",
		"SuspendedEVSE",
		"SuspendedEV",
		"Finishing",
		"Reserved",
		"Unavailable",
		"Faulted"
};

const char *AvailabilityStatuses[] = {
		"Accepted",
		"Rejected",
		"Scheduled"
};

const char *UnlockStatuses[] = {
		"Unlocked",
		"UnlockFailed",
		"NotSupported"
};

const char *TriggerMessageStatuses[] = {
		"Accepted",
		"Rejected",
		"NotImplemented"
};

// Required ID string
const char PSTR_chargePointVendor[]  = "chargePointVendor";
const char PSTR_chargePointModel[]   = "chargePointModel";

// Required
#if !defined(CHARGE_POINT_VENDOR) || !defined(CHARGE_POINT_MODEL)
#error "CHARGE_POINT_VENDOR or CHARGE_POINT_MODEL not defined!"
#endif

const c_string20_t chargePointVendor  = { /*CHARGE_POINT_VENDOR "\0"*/"IM.EVA" };
const c_string20_t chargePointModel  =  { CHARGE_POINT_MODEL "\0" };

#if defined(CHARGE_POINT_SERIAL_NUMBER)
const char PSTR_chargePointSerialNumber []   = "chargePointSerialNumber";
const c_string25_t chargePointSerialNumber   = { /*CHARGE_POINT_SERIAL_NUMBER "\0" */"00001"};
#endif

#if defined(CHARGE_BOX_SERIAL_NUMBER)
const char PSTR_chargeBoxSerialNumber   []   = "chargeBoxSerialNumber";
const c_string25_t chargeBoxSerialNumber     = { /*CHARGE_BOX_SERIAL_NUMBER "\0"*/"IM.EVA00001" };
#endif

#if defined(FIRMWARE_VERSION)
const char PSTR_firmwareVersion []   = "firmwareVersion";
const c_string50_t firmwareVersion   = { FIRMWARE_VERSION "\0" };
#endif

const char PSTR_iccid   []   = "iccid";

#if defined(ICCID)
const c_string20_t  iccid    = { ICCID "\0" };
#endif

const char PSTR_imsi    []   = "imsi";

#if defined(IMSI)
const c_string20_t imsi      = { IMSI "\0" };
#endif

#if defined(METER_SERIAL_NUMBER)
const char PSTR_meterSerialNumber []  = "meterSerialNumber";
const c_string25_t meterSerialNumber  = { METER_SERIAL_NUMBER "\0" };
#endif

#if defined(METER_TYPE)
const char PSTR_meterType   []   = "meterType";
const c_string25_t meterType     = { METER_TYPE "\0" };
#endif


const char PSTR_Req_Head        []   	= "[2,\"%s\",\"";
const char PSTR_Response_Head   []   	= "[3,\"%s\",\"";
const char PSTR_Response_Head_simple[] 	= "[3, \"%s\"";
const char PSTR_Error_Head      []   	= "[4,\"%s\",\"";

const char PSTR_Obj_Start       []   	= "\",{";
const char PSTR_Obj_Start_Simple[]   	= ",{";
const char PSTR_Obj_End         []   	= "}]";










