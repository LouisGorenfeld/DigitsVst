/*
 *  DigitsBench.cpp
 *  PDVST
 *
 *  Created by Louis Gorenfeld on 9/20/12.
 *  Copyright 2012 ExtentOfTheJam. All rights reserved.
 *
 *	This is a minimal class to provide a decent benchmark for
 *  Digits outside of the host
 */

#include "VstCore.h"

int main()
{
	static const int kBlockSize = 256; 
	VstCore digitsCore((void*)1);

	digitsCore.setSampleRate(48000);
	digitsCore.setBlockSize(kBlockSize);
	digitsCore.setProgram(3 /* Flanger strings */);
	
	
	VstEvents *ev = (VstEvents*)new char[sizeof(VstEvents) + 2*sizeof(VstMidiEvent*)];
	VstMidiEvent midiEvents[4];
	midiEvents[0].midiData[0] = 0x90;
	midiEvents[0].midiData[1] = 64;
	midiEvents[0].midiData[2] = 0x7f;
	midiEvents[0].deltaFrames = 0;
	midiEvents[1].midiData[0] = 0x90;
	midiEvents[1].midiData[1] = 66;
	midiEvents[1].midiData[2] = 0x7f;
	midiEvents[1].deltaFrames = 0;
	midiEvents[2].midiData[0] = 0x90;
	midiEvents[2].midiData[1] = 68;
	midiEvents[2].midiData[2] = 0x7f;
	midiEvents[2].deltaFrames = 0;
	midiEvents[3].midiData[0] = 0x90;
	midiEvents[3].midiData[1] = 70;
	midiEvents[3].midiData[2] = 0x7f;
	midiEvents[3].deltaFrames = 0;
	
	ev->events[0] = &midiEvents[0];
	ev->events[1] = &midiEvents[1];
	ev->events[2] = &midiEvents[2];
	ev->events[3] = &midiEvents[3];
	ev->numEvents = 4;
	
	digitsCore.processEvents(ev);
/*	
	FILE *fp = fopen("DigitsBench.raw", "wb");
	float *outs[2];
	outs[0] = new float[48000];
	outs[1] = new float[48000];
	digitsCore.processReplacing(0, outs, 48000);
	
	fwrite(outs[0], sizeof(float), 48000, fp);
	
	fclose(fp);
*/
	
	float *outs[2];
	outs[0] = new float[kBlockSize];
	outs[1] = new float[kBlockSize];

	// Render 1 minute of audio at 48khz
	for (int i=0; i<(48000*60)/256; i++)
	{
		digitsCore.processReplacing(0, outs, kBlockSize);		
	}
	
	delete[] outs[0];
	delete[] outs[1];
		 
	return 0;
}

