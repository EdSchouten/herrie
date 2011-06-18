/*
 * Copyright (c) 2006-2011 Ed Schouten <ed@80386.nl>
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

/*
 * A small problem with CoreAudio is that it doesn't actually have a
 * standard blocking push-interface such as OSS or ALSA. We have to
 * install a routine in the audio layer that gets called routinely to
 * pull data from buffers.
 *
 * According to a discussion on the CoreAudio mailing list, we can't use
 * stuff such as spinlocks from within this callback routine (probably
 * served through some kind of interrupt-driven context) we guarantee
 * buffer locking by using atomic operations on abufulen; the indicator
 * of the length of the buffer that is used.
 */

/**
 * @brief The audio device ID obtained from CoreAudio.
 */
AudioDeviceID			adid;
/**
 * @brief The stream format used by the CoreAudio device.
 */
AudioStreamBasicDescription	afmt;
#ifdef MAC_OS_X_VERSION_10_5
/**
 * @brief The IO-process ID.
 */
AudioDeviceIOProcID		aprocid;
#else /* !MAC_OS_X_VERSION_10_5 */
#define aprocid			audio_output_ioproc
#endif /* MAC_OS_X_VERSION_10_5 */

/**
 * @brief The buffer that will be played when the current buffer is
 *        finished processing.
 */
int16_t				*abufnew;
/**
 * @brief The buffer that is currently processed by CoreAudio.
 */
int16_t				*abufcur;
/**
 * @brief The length of the buffers used by CoreAudio.
 */
int				abuflen;
/**
 * @brief The length of the abufcur that is currently used. It should
 *        only be used with g_atomic_* operations.
 */
int				abufulen = 0;
/**
 * @brief A not really used mutex that is used for the conditional
 *        variable.
 */
GMutex				*abuflock;
/**
 * @brief The conditional variable that is used to inform the
 *        application that a buffer has been processed.
 */
GCond				*abufdrained;
#ifdef BUILD_VOLUME
/**
 * @brief Preferred audio channels used for audio playback. We use it to
 *        set the volume.
 */
UInt32				achans[2] = { 0, 0 };
#endif /* BUILD_VOLUME */

/**
 * @brief Pull-function needed by CoreAudio to copy data to the audio
 *        buffers.
 */
static OSStatus
audio_output_ioproc(AudioDeviceID inDevice, const AudioTimeStamp *inNow,
    const AudioBufferList *inInputData, const AudioTimeStamp *inInputTime,
    AudioBufferList *outOutputData, const AudioTimeStamp *inOutputTime,
    void *inClientData)
{
	int i, len;
	float *ob = outOutputData->mBuffers[0].mData;

	/* Stop the IOProc handling if we're going idle */
	len = g_atomic_int_get(&abufulen);
	if (len == 0)
		AudioDeviceStop(adid, aprocid);

	/* Convert the data to floats */
	for (i = 0; i < len; i++)
		ob[i] = (float)abufcur[i] / SHRT_MAX;

	/* Empty the buffer and notify that we can receive new data */
	g_atomic_int_set(&abufulen, 0);
	g_cond_signal(abufdrained);

	/* Fill the trailer with zero's */
	for (; i < abuflen; i++)
		ob[i] = 0.0;
	
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

	/* Decide what buffer size to use */
	size = sizeof abuflen;
	abuflen = 32768;
	AudioDeviceSetProperty(adid, 0, 0, false,
	    kAudioDevicePropertyBufferSize, size, &abuflen);
	if (AudioDeviceGetProperty(adid, 0, false,
	    kAudioDevicePropertyBufferSize, &size, &abuflen) != 0)
		goto error;

#ifdef BUILD_VOLUME
	/* Store the audio channels */
	size = sizeof achans;
	AudioDeviceGetProperty(adid, 0, false,
	    kAudioDevicePropertyPreferredChannelsForStereo, &size, &achans);
#endif /* BUILD_VOLUME */

	/* The buffer size reported is in floats */
	abuflen /= sizeof(float);
	abufnew = g_malloc(abuflen * sizeof(int16_t));
	abufcur = g_malloc(abuflen * sizeof(int16_t));

	/* Locking down the buffer length */
	abuflock = g_mutex_new();
	abufdrained = g_cond_new();

	/* Add our own I/O handling routine */
#ifdef MAC_OS_X_VERSION_10_5
	if (AudioDeviceCreateIOProcID(adid, audio_output_ioproc, NULL,
	    &aprocid) != 0)
#else /* !MAC_OS_X_VERSION_10_5 */
	if (AudioDeviceAddIOProc(adid, audio_output_ioproc, NULL) != 0)
#endif /* MAC_OS_X_VERSION_10_5 */
		goto error;

	return (0);
error:
	g_printerr("%s\n", _("Cannot open the audio device."));
	return (-1);
}

int
audio_output_play(struct audio_file *fd)
{
	UInt32 len, size;
	int16_t *tmp;
	
	/* Read data in our temporary buffer */
	if ((len = audio_file_read(fd, abufnew, abuflen)) == 0)
		return (-1);

	if (fd->srate != afmt.mSampleRate ||
	    fd->channels != afmt.mChannelsPerFrame) {
		/* Sample rate or the amount of channels has changed */
		afmt.mSampleRate = fd->srate;
		afmt.mChannelsPerFrame = fd->channels;

		if (AudioDeviceSetProperty(adid, 0, 0, 0,
		    kAudioDevicePropertyStreamFormat, sizeof afmt, &afmt) != 0) {
		    	/* Get current settings back */
			size = sizeof afmt;
			AudioDeviceGetProperty(adid, 0, false,
			    kAudioDevicePropertyStreamFormat, &size, &afmt);

			gui_msgbar_warn(_("Sample rate or amount of channels not supported."));
			return (-1);
		}
	}

	/* XXX: Mutex not actually needed - only for the condvar */
	g_mutex_lock(abuflock);
	while (g_atomic_int_get(&abufulen) != 0)
		g_cond_wait(abufdrained, abuflock);
	g_mutex_unlock(abuflock);
	
	/* Toggle the buffers */
	tmp = abufcur;
	abufcur = abufnew;
	abufnew = tmp;

	/* Atomically set the usage length */
	g_atomic_int_set(&abufulen, len);

	/* Start processing of the data */
	AudioDeviceStart(adid, aprocid);

	return (0);
}

void
audio_output_close(void)
{
	AudioDeviceStop(adid, aprocid);
#ifdef MAC_OS_X_VERSION_10_5
	AudioDeviceDestroyIOProcID(adid, aprocid);
#else /* !MAC_OS_X_VERSION_10_5 */
	AudioDeviceRemoveIOProc(adid, aprocid);
#endif /* MAC_OS_X_VERSION_10_5 */

	g_free(abufnew);
	g_free(abufcur);
}

#ifdef BUILD_VOLUME
/**
 * @brief Adjust the audio output by a certain ratio and return the new
 *        value.
 */
static int
audio_output_volume_adjust(Float32 n)
{
	Float32 vl, vr, vn;
	UInt32 size = sizeof vl;

	/*
	 * Merge left and right. On Mac OS X, we want to do this. My
	 * PowerBook has this odd bug that the built-in OS X volume
	 * applet makes the sound card go unbalanced after a long amount
	 * of time. We can prevent that over here...
	 */
	if (AudioDeviceGetProperty(adid, achans[0], false,
	    kAudioDevicePropertyVolumeScalar, &size, &vl) != 0 ||
	    AudioDeviceGetProperty(adid, achans[1], false,
	    kAudioDevicePropertyVolumeScalar, &size, &vr) != 0)
		return (-1);
	vn = CLAMP((vl + vr) / 2.0 + n, 0.0, 1.0);

	/* Set the new volume */
	if (AudioDeviceSetProperty(adid, 0, achans[0], false,
	    kAudioDevicePropertyVolumeScalar, size, &vn) != 0 ||
	    AudioDeviceSetProperty(adid, 0, achans[1], false,
	    kAudioDevicePropertyVolumeScalar, size, &vn) != 0)
		return (-1);

	return (vn * 100.0);
}

int
audio_output_volume_up(void)
{
	return audio_output_volume_adjust(0.04);
}

int
audio_output_volume_down(void)
{
	return audio_output_volume_adjust(-0.04);
}
#endif /* BUILD_VOLUME */
