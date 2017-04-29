/*
 *  FauxVst.h
 *  PDVST
 *
 *  Created by Louis Gorenfeld on 9/18/12.
 *  Copyright 2012 Extent of the Jam. All rights reserved.
 *
 */

// This header is intended to get Digits to compile for non-VST
// builds. It's essentially a stub. DO NOT use it to figure out
// the VST SDK since it diverges from the VST SDK spec

#ifndef _FAUX_VST_H_
#define _FAUX_VST_H_

#define vst_strncpy strncpy
#define vst_strncat strncat

typedef int VstInt32;
typedef int* VstIntPtr;

// This is not used, but it should be non-zero when instantiating
// the plugin (just set it to 1 or something)
typedef void* audioMasterCallback;

const int kVstMaxProgNameLen = 128;
const int kVstMaxParamStrLen = 128;
const int kVstMaxEffectNameLen = 128;
const int kVstMaxVendorStrLen = 128;
const int kVstMaxProductStrLen = 128;

// Dummy declarations to make the compiler happy
const int kVstMidiType = 0;
const int kVstPinIsStereo = 0;
const int kVstPinIsActive = 0;

struct VstPinProperties
{
	char label[kVstMaxParamStrLen];
	int flags;
};

struct VstMidiEvent
{
	// NOTE: The official SDK uses 4 elements here
	char midiData[3];
	int deltaFrames;
	int type;
	
	VstMidiEvent() { type = kVstMidiType; }
};

struct VstEvents
{
	int numEvents;
	// Only support VstMidiEvents for now
	VstMidiEvent *events[2];
};

class AudioEffect
{
public:
	AudioEffect() {}
	~AudioEffect() {}
};

class AudioEffectX : public AudioEffect
{
public:
	AudioEffectX(audioMasterCallback audioMaster, int numPatches, int numParameters) {}
	~AudioEffectX() {}
	
protected:
	void setNumInputs(int num) {}
	void setNumOutputs(int num) {}
	void canProcessReplacing() {}
	void isSynth() {}
	void setUniqueID(int id) {}
	void programsAreChunks(bool yeah) {}
	void suspend() {}
	void resume() {}
	void updateDisplay() {}
};

#endif
