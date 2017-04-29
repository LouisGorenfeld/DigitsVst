/* Phase Distortion Synthesizer (c) 2011 Louis Gorenfeld <louis.gorenfeld@gmail.com> */

#ifndef _RESOGEN_H_
#define _RESOGEN_H_

#include "SndBuf.h"

class ResoGen
{
public:
	enum resoStyle_t
	{
		kSawQuarter,
		kSawHalf,
		kSquareQuarter,
		kSquareHalf
	};
	
	ResoGen();
	~ResoGen() {}
	void SetPitch(float delta) { m_delta = delta; }
	
	void SetStyle(resoStyle_t style) { m_style = style; }
	
	// Render ADDING one buffer of audio (control rate chunk)
	void Render(float *out, int len, float volume=1.0f);
	
	// Brightness of the tone (filter cutoff)
//	void SetBright(float bright) { m_bright = 2.0f + (bright * 14.0f); }

	// Reset phase of oscillators
	void ResetPhase();
	
	void SetPulseWidth(float width) { m_pw = width; }
	void SetBuffers(SndBuf *ampBuf, SndBuf *shpBuf, SndBuf **lfoBufs)
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
	
private:
	float *m_ampBuf;
	float *m_shpBuf;
	float *m_lfoBufs[Tables::kNumLFOs];
	float m_lfoShaper[Tables::kNumLFOs];
	float m_lfoPitch[Tables::kNumLFOs];
	float m_lfoPWM[Tables::kNumLFOs];
	
	resoStyle_t m_style;
	float m_delta;
	// Position of slave (sine)
	float m_pos1;
	// Position of master (window)
	float m_pos2;
	// Multiplier for slave frequency
//	float m_bright;
	// Wave inversion for square patterns
	float m_flip;
	float m_pw;
};

#endif
