#ifndef _CHORUS_H_
#define _CHORUS_H_

#include "FilterBank.h"

class Chorus
{
public:
	Chorus();
	~Chorus();
	void SetSamplingRate(float sR);
	void RenderWet(float *in, float *out, int len);
	void SetMinDelay(float minDelay) { m_minDelay = minDelay; RecalcDelays(); }
    float GetMinDelay() { return m_minDelay; }
	void SetRange(float range) { m_range = range; RecalcDelays(); }
    float GetRange() { return m_range; }
    void SetLFORate(float hz) { m_lfoRate = hz; RecalcDelays(); }
	void SetWet(float wet) { m_wet = wet; }
    float GetWet() { return m_wet; }
	void SetBucketBins(int n) { m_bins = n; }
    void SetNoiseAmount(float x) { m_noiseAmt = x; }
    void SetOffset(float offset) { m_lfoPos = offset; }
    void SetFeedback(float fb) { m_feedback = fb; }
    float GetFeedback() { return m_feedback; }
    void SetTone(float prelp, float postlp, float hp)
    {
        m_preLP[0].SetFrequency(prelp);
        m_preLP[1].SetFrequency(prelp);
        m_postLP[0].SetFrequency(postlp);
        m_postLP[1].SetFrequency(postlp);
        m_postHP.SetFrequency(hp);
    }
	void RecalcDelays()
	{
		m_minDelaySamp = (m_minDelay/1000.0f)*m_sR;
		m_rangeSamp = (m_range/1000.0f)*m_sR;
        m_lfoDelta = m_lfoRate/m_sR;
	}
	
private:
	// Delay buf. This should be enough
	// for a second and needs to be manually
	// resized when the buffer size changes
	// (not part of SndBuf's regular size).	
	float* m_delBuf;
	// Write ptr
	int m_wIndex;
	
    float m_lfoPos;
    float m_lfoDelta;
    float m_lfoRate;
    
	float m_minDelay;
	float m_range;
	float m_minDelaySamp;
	float m_rangeSamp;
    float m_lastOut;
	
	float m_feedback;
	float m_sR;
	
	float m_wet;
	int m_bufSize;

	// Sample-hold for bucket brigade emulation
	float m_bins;
    float m_shLast; // last sample to hold
	float m_shPos;
    
    // Noise amount
    float m_noiseAmt;

	// Variables for pre and post anti-aliasing filters
	// (more bucket brigade emulation)
	// 6dB at 6khz
    LowPass1p m_preLP[2];
    LowPass1p m_postLP[2];
    LowPass1p m_triSmoother;
    HighPass m_postHP;
};

#endif
