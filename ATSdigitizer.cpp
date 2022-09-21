#include "atsdigitizer.h"
#include "AlazarApi.h"
#include "stdio.h"

ATSdigitizer::ATSdigitizer(U32 msysID, U32 mboaID):systemId(msysID),boardId(mboaID),retCode(ApiSuccess)
{
	// Get a handle to the board
	boardHandle = AlazarGetBoardBySystemID(systemId, boardId);
	if (boardHandle == NULL)
	{
		printf("Error: Unable to open board system Id %u board Id %u\n", systemId, boardId);
		//return FALSE;
	}
}

ATSdigitizer::~ATSdigitizer()
{

}

int ATSdigitizer::AlazarDLLCfg()
{
	// abort buffer read 
	retCode = AlazarAbortAsyncRead(boardHandle);
	if (retCode != ApiSuccess) {
		printf("Error: AlazarAbortAsyncRead failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// set external capture clock
	retCode = AlazarSetCaptureClock(boardHandle,
		FAST_EXTERNAL_CLOCK,
		SAMPLE_RATE_USER_DEF,
		CLOCK_EDGE_RISING,
		0);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetCaptureClock failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// set external clock range
	retCode = AlazarSetExternalClockLevel(boardHandle,
		55);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetExternalClockLevel failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// set capture data formate
	retCode = AlazarSetParameter(boardHandle,
		0,
		SET_DATA_FORMAT,
		DATA_FORMAT_SIGNED);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetParameter failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// set SETGET_ASYNC_BUFFCOUNT
	long buffcount = 128;
	retCode = AlazarSetParameter(boardHandle,
		0,
		SETGET_ASYNC_BUFFCOUNT,
		buffcount);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetParameter failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// set input control parameters of Channel A
	retCode = AlazarInputControl(boardHandle,
		CHANNEL_A, DC_COUPLING,
		INPUT_RANGE_PM_2_V,
		IMPEDANCE_50_OHM);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarInputControlEx failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// TODO: Select trigger inputs and levels as required -->external 155/CHB 128
	double triggerLevel_volts = 1.; // trigger level
	double triggerRange_volts = 5.; // input range
	U32 triggerLevel_code =
		(U32)(128 + 127 * triggerLevel_volts / triggerRange_volts);
	retCode = AlazarSetTriggerOperation(boardHandle,
		TRIG_ENGINE_OP_J,
		TRIG_ENGINE_J, TRIG_EXTERNAL,
		TRIGGER_SLOPE_POSITIVE,
		triggerLevel_code,
		TRIG_ENGINE_K,
		TRIG_CHAN_B,
		TRIGGER_SLOPE_POSITIVE,
		128);// 0--> negative, 128-->0, 255--> positive
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetTriggerOperation failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// TODO: Select external trigger parameters
	retCode = AlazarSetExternalTrigger(
		boardHandle, // HANDLE -- board handle
		DC_COUPLING, // U32 -- coupling id
		ETR_TTL // U32 -- external range id TTL-->2V
	);

	// TODO: Set trigger delay /unit sample
	double triggerDelay_sec = 0;
	U32 triggerDelay_samples = (U32)(triggerDelay_sec * 200e3 + 0.5);// should be multiple of 16 for 1 channel and multiple of 8 for 2 chs
	retCode = AlazarSetTriggerDelay(boardHandle, triggerDelay_samples);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetTriggerDelay failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// set trigger timeout: The timeout value is expressed in 10 us units, where 0 means disable the timeout counterand wait forever for a trigger event.
	double triggerTimeout_sec = 500;
	U32 triggerTimeout_clocks = (U32)(triggerTimeout_sec / 10.e-5 + 0.5);
	retCode = AlazarSetTriggerTimeOut(boardHandle, triggerTimeout_clocks);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetTriggerTimeOut failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// TODO: Configure AUX I/O connector as required
	retCode = AlazarConfigureAuxIO(boardHandle, AUX_IN_TRIGGER_ENABLE, TRIGGER_SLOPE_POSITIVE);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarConfigureAuxIO failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// Disable OCT ignore bad clock
	double dGoodClockDuration = 0; //unit of seconds
	double dBadClockDuration = 0;
	double* pdTriggerPulseWidth = NULL;
	double* pdTriggerCycleTime = NULL;
	retCode = AlazarOCTIgnoreBadClock(
			boardHandle,
			0, //Enable(1) or disable(0) OCT ignore bad clock.
			dGoodClockDuration,
			dBadClockDuration,
			pdTriggerCycleTime,
			pdTriggerPulseWidth);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarOCTIgnoreBadClock failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	return TRUE;
}

