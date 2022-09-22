#include "atsdigitizer.h"
#include "AlazarApi.h"
#include "stdio.h"

#ifdef _WIN32
#include <conio.h>
#else // ifndef _WIN32
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define TRUE  1
#define FALSE 0

#define _snprintf snprintf

inline U32 GetTickCount(void);
inline void Sleep(U32 dwTime_ms);
inline int _kbhit(void);
inline int GetLastError();
#endif // ifndef _WIN32

// TODO: Select the number of DMA buffers to allocate.

#define BUFFER_COUNT 4

// Globals variables
U16* BufferArray[BUFFER_COUNT] = { NULL };
double samplesPerSec = 0.0;

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
	U32 triggerLevel_code = (U32)(128 + 127 * triggerLevel_volts / triggerRange_volts);
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
    // external clock should be larger than 200k(ECLK port: from laser CLK)
	U32 triggerDelay_samples = (U32)(triggerDelay_sec * 200e3 + 100);// should be multiple of 16 for 1 channel and multiple of 8 for 2 chs
	retCode = AlazarSetTriggerDelay(boardHandle, triggerDelay_samples);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetTriggerDelay failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// set trigger timeout: The timeout value is expressed in 10 us units, where 0 means disable the timeout counterand wait forever for a trigger event.
	double triggerTimeout_sec = 500;
	U32 triggerTimeout_clocks = (U32)(triggerTimeout_sec / 10.e-5 + 0.5);// unit 10us, 0--> (infinite) forever wait
	retCode = AlazarSetTriggerTimeOut(boardHandle, triggerTimeout_clocks);
	if (retCode != ApiSuccess)
	{
		printf("Error: AlazarSetTriggerTimeOut failed -- %s\n", AlazarErrorToText(retCode));
		return FALSE;
	}

	// TODO: Configure AUX I/O connector as required tigger enable to ADMA mode
	retCode = AlazarConfigureAuxIO(boardHandle, AUX_IN_TRIGGER_ENABLE, TRIGGER_SLOPE_POSITIVE);
    //retCode = AlazarConfigureAuxIO(boardHandle, AUX_OUT_TRIGGER, 0);
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

int ATSdigitizer::AlazarDLLAcq() {
    // There are no pre-trigger samples in NPT mode
    U32 preTriggerSamples = 0;

    // TODO: Select the number of post-trigger samples per record
    U32 postTriggerSamples = 1280;

    // TODO: Specify the number of records per DMA buffer
    U32 recordsPerBuffer = 300;


    // TODO: Specify the total number of buffers to capture
    U32 buffersPerAcquisition = 100;//Set to 0x7fffffff to acquire indefinitely until the acquisition is aborted

    // TODO: Select which channels to capture (A, B, or both)
    U32 channelMask = CHANNEL_A;

    // TODO: Select if you wish to save the sample data to a file
    BOOL saveData = true;//false

    // Calculate the number of enabled channels from the channel mask
    int channelCount = 0;
    int channelsPerBoard = 2;
    for (int channel = 0; channel < channelsPerBoard; channel++)
    {
        U32 channelId = 1U << channel;
        if (channelMask & channelId)
            channelCount++;
    }

    // Get the sample size in bits, and the on-board memory size in samples per channel
    U8 bitsPerSample;
    U32 maxSamplesPerChannel;
    RETURN_CODE retCode = AlazarGetChannelInfo(boardHandle, &maxSamplesPerChannel, &bitsPerSample);
    if (retCode != ApiSuccess)
    {
        printf("Error: AlazarGetChannelInfo failed -- %s\n", AlazarErrorToText(retCode));
        return FALSE;
    }

    // Calculate the size of each DMA buffer in bytes
    float bytesPerSample = (float)((bitsPerSample + 7) / 8);
    U32 samplesPerRecord = preTriggerSamples + postTriggerSamples;
    U32 bytesPerRecord = (U32)(bytesPerSample * samplesPerRecord +
        0.5); // 0.5 compensates for double to integer conversion 
    U32 bytesPerBuffer = bytesPerRecord * recordsPerBuffer * channelCount;

    // Create a data file if required
    FILE* fpData = NULL;

    if (saveData)
    {
        fpData = fopen("data.bin", "wb");
        if (fpData == NULL)
        {
            printf("Error: Unable to create data file -- %u\n", GetLastError());
            return FALSE;
        }
    }


    // Allocate memory for DMA buffers
    BOOL success = TRUE;

    U32 bufferIndex;
    for (bufferIndex = 0; (bufferIndex < BUFFER_COUNT) && success; bufferIndex++)
    {
#ifdef _WIN32 // Allocate page aligned memory
        BufferArray[bufferIndex] =
            (U16*)VirtualAlloc(NULL, bytesPerBuffer, MEM_COMMIT, PAGE_READWRITE);
#else
        BufferArray[bufferIndex] = (U16*)valloc(bytesPerBuffer);
#endif
        if (BufferArray[bufferIndex] == NULL)
        {
            printf("Error: Alloc %u bytes failed\n", bytesPerBuffer);
            success = FALSE;
        }
    }


    // Configure the record size
    if (success)
    {
        retCode = AlazarSetRecordSize(boardHandle, preTriggerSamples, postTriggerSamples);
        if (retCode != ApiSuccess)
        {
            printf("Error: AlazarSetRecordSize failed -- %s\n", AlazarErrorToText(retCode));
            success = FALSE;
        }
    }

    // Configure the board to make an NPT AutoDMA acquisition
    if (success)
    {
        //Set to 0x7fffffff to acquire indefinitely until the acquisition is aborted
        U32 recordsPerAcquisition = recordsPerBuffer * buffersPerAcquisition;

        //U32 admaFlags = ADMA_EXTERNAL_STARTCAPTURE | ADMA_NPT | ADMA_FIFO_ONLY_STREAMING;
        U32 admaFlags = ADMA_EXTERNAL_STARTCAPTURE | ADMA_NPT;

        retCode = AlazarBeforeAsyncRead(boardHandle, channelMask, -(long)preTriggerSamples,
            samplesPerRecord, recordsPerBuffer, recordsPerAcquisition,
            admaFlags);
        if (retCode != ApiSuccess)
        {
            printf("Error: AlazarBeforeAsyncRead failed -- %s\n", AlazarErrorToText(retCode));
            success = FALSE;
        }
    }

    // Add the buffers to a list of buffers available to be filled by the board

    for (bufferIndex = 0; (bufferIndex < BUFFER_COUNT) && success; bufferIndex++)
    {
        U16* pBuffer = BufferArray[bufferIndex];
        retCode = AlazarPostAsyncBuffer(boardHandle, pBuffer, bytesPerBuffer);
        if (retCode != ApiSuccess)
        {
            printf("Error: AlazarPostAsyncBuffer %u failed -- %s\n", bufferIndex,
                AlazarErrorToText(retCode));
            success = FALSE;
        }
    }


    // Arm the board system to wait for a trigger event to begin the acquisition
    if (success)
    {
        retCode = AlazarStartCapture(boardHandle);
        if (retCode != ApiSuccess)
        {
            printf("Error: AlazarStartCapture failed -- %s\n", AlazarErrorToText(retCode));
            success = FALSE;
        }
    }

    // Wait for each buffer to be filled, process the buffer, and re-post it to
    // the board.
    if (success)
    {
        printf("Capturing %d buffers ... press any key to abort\n", buffersPerAcquisition);

        U32 startTickCount = GetTickCount();
        U32 buffersCompleted = 0;
        INT64 bytesTransferred = 0;
        while (buffersCompleted < buffersPerAcquisition)
        {
            // TODO: Set a buffer timeout that is longer than the time
            //       required to capture all the records in one buffer.
            U32 timeout_ms = 5000;

            // Wait for the buffer at the head of the list of available buffers
            // to be filled by the board.
            bufferIndex = buffersCompleted % BUFFER_COUNT;
            U16* pBuffer = BufferArray[bufferIndex];

            retCode = AlazarWaitAsyncBufferComplete(boardHandle, pBuffer, timeout_ms);
            if (retCode != ApiSuccess)
            {
                printf("Error: AlazarWaitAsyncBufferComplete failed -- %s\n",
                    AlazarErrorToText(retCode));
                success = FALSE;
            }

            if (success)
            {
                // The buffer is full and has been removed from the list
                // of buffers available for the board

                buffersCompleted++;
                bytesTransferred += bytesPerBuffer;

                // TODO: Process sample data in this buffer.---------------------------

                // NOTE:
                //
                // While you are processing this buffer, the board is already filling the next
                // available buffer(s).
                //
                // You MUST finish processing this buffer and post it back to the board before
                // the board fills all of its available DMA buffers and on-board memory.
                //
                // Samples are arranged in the buffer as follows: S0A, S0B, ..., S1A, S1B, ...
                // with SXY the sample number X of channel Y.
                //
                // A 12-bit sample code is stored in the most significant bits of in each 16-bit
                // sample value. 
                // Sample codes are unsigned by default. As a result:
                // - a sample code of 0x0000 represents a negative full scale input signal.
                // - a sample code of 0x8000 represents a ~0V signal.
                // - a sample code of 0xFFFF represents a positive full scale input signal.  
                if (saveData)
                {
                    // Write record to file
                    size_t bytesWritten = fwrite(pBuffer, sizeof(BYTE), bytesPerBuffer, fpData);
                    if (bytesWritten != bytesPerBuffer)
                    {
                        printf("Error: Write buffer %u failed -- %u\n", buffersCompleted,
                            GetLastError());
                        success = FALSE;
                    }
                }
            }

            // Add the buffer to the end of the list of available buffers.
            if (success)
            {
                retCode = AlazarPostAsyncBuffer(boardHandle, pBuffer, bytesPerBuffer);
                if (retCode != ApiSuccess)
                {
                    printf("Error: AlazarPostAsyncBuffer failed -- %s\n",
                        AlazarErrorToText(retCode));
                    success = FALSE;
                }
            }

            // If the acquisition failed, exit the acquisition loop
            if (!success)
                break;

            // If a key was pressed, exit the acquisition loop
            if (_kbhit())
            {
                printf("Aborted...\n");
                break;
            }

            // Display progress
            printf("Completed %u buffers\r", buffersCompleted);
        }

        // Display results
        double transferTime_sec = (GetTickCount() - startTickCount) / 1000.;
        printf("Capture completed in %.2lf sec\n", transferTime_sec);

        double buffersPerSec;
        double bytesPerSec;
        double recordsPerSec;
        U32 recordsTransferred = recordsPerBuffer * buffersCompleted;

        if (transferTime_sec > 0.)
        {
            buffersPerSec = buffersCompleted / transferTime_sec;
            bytesPerSec = bytesTransferred / transferTime_sec;
            recordsPerSec = recordsTransferred / transferTime_sec;
        }
        else
        {
            buffersPerSec = 0.;
            bytesPerSec = 0.;
            recordsPerSec = 0.;
        }

        printf("Captured %u buffers (%.4g buffers per sec)\n", buffersCompleted, buffersPerSec);
        printf("Captured %u records (%.4g records per sec)\n", recordsTransferred, recordsPerSec);
        printf("Transferred %I64d bytes (%.4g bytes per sec)\n", bytesTransferred, bytesPerSec);
    }

    // Abort the acquisition
    retCode = AlazarAbortAsyncRead(boardHandle);
    if (retCode != ApiSuccess)
    {
        printf("Error: AlazarAbortAsyncRead failed -- %s\n", AlazarErrorToText(retCode));
        success = FALSE;
    }

    // Free all memory allocated
    for (bufferIndex = 0; bufferIndex < BUFFER_COUNT; bufferIndex++)
    {
        if (BufferArray[bufferIndex] != NULL)
        {
#ifdef _WIN32
            VirtualFree(BufferArray[bufferIndex], 0, MEM_RELEASE);
#else
            free(BufferArray[bufferIndex]);
#endif
        }
    }

    // Close the data file

    if (fpData != NULL)
        fclose(fpData);


    return success;
}

#ifndef WIN32
inline U32 GetTickCount(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
        return 0;
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

inline void Sleep(U32 dwTime_ms)
{
    usleep(dwTime_ms * 1000);
}

inline int _kbhit(void)
{
    struct timeval tv;
    fd_set rdfs;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&rdfs);
    FD_SET(STDIN_FILENO, &rdfs);

    select(STDIN_FILENO + 1, &rdfs, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &rdfs);
}

inline int GetLastError()
{
    return errno;
}
#endif

