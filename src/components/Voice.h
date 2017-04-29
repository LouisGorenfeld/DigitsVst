/* Phase Distortion Synthesizer (c) 2011 Louis Gorenfeld <louis.gorenfeld@gmail.com> */

#ifndef _VOICE_H_
#define _VOICE_H_

#include <stdint.h>
#include "SndBuf.h"
#include "PhaseDist.h"
#include "ResoGen.h"
#include "Contour.h"
#include "FilterBank.h"
#include "LFO.h"

class Voice
{
public:
    struct voiceBuffers_t
    {
        SndBuf* m_voiceBuf;
        SndBuf* m_shpEnvBuf;
        SndBuf* m_ampEnvBuf;
        SndBuf* m_lfoBufs[Tables::kNumLFOs];
        SndBuf* m_fmScratchBuf;
	};

	// These probably could be moved to VstCore
	static float kAttackMinimumAmp;
	static float kAttackMinimumShaper;
	static float kAttackRange;
	static float kFadeMinimum;
	static float kFadeRange;
	static float kReleaseMinimum;
	static float kReleaseRange;
	static float kDecayMinimum;
	static float kDecayRange;
	static float kDetuneMaximum;
	static float kLFOMinimum;
	static float kLFORange;
	static float kPWMMinimum;
	static float kPWMRange;
	
    // voiceNumber = the number of the voice this is (for debug purposes)
	Voice(int voiceNumber);
	~Voice();
	
	// Set doContourGen to false when rendering as a subvoice
    // Buffers must be passed in at render-time because multicore support needs separate buffers for each
    // helper thread
    // doContourGens is true if the contour generators are supposed to be renderd. False if we're intentionally
    // rendering a subvoice.
    // debugthread is the number of the thread (0=core, 1=helper 1, 2=helper 2, etc) for debug purposes
	void Render(float *out, int len, Voice::voiceBuffers_t* bufs, bool doContourGens, int debugthread);
   // BUFFER SET IS FOR: fmIn, SndBuf *shaperBuf, SndBuf *ampBuf, SndBuf **lfoBufs, bool doContourGens);
	void NoteOn(uint64_t timestamp, float deltaLo, float deltaHi, float delta, int midiKey, int velocity, bool reset=true);
	// Pass in a negative value to turn the note off regardless of pitch
	void NoteOff(int key);
	// Pass in -1.0 to 1.0
	void PitchBend(float value);
    // Pass in 0 to 1.0
    void SetModWheel(float value) { m_modWheel = value; }
    // Pass in 0 to 1.0
    void SetAftertouch(float value) { m_aftertouch = value; }
	// Notify the Voice of the current sampling rate
    void SetSamplingRate(float sR) { m_sR = sR; }
    
	void ResetPhases()
	{	
		m_resoGen.ResetPhase();
		m_pdOsc1.ResetPhase();
		m_pdOsc2.ResetPhase();
	}
	
	// Return true if this voice can be stolen
	// or reused
	bool IsFree()
    {
        return m_ampEnv.IsDone();
    }
	bool IsReleased() { return m_ampEnv.IsReleased(); }
	int Pitch() { return m_pitch; }
	void SetParameter(int index, float value);
	inline float Clip(float x);
	uint64_t GetTimestamp() { return m_timestamp; }
	
	void InitiateGlide(float delta, float deltaHi, float deltaLo);
	
	// Set ties between velocity and amplitude/shaper 
	void SetVelocityAmp(float amt) { m_velAmp = amt; }
	void SetVelocityShp(float amt) { m_velShp = amt; }
	
	void SetPulseWidths();
	void SetBasisWaves();
	
	// At the beginning of buffer processing, these are all true. As voices are rendered,
	// needsRender is set to false.
    bool NeedsRender() { return m_needsRender; }
    // In multicore mode, make sure this is wrapped in a mutex. It's not individually
    // wrapped because that's wasteful, kinda like individually-wrapped M&Ms.
	void SetNeedsRender(bool needsRender=true)
    {
        m_needsRender = needsRender;
    }
    void SetIsSubVoice(bool isSubVoice) { m_isSubVoice = isSubVoice; }
    bool IsSubVoice() { return m_isSubVoice; }
    
	// Set the subvoice pointer (for optimization of orch/phat/unison mode)
	void SetNextSubVoice(Voice* nextSubVoice)
    {
        fprintf(stderr, "SetNextSubVoice %p -> %p\n", this, nextSubVoice);
        m_nextSubVoice = nextSubVoice;
        if (nextSubVoice)
            nextSubVoice->SetIsSubVoice(true);
    }
	
	// Recursively go through the subvoice list and turn off all the envelopes (disable voice)
	void DisableSubVoices()
	{
		m_ampEnv.SetDone();
		if (m_nextSubVoice)
			m_nextSubVoice->DisableSubVoices();
	}
	
    // When we engage the filter, we lock the shaper at a
    // specific value. This is the amount it's locked to.
    void LockShaperTo(float val) { m_lockShaperAmt = val; }
	
    void UpdateFilterSamplingRate(float sR) { m_trueFilter.SetSamplingRate(sR); }
    
    // Set influence for aftertouch and modwheel
    void SetAftertouchShaper(float value) { m_atouchShp = value; }
    void SetAftertouchLFO1Amp(float value) { m_atouchLFO1Amp = value; }
    void SetModWheelShaper(float value) { m_modShp = value; }
    void SetModWheelLFO1Amp(float value) { m_modLFO1Amp = value; }
    
    bool ShaperLocked() { return (m_lockShaperAmt > 0); }
    
    // Divider should be in the range of 1 to 8, or 0 for off.
//    void SetBitCrushDivider(float div) { m_bitCrushDivider = div; }
//    void SetBitCrushBits(int bits) { m_bitCrushBits = pow(2, bits); }
    
private:
#ifdef DIGITS_MULTICORE
    // Mutex is locked when setParameter
    pthread_mutex_t m_mutex;
#endif
    
    enum GlideMode
    {
        kGlide_PolyNoGlide,
        kGlide_PolyGlideAlways,
        kGlide_MonoGlideWhenHeld,
        kGlide_MonoGlideAlways
    };
    
	uint64_t m_timestamp;
	
	Contour m_ampEnv;
	Contour m_shaperEnv;
	PhaseDist m_pdOsc1;
	PhaseDist m_pdOsc2;
	LFO m_lfos[Tables::kNumLFOs];
	
	ResoGen m_resoGen;
	// MIDI pitch value used for note on/offs
	int m_pitch;
	float m_detune;
	float m_octave;
	float m_mix;
	float m_resoVol;
	float m_pulseWidth;
	float m_lfoAmplitude[Tables::kNumLFOs]; // How much LFO affects the amplitude
	
	// Pitch delta
	float m_delta;
	// Pitch bend down delta
	float m_deltaLo;
	// Pitch bend up delta
	float m_deltaHi;
    
    // The same, but for notes that we're sliding to (monophonic mode)
	float m_delta2;
	float m_deltaLo2;
	float m_deltaHi2;
    // The position from 0 to 1 between the two pitches we're at
    float m_deltaBlend;
    // How long it takes for a portamento (glide) to finish. This is in terms of control
    // rate frames. At the time of writing, the control rate is 1000/sec, so milliseconds.
    float m_glideSpeed;
    // The glide mode
    GlideMode m_glideMode;
    
	// Last pitch wheel value
	float m_bend;
    // Last mod wheel value
    float m_modWheel;
    // Last aftertouch value
    float m_aftertouch;
	// Velocity
	float m_velocity;
	
    int m_basisWave; // 0 = cos, 1 = sin
	
	// These are stored here because we want to set these per
	// note (for tying velocity to brightness)
	float m_shpMin;
	float m_shpMax;
	float m_velShp;
	float m_velAmp;
    
    float m_modShp;
    float m_modLFO1Amp;
    float m_atouchShp;
    float m_atouchLFO1Amp;
    
	// Calculate pitch based on frequency deltas and pitch wheel value
	void CalcPitch();
	// Recalculate envelopes (for sample rate changes)
	void RecalcEnvs() {} // TODO

	// Set to null when there's no more subvoices
	Voice *m_nextSubVoice;
	// This is so that the main render loop doesn't end up
	// rendering this voice twice (if it's a subvoice)
	bool m_needsRender;
    // This is a sub-voice (phat/unison/orch mode)
    bool m_isSubVoice;
	
	LowPassReso m_trueFilter;
    // When we engage the filter, we lock the shaper at a
    // specific value. This is the amount it's locked to.
    float m_lockShaperAmt;
    
    // Current sampling rate
    float m_sR;
    
    // The number of the voice for debugging purposes
    int m_voiceNumber;
    
    // Bitfield that shows the LFO assignments. When it's 0, it's
    // safe not to process that LFO
    static const int kLFOa_toShaper = 1;
    static const int kLFOa_toPWM = 2;
    static const int kLFOa_toAmp = 3;
    static const int kLFOa_toPitch = 4;
    int m_lfoIsAssigned[Tables::kNumLFOs];
    
    // Envelope settings, for rate/level scaling upon note-on
    // Rate and level scaling, in terms of
    // speedup or attenuation per octave. Ramps
    // based on frequency. e.g., .5 for level would
    // attenuate the envelope by .5 per octave. 2 for
    // rate would double the envelope speed each octave,
    // starting at 60hz.
    float m_ampRateScale;
    float m_ampLevelScale;
    float m_ampAttack;
    float m_ampDecay;
    float m_ampRelease;
    float m_shpRateScale;
    float m_shpLevelScale;
    float m_shpAttack;
    float m_shpDecay;
    float m_shpRelease;
    
    // Sample-hold for the smart bitcrush
    float m_bitCrushPos;
    float m_bitCrushDelta;
    int m_bitCrushLastSample;
    float m_bitCrushBlend;
    float m_bitCrushDivider;
    int m_bitCrushBits;
};

#endif

