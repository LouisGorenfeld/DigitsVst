/* Phase Distortion Synthesizer (c) 2011 Louis Gorenfeld <louis.gorenfeld@gmail.com> */

#ifndef _PHASEDIST_
#define _PHASEDIST_

#include <math.h>
#include "SndBuf.h"
#include "BufferManager.h"

class PhaseDist 
{
public:
	PhaseDist();
	~PhaseDist() {};

	// Set the pitch in hz/sR (delta)
	void SetPitch(float delta)
    {
        m_delta = delta;
        m_deltaDest = delta;
    }

	void SetGlideDest(float delta) { m_deltaDest = delta; }
	
	// Render ADDING one buffer of audio (control rate chunk)
	void Render(float *fmIn, float *out, int len, float volume);
	
	// Brightness of the tone (filter cutoff)
//	void SetBright(float bright) { m_bright = m_skew - (bright * m_skew); }

	// Set FM input source (affects shaper position)
	// 0 if unused
//	void SetFMInput(SndBuf* fmInput) { m_fmInput = fmInput; }

	// Set FM input amount (affects shaper position)
	void SetFMInputAmt(float amt) { m_fmInputAmt1 = amt; }

	// How many times per cycle the shaper runs (2x for square)
	void SetShaperMod(float mod) { m_shaperMod = mod; }

	// Set skew point (.5 is normal for most CZ waves)
	void SetSkew(float skew) { m_skew = skew; }

	// Set basis waveform
	void SetBasis(int basis) { m_basisWave = basis; }
	
	void ResetPhase() { m_pos = 0; m_detailCounter = 0; }
	
	void SetGlideSpeed(float ms, float sR)
	{
		float sams = (ms/1000.0)*sR;
		m_glide = pow((float)M_E, (float)-1.0f/sams);
		m_glide1 = 1.0f - m_glide;
	}
	
	void SetPulseWidth(float width) { m_pwmWidth = width; }
	void SetBuffers(SndBuf* ampBuf, SndBuf* shpBuf, SndBuf** lfoBufs)
	{
		m_ampBuf = ampBuf->Data();
		m_shpBuf = shpBuf->Data();
		m_lfoBufs[0] = lfoBufs[0]->Data();
		m_lfoBufs[1] = lfoBufs[1]->Data();
		m_lfoBufs[2] = lfoBufs[2]->Data();
	}
	
	void SetLFOShaper(int lfo, float value) { m_lfoShaper[lfo] = value; }
	void SetLFOPitch(int lfo, float value) { m_lfoPitch[lfo] = value; }
	void SetLFOPWM(int lfo, float value) { m_lfoPWM[lfo] = value; }
	
	// Get delta with modulation factored in (for keyscaling)
	float GetDelta() { return m_delta; }
	// Set keyscaling amount for shaper (shaper attenuation)
	void SetKeyScale(float x) { m_keyScale = x; }
    // See Voice for documentation on LockShaperTo
	void LockShaperTo(float val) { m_lockShaperAmt = val; }
    
private:
	// Recalc slopes only every N samples
	static const int kDetailLevel = 16;
	
	float *m_ampBuf;
	float *m_shpBuf;
	float *m_lfoBufs[Tables::kNumLFOs];
	
	float m_fmInputAmt;
	float m_fmInputAmt1; // zipper noise smoothing (new value)
	float m_shaperMod;
	float m_skew;

	float m_delta;
	// Delta destination (for slides)
	float m_deltaDest;
	// Coefficients for one-pole smoother (for slides)
	float m_glide;
	float m_glide1;
	float m_pos;
	int m_basisWave;
	float m_pwmWidth;
	
	// LFO severtity
	float m_lfoShaper[Tables::kNumLFOs];
	float m_lfoPitch[Tables::kNumLFOs];
	float m_lfoPWM[Tables::kNumLFOs];
	
	// Shaper keyscaler
	float m_keyScale;
	// Lock shaper to this value (see Voice)
    float m_lockShaperAmt;
    
	int m_detailCounter;
    
};

#endif
