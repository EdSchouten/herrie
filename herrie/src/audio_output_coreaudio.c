/*
 * Copyright (c) 2006-2007 Ed Schouten <ed@fxq.nl>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/**
 * @file audio_output_coreaudio.c
 * @brief Apple CoreAudio audio output driver.
 */

#include "stdinc.h"

#include <CoreAudio/AudioHardware.h>

#include "audio_file.h"
#include "audio_output.h"
#include "gui.h"

AudioDeviceID			adid;
AudioStreamBasicDescription	afmt;

short				*abuf;
UInt32				abuflen;

UInt32				abufulen = 0;
GMutex				*abuflock;
GCond				*abufdrained;

static OSStatus
audio_output_ioproc(AudioDeviceID inDevice, const AudioTimeStamp *inNow,
    const AudioBufferList *inInputData, const AudioTimeStamp *inInputTime,
    AudioBufferList *outOutputData, const AudioTimeStamp *inOutputTime,
    void *inClientData)
{
	UInt32 i;
	float *ob = outOutputData->mBuffers[0].mData;

	g_mutex_lock(abuflock);

	/* Convert the data to floats */
	for (i = 0; i < abufulen / sizeof(short); i++)
		ob[i] = GINT16_FROM_LE(abuf[i]) * (0.5f / SHRT_MAX);
	/* Fill the trailer with zero's */
	for (; i < abuflen / sizeof(short); i++)
		ob[i] = 0.0;

	/* Empty the buffer and notify that we can receive new data */
	abufulen = 0;
	g_mutex_unlock(abuflock);
	g_cond_signal(abufdrained);
	
	return (0);
}

int
audio_output_open(void)
{
	UInt32 size;

	/* Obtain the audio device ID */
	size = sizeof adid;
	if (AudioHardwareGetProperty(
	    kAudioHardwarePropertyDefaultOutputDevice, 
	    &size, &adid) != 0 || adid == kAudioDeviceUnknown)
		goto error;

	/* Adjust the stream format */
	size = sizeof afmt;
	if (AudioDeviceGetProperty(adid, 0, false,
	    kAudioDevicePropertyStreamFormat, &size, &afmt) != 0 ||
	    afmt.mFormatID != kAudioFormatLinearPCM)
		goto error;

	/* To be set on the first run */
	afmt.mSampleRate = 0;
	afmt.mChannelsPerFrame = 0;
	afmt.mBytesPerFrame = afmt.mChannelsPerFrame * sizeof (float);
	afmt.mBytesPerPacket = afmt.mBytesPerFrame * afmt.mFramesPerPacket;

	/* Allocate the audio buffer */
	size = sizeof abuflen;
	if (AudioDeviceGetProperty(adid, 0, false,
	    kAudioDevicePropertyBufferSize, &size, &abuflen) != 0)
		goto error;
	/* The buffer size reported is in floats */
	abuflen /= sizeof(float) / sizeof(short);
	abuf = g_malloc(abuflen);

	/* Locking down the buffer length */
	abuflock = g_mutex_new();
	abufdrained = g_cond_new();

	/* Add our own I/O handling routine */
	if (AudioDeviceAddIOProc(adid, audio_output_ioproc, NULL) != 0 ||
	    AudioDeviceStart(adid, audio_output_ioproc) != 0)
		goto error;

	return (0);
error:
	g_printerr(_("Cannot open the audio device.\n"));
	return (-1);
}

int
audio_output_play(struct audio_file *fd)
{
	int ret;

	if (fd->srate != afmt.mSampleRate ||
	    fd->channels != afmt.mChannelsPerFrame) {
		/* Sample rate or the amount of channels has changed */
		afmt.mSampleRate = fd->srate;
		afmt.mChannelsPerFrame = fd->channels;

		if (AudioDeviceSetProperty(adid, 0, 0, 0,
		    kAudioDevicePropertyStreamFormat, sizeof afmt, &afmt) != 0) {
			gui_msgbar_warn(_("Sample rate or amount of channels not supported."));
			return (0);
		}
	}

	g_mutex_lock(abuflock);
	while (abufulen != 0)
		g_cond_wait(abufdrained, abuflock);
	
	/* Read data and store it inside our temporary buffer */
	ret = abufulen = audio_file_read(fd, abuf, abuflen);

	g_mutex_unlock(abuflock);
	return (ret);
}

void
audio_output_close(void)
{
	AudioDeviceStop(adid, audio_output_ioproc);
	AudioDeviceRemoveIOProc(adid, audio_output_ioproc);
}
