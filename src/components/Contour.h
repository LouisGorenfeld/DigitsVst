/* Phase Distortion Synthesizer (c) 2011 Louis Gorenfeld <louis.gorenfeld@gmail.com> */


#ifndef _CONTOUR_H_
#define _CONTOUR_H_

#include <math.h>
#include "SndBuf.h"
#include "BufferManager.h"

// Linear trapezoidal contour generator
// (wow, that makes it sounds so much more complex than it is!)
class Contour
{
public:
	Contour();
	~Contour() {}

	// Set attack in ms
	void SetAttackExponential(float a, float sR)
	{
		float sams = (a/1000.0)*sR;
		m_attack = pow((float)M_E, (float)-1.0f/sams);
		m_attack1 = 1.0f - m_attack;
	}
    // Set Decay in ms, return seconds Release
	float SetDecayExponential(float d, float sR)
	{
		float sams = (d/1000.0)*sR;
		m_decay = pow((float)M_E, (float)-1.0f/sams);
		m_decay1 = 1.0f - m_decay;
        m_decay1TimesSusLevel = m_decay1*m_sustainLevel;
		
		sams = log(.001)/log(m_release);
		return (sams*1000.0f) / sR;
	}
	// Set Release in ms, return seconds Release
	float SetReleaseExponential(float d, float sR)
	{
		float sams = (d/1000.0)*sR;
		m_release = pow((float)M_E, (float)-1.0f/sams);
		m_release1 = 1.0f - m_release;
		
		sams = log(.001)/log(m_release);
		return (sams*1000.0f) / sR;
	}
	void SetFadeDirect(float f)
	{
		m_fade = f;
		m_fade1 = 1.0f - m_fade;
		m_sustain = (f > 0);
	}
	void SetFadeExponential(float f, float sR) 
	{
		float sams = (f/1000.0)*sR;
		m_fade = pow((float)M_E, (float)-1.0f/sams);
		m_fade1 = 1.0f - m_fade;	
		m_sustain = (f > 0);
	}
    // level is amplitude from 0 to 1.0
    void SetSustainLevel(float level)
    {
        m_sustainLevel = level;
        m_decay1TimesSusLevel = m_decay1*m_sustainLevel;
    }
	void SetMinimum(float min) { m_min = min; }
	void SetMaximum(float max) { m_max = max; }
    // Render REPLACING
	void Render(float *out, int len);
	SndBuf::sam_t GetValue() { return m_curValue; }

	void GateOn(bool reset=true) 
	{ 
		if (reset)
        {
            m_lastOut = 0;
			m_curValue = m_noAttack? 1.0f : 0.0f;
        }
		m_phase = kAttack;
	}
	void GateOff() 
	{ 
		if (m_phase != kFinished)
		{
			m_phase = kRelease;
		}
	}
	bool IsDone() { return m_phase == kFinished; }
	bool IsReleased() { return m_phase == kRelease; }
	void SetDone() { m_phase = kFinished; }
	inline float Clip(float x);
	
	void NoAttack(bool on=true) { m_noAttack = on; }
    
    // A control (such as modwheel) can be hooked to this to control
    // the brightness of the sound. The envelope wave is then multiplied
    // by it. It is smoothed to prevent clicking.
    inline void SetControlInput(float in)
    {
        m_ctlIn = in;
    }
	
private:
	enum phase_t
	{
		kAttack,
		kDecay,
		kSustain, // ..and fade
        kRelease,
		kFinishing,
		kFinished,
	};
	
	SndBuf::sam_t m_curValue;
	float m_attack;
    float m_decay;
	float m_release;
	float m_fade;
	bool m_sustain;
    float m_sustainLevel;
	float m_min;
	float m_max;
	phase_t m_phase;
    // Discrete control value input here to control brightness
    float m_ctlIn;
    // Smoothed control value
    float m_ctlVal;
	
	// Coefficients and such the one-pole exponential glide filter
	float m_attack1;
    float m_decay1;
	float m_release1;
	float m_fade1;
	float m_lastOut;
    
    // m_decay1 * m_sustainLevel used for envelope decay calculation
    float m_decay1TimesSusLevel;
	
	bool m_noAttack;
};

#endif


