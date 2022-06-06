#pragma once

// from https://github.com/QuarkTheAwesome/LiveSynthesisU/blob/master/src/program.h

#include <sndcore2/core.h>
#include <sndcore2/voice.h>

typedef unsigned char sample;

typedef struct {
	void* voice;
	
	sample* samples;
	int numSamples;
	
	int modified; //data has been modified and is ready to be played
	int stopRequested; //request to stop the voice
	int stopped; //whether the voice has stopped
	int newSoundRequested; //request to change the tone of the voice
	float newFrequency; //frequency of new tone
	int nextDataRequested; //request the audio callback prepare for a new set of samples
	int readyForNextData; //audio callback has prepared for new samples
} voiceData;
