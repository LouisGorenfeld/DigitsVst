/* Phase Distortion Synthesizer (c) 2011 Louis Gorenfeld <louis.gorenfeld@gmail.com> */

#ifndef _VSTCORE_H_
#define _VSTCORE_H_

#define VERSION_MAJ 0x1
#define VERSION_MIN 0x5
#define VERSION_PAT_MAJ 0x1
#define VERSION_PAT_MIN 0x2

#ifdef FAUX_VST_HEADER
#include "FauxVST.h"
#else
#include "public.sdk/source/vst2.x/audioeffectx.h"
#endif

#include <vector>
#include <memory>
#include "PatchBankList.h"
#include "Voice.h"
#include "Chorus.h"
#include "LFO.h"
#include "Oversampler.h"

#ifdef DIGITS_MULTICORE
#include <pthread.h>
void* digits_helper(void* arg);
#endif

#ifdef DBGMCORE
#define DEBUG_MCORE(...) { fprintf(stderr, __VA_ARGS__); }
#define DEBUGFRAMESEPARATOR
#else
#define DEBUG_MCORE
#endif

class VstCore : public AudioEffectX
{
public:
	enum paramNums_t
	{
		kO1Skew = 0,
		kO1ShpMod,
		kO2Skew,
		kO2ShpMod,
		kO2Vol,
#ifdef DIGITS_PRO
        kResetOscs,
#endif
		kResoVol,
		kResoWave,
		kAmpAtt,
		kAmpDec,
		kAmpSus,
        kAmpFade,
        kAmpRel,
		kShpAtt,
		kShpDec,
		kShpSus,
		kShpFade,
        kShpRel,
		kShpMin,
		kShpMax,
		kFM,
		kDetune,
		kOctave,
		kBasisWave,
		kPulseWidth,
		kVelShaper,
		kVelVolume,
        kModShaper,
        kModLFO1Amp,
#ifdef DIGITS_PRO
        kAftertouchShaper,
        kAftertouchLFO1Amp,
#endif
		kLFORate,
		kLFOType,
		kLFODelay,
        kLFORepeat,
		kLFOShp,
		kLFOAmp,
		kLFOFrq,
		kLFOPWM,
#ifdef DIGITS_PRO
		kLFO2Rate,
		kLFO2Type,
		kLFO2Delay,
        kLFO2Repeat,
		kLFO2Shp,
		kLFO2Amp,
		kLFO2Frq,
		kLFO2PWM,
		kLFO3Rate,
		kLFO3Type,
		kLFO3Delay,
        kLFO3Repeat,
		kLFO3Shp,
		kLFO3Amp,
		kLFO3Frq,
		kLFO3PWM,
#endif
		kOrch,
		kDel_L,
		kDel_R,
		kDelWet,
		kDelFB,
		kDelLoss,
        kChrRate,
        kChrDepth,
        kChrDirtyClean,
        kChrTone,
        kChrWetDry,
        kChrFlang,
        kBitDiv,
        kBitBit,
        kBitBlend,
#ifdef DIGITS_PRO
        kLockShaperForFilter,
        kFilterReso,
        kFilterMode,
        kMonoMode,
        kPortaSpeed,
        kAmpRateScale,
        kShpRateScale,
        kShpLevelScale,
        kPitchbendUp,
        kPitchbendDn,
#endif
		kGain,
		kNumParams // Maximum value
	};
	static const char* s_paramNames[];

    
#ifdef DIGITS_MULTICORE
    class HelperThread_t
    {
    public:
        // Constructor
        // rt_prio should be set to the priority we want the thread to be
        HelperThread_t(VstCore* core, Voice::voiceBuffers_t* bufs, int rt_prio);
        ~HelperThread_t();
        
        // Signal the helper thread to start processing a chunk. Call from core.
        void StartProcessChunk(int len, int forBuf)
        {
            DEBUG_MCORE("c: StartProcessChunk: get lock\n");
            pthread_mutex_lock(&m_beginChunkMutex);
            DEBUG_MCORE("c: StartProcessChunk: did processing = false, for buffer=%d\n", forBuf);
            m_curBuf = forBuf;
            m_chunkLen = len;
            m_didProcessing = false;
            pthread_mutex_unlock(&m_beginChunkMutex);
            DEBUG_MCORE("c: StartProcessChunk: release lock\n");
            pthread_cond_signal(&m_beginChunkCond);
            DEBUG_MCORE("c: StartProcessChunk: signal helper for %u\n", forBuf);
        }
        bool DidProcessing() { return m_didProcessing; }
        // Call from core
        void Quit()
        {
            DEBUG_MCORE("Shutting down helper\n");
            m_quitNow = true;
            StartProcessChunk(0, 0);
            pthread_join(m_thread, 0);
            DEBUG_MCORE("Done shutting down helper\n");
            m_quitNow = false;
        }

        pthread_mutex_t* GetMutex() { return &m_beginChunkMutex; }

        // Do not call directly. This is called by the thread callback.
        void ThreadCore();
        void SetOutputBuffer(float* buf) { m_outBuf = buf; }

    private:
        pthread_mutex_t m_beginChunkMutex;
        pthread_cond_t m_beginChunkCond;
        pthread_t m_thread;
        Voice::voiceBuffers_t* m_bufs;
        float* m_outBuf;
        int m_chunkLen;
        VstCore* m_vstCore;
        bool m_quitNow;
        // True if processing happened this frame
        bool m_didProcessing;
        unsigned int m_curBuf;
    };
#endif
    
	VstCore(audioMasterCallback audioMaster);
	~VstCore();

	// VST functions 
	virtual VstInt32 getProgram () { return m_curProgram; }
	virtual void setProgram(VstInt32 program);
	virtual void getProgramName(char *name);
	virtual void setProgramName(char *name);
    // The non-VST SetProgramName. We use this because it's better, and Digits won't let
    // you permanently rename patches from solely within the DAW anyway/
    // SetProgram is also a non-VST variant here (notice capitalization). Both these functions
    // are guaranteed to be safe to call without triggering any DAW-side behavior-- they're
    // solely superficial and are used to keep the UI in sync.
    void SetProgramName(std::string name) { m_curProgramName = name; }
    void SetProgram(int program) { m_curProgram = program; }
	virtual void processReplacing(float **ins, float **outs, VstInt32 frames);
	virtual VstInt32 processEvents(VstEvents *events);
	virtual VstInt32 canDo(char *text);
	virtual void setParameter(VstInt32 index, float value);
	virtual float getParameter(VstInt32 index);
	virtual void getParameterLabel(VstInt32 index, char *text);
	virtual void getParameterDisplay(VstInt32 index, char *text);
	virtual void getParameterName(VstInt32 index, char *text);
	virtual void setSampleRate(float sR);
	virtual void setBlockSize(VstInt32 blockSize);
	virtual bool getOutputProperties(VstInt32 index, VstPinProperties* prop);
	virtual bool getEffectName(char *name);
	virtual bool getVendorString(char *text);
	virtual bool getProductString(char *text);
	virtual VstInt32 getNumMidiInputChannels() { return 1; }
	virtual VstInt32 getNumMidiOutputChannels() { return 0; }
	void HandleMidiEvent(char* midiEvent);
    // Set chorus for either chorus or flanger operation
    void SetChorusFlanger(bool isChorus);

	// Save/Load functions
	virtual VstInt32 getChunk(void** data, bool isPreset);
	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset = false);
    bool LoadFXP(const std::string& fn);
	bool SaveFXP(const std::string& fn);

    PatchBankList& GetPatchBankList() { return m_pbList; }
    
	void NoteOn(int pitch, int velocity);
	void NoteOff(int pitch);
    void ReleaseAllHeldNotes();
    
    bool VoiceIsFree(int voiceNum) { return m_voices[voiceNum]->IsFree(); }
    bool VoiceIsSubVoice(int voiceNum) { return m_voices[voiceNum]->IsSubVoice(); }
	void VoiceRender(int voiceNum, float *out, int len, Voice::voiceBuffers_t *bufs, int debugvoice, int debugthread)
    {
        m_voices[voiceNum]->Render(out, len, bufs, true, debugthread);
    }
    
    unsigned int CurBuf() { return m_curBuf; }

#ifdef DIGITS_MULTICORE
    // Allocate a helper thread and start it. This routine is not very smart, so use
    // it carefully.
    void StartHelper(int helperIndex);
#endif
    
    void AllNotesOff();
    void ResetPhases();
    void ResetEngine();
    
private:
	static const int kNumVoices = 40;
	static const int kOctaves = 10; // Octaves that there are deltas for
	static const char* s_paramNamesChunk[];
	    
	int m_curProgram;
    std::string m_curProgramName;
    
    // This is for the editor window, but must persist between
    // editor window allocation/reallocations
    PatchBankList m_pbList;
    
	Voice *m_voices[kNumVoices];
	Chorus m_chorus[2];
    // Chorus or flanger mode?
    bool m_chorusMode;
//    Oversampler m_oversampler;
//    int m_oversampleAmt;
    
	typedef std::vector<VstMidiEvent> midiEvents_t; 
	std::vector<VstMidiEvent> m_midiEvents;
	static float s_freqDelta[kOctaves*12];
	// Orch (spread, supersaw, etc) mode variance amount
	float m_orch;
	// Overall gain of the sound
	float m_gain;
    // Voice is set to monophonic
    bool m_isMono;
    // Voice is set to always glide
    bool m_alwaysGlide;
    
	// Calculate deltas for each frequency (tied to sampling rate)
	void CalculateFreqDeltas();
	// Get delta for MIDI note number
	float GetDelta(VstInt32 midiNoteNum);
	// Find lowest key pressed (used in mono mode)
	int FindLowestNote();

    // 2 for now (dual core)
    Voice::voiceBuffers_t m_bufSets[2];
    // The helper's extra buffers
    // 1 for now (dual core)
    SndBuf* m_helperOutBufs[1];
    
    SndBuf* m_tempBuf;
    
	// Bitfield for which keys are on (mono mode)
	uint32_t m_keyBits[4];

    // State of MIDI sustain pedal
    bool m_sustainDown;
	
	// Frames left to write within the control rate-sized buffer
	// left to write out to the host's buffer, preserved
	// between processReplacing calls
	int m_writeLeft;
	BufferManager m_manager;
	
	// True if this key is held down (keybits)
	inline bool IsNoteOn(int pitch);

	// Very simple store of floating point settings
	// for our program data
	float m_simpleProgram[kNumParams];

	// Voice allocation and stealing algorithm
	int NextFreeVoice();
	
	// Count up on each key press so that we can
	// figure out which notes are oldest
	uint64_t m_timestampClock;
	// Where does the next control period start? This is carried
	// over from buffer to buffer.
	int m_nextSpan;
	
	// Dynamically allocated space in which to save one patch.
	// FXBs are not supported. This is only dynamic because the 
	// patch length varies because it's string based, but only
	// varies between versions. It calculates the patch len on save.
	char *m_patchData;
	
	// Last status byte for "running status" MIDI feature
	int m_lastStatus;
	
	// DC filter coefficients
	float m_dc_a[2];
	float m_dc_b[2];
	float m_dc_out[2];
	float m_dc_in[2];

	// Delay lines for echo
	float *m_delL;
	float *m_delR;
	// Read/write pointers
	int m_del_wL;
	int m_del_wR;
	// Wrap for pointer index in samples 
	int m_del_wrapL;
	int m_del_wrapR;
	float m_delWet;
	float m_delFB;
	// Last sample for high frequency decay (4-pole)
	float m_del_lastL[4];
	float m_del_lastR[4];
	float m_del_filterA;
	float m_del_filterB;
    // Delay lines for chorus
    
    
    // Lite mode for phat detune
    bool m_phatLite;
	
    // Last note-on pitch recorded (for always-glide mode)
    int m_lastPitch;
    
    // Pitch bend amounts in semitones
    int m_bendUpAmt;
    int m_bendDnAmt;
    
    // True if the oscillator phases are reset upon note on
    bool m_resetOscs;
    
#ifdef DIGITS_MULTICORE
    HelperThread_t* m_helpers[1];
#endif
    
    int m_numCores;

    void PitchBend(float value);
	void ZeroPatch();
	
	void ApplyDC(float *in, int len);
    
    // Current chunk buffer's number (keep incrementing; wrap-around is OK)
    // This is used to prevent the helper thread from waking up too late and
    // performing work too early on the following frame, which knocks it out of sync
    unsigned int m_curBuf;
    
    SndBuf *m_chorusDryBuf;
};

#endif

