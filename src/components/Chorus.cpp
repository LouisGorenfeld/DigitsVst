#include <cmath>
#include <stdlib.h> // rand
#include "Chorus.h"

Chorus::Chorus() :
	m_delBuf(0),
    m_wIndex(0),
    m_lfoPos(0),
    m_lfoDelta(0),
    m_lfoRate(0),
    m_minDelay(0),
    m_range(0),
    m_minDelaySamp(0),
    m_rangeSamp(0),
    m_lastOut(0),
    m_feedback(0),
    m_sR(0),
	m_wet(0),
    m_bufSize(0),
    m_bins(512),
    m_shLast(0),
    m_shPos(0),
    m_noiseAmt(1.0f/256.0f)
{
    m_preLP[0].SetFrequency(6000);
    m_preLP[1].SetFrequency(6000);
    m_postLP[0].SetFrequency(4000);
    m_postLP[1].SetFrequency(4000);
    m_postHP.SetFrequency(500);
    m_triSmoother.SetFrequency(5);
    SetSamplingRate(44100);     // Just so that m_delBuf gets initialized
}

Chorus::~Chorus()
{
}

void Chorus::SetSamplingRate(float sR)
{
	m_sR = sR;
	if (m_delBuf)
		delete[] m_delBuf;
	m_delBuf = new float[(int)sR+1];
    for (int i=0; i < sR+1; i++)
        m_delBuf[i] = 0;
	m_bufSize = sR;
    m_preLP[0].SetSamplingRate(sR);
    m_preLP[1].SetSamplingRate(sR);
    m_postLP[0].SetSamplingRate(sR);
    m_postLP[1].SetSamplingRate(sR);
    m_postHP.SetSamplingRate(sR);
    m_triSmoother.SetSamplingRate(sR);
    m_wIndex = 0;
    m_lfoPos = 0;
    m_shPos = 0;
}

void Chorus::RenderWet(float *in, float *out, int len)
{
//    for (int i=0; i < len; i++)
//        in[i] = ((rand()&65536)/32767.0) - 1.0;
	// TODO: This could be made a lot more efficient
    
    m_preLP[0].Render(in, len);
    m_preLP[1].Render(in, len);
    
	for (int i=0; i<len; i++)
	{
        float lfoSam;
        m_lfoPos += m_lfoDelta;
        while (m_lfoPos >= 1.0f)
            m_lfoPos -= 1.0f;

        if (m_lfoPos >= .5f)
            lfoSam = 2.0f - (m_lfoPos*2.0f);
        else
            lfoSam = m_lfoPos * 2.0f;
        lfoSam = m_triSmoother.Tick(lfoSam);
        
		float delay = m_minDelaySamp + (lfoSam*m_rangeSamp);
		float rIndex = (float)m_wIndex - delay;
		while (rIndex < 0)
			rIndex += m_bufSize;
        
 //       fprintf(stderr, "delay=%d rIndex=%d, wIndex=%d\n", (int)delay, (int)rIndex, (int)m_wIndex);
		
		float rIndexFrac = fmod(rIndex, 1.0f);
		int rIndexInt = (int)rIndex;
		int rIndexInt2 = (int)rIndex - 1;
		while (rIndexInt2 < 0)
			rIndexInt2 += m_bufSize;

        // Sample hold to emulate bucket brigade
        float shDelta = (m_bins / (delay / m_sR)) / m_sR; // is this too slow? does m_sR cancel? :)
        if (shDelta >= 1.0f)
        {
            float n = ((rand() & 65535) - 32767.0) / (32767.0f);
            // TODO: Optimize noise
            m_shLast = in[i] + m_lastOut * m_feedback + n * m_noiseAmt;
        }
        else
        {
            m_shPos += shDelta;
            while (m_shPos >= 1.0f)
            {
                // TODO: Anti-aliasing pre-filter
                // TODO: Optimize noise
                float n = ((rand() & 65535) - 32767.0) / (32767.0f);
                m_shLast = in[i] + n * m_noiseAmt;
                m_shPos -= 1.0f;
            }
        }
        
		m_delBuf[m_wIndex] = m_shLast;
		// Linear interpolation between index int 1 & 2 depending
		// on amount of fraction
		out[i] = (m_delBuf[rIndexInt]*rIndexFrac + m_delBuf[rIndexInt2]*(1-rIndexFrac));
        m_lastOut = out[i];
        out[i] *= m_wet;
		m_wIndex = (m_wIndex + 1) % (m_bufSize);
	}

    m_postLP[0].Render(out, len);
    m_postLP[1].Render(out, len);
    m_postHP.Render(out, len);
}

