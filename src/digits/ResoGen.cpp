/* Phase Distortion Synthesizer (c) 2011 Louis Gorenfeld <louis.gorenfeld@gmail.com> */

#include <cmath>
#include "Tables.h"
#include "ResoGen.h"

ResoGen::ResoGen() :
    m_ampBuf(0),
    m_shpBuf(0),
	m_style(kSawQuarter),
    m_delta(0),
	m_pos1(0),
	m_pos2(0),
	m_flip(1.0f),
	m_pw(.5)
{
    m_lfoBufs[0] = m_lfoBufs[1] = m_lfoBufs[2] = 0;
    m_lfoShaper[0] = m_lfoShaper[1] = m_lfoShaper[2] = 0;
    m_lfoPWM[0] = m_lfoPWM[1] = m_lfoPWM[2] = 0;
    m_lfoPitch[0] = m_lfoPitch[1] = m_lfoPitch[2] = 0;
}

void ResoGen::ResetPhase()
{
	m_pos1 = 0;
	m_pos2 = 0;
	m_flip = 1.0f;
}

void ResoGen::Render(float *out, int len, float volume)
{
	float* ampContour = m_ampBuf;
	float* brightContour = m_shpBuf;
	
	// Match the volume of the normal oscillator
	volume *= 2.0f;
	
	if (!ampContour || !brightContour || !out)
		return;
	
	// Calculate PWM shaper
	float m3 = .5f / m_pw;
	float m4 = (.5f) / (1.0f - m_pw);
	float b4 = 1.0f - m4;	
	float pwmPos;
	if (m_pos2 < m_pw)
		pwmPos = m3*m_pos2;
	else
		pwmPos = m4*m_pos2 + b4;
	
	float sam;
	
	for (int i=0; i<len; i++)
	{
		// Saw style
		float bright = .5f + ((brightContour[i]-.5f) * 18.0f);
#ifdef DIGITS_PRO
        float lfoInfluence = ((m_lfoBufs[0][0]+1.0f)*.5f) * m_lfoShaper[0]
        + ((m_lfoBufs[1][0]+1.0f)*.5f) * m_lfoShaper[1]
        + ((m_lfoBufs[2][0]+1.0f)*.5f) * m_lfoShaper[2];
#else
        float lfoInfluence =  ((m_lfoBufs[0][0]+1.0f)*.5f) * m_lfoShaper[0];
#endif
		bright -= lfoInfluence;
		if (m_style == kSawQuarter || m_style == kSquareQuarter) { // Saw quarter cosine
			sam = Tables::Sin(m_pos1 * bright) * .5f * Tables::Cos(pwmPos * .25f) * volume * ampContour[i] * m_flip;
			for (int j=0; j<2; j++) 
			{
				sam = Tables::SoftClip(sam);
			}
		}
		else if (m_style == kSawHalf || m_style == kSquareHalf) // Saw half sine
			sam = ((Tables::Sin(m_pos1 * bright) * .5f) + 1.0f) * Tables::Sin(pwmPos * .5f) * volume * ampContour[i] * m_flip;
		else
			sam = 0; // Error condition!
		
		*out++ += sam;
		
        lfoInfluence = 0;
        if (m_lfoPitch[0])
            lfoInfluence += m_lfoBufs[0][i] * m_lfoPitch[0];
        if (m_lfoPitch[1])
            lfoInfluence += m_lfoBufs[1][i] * m_lfoPitch[1];
        if (m_lfoPitch[2])
            lfoInfluence += m_lfoBufs[2][i] * m_lfoPitch[2];
		m_pos1 += m_delta + (m_delta * lfoInfluence);
		m_pos2 += m_delta + (m_delta * lfoInfluence);
		while (m_pos1 >= 1.0f)
			m_pos1 -= 1.0f;
		while (m_pos2 >= 1.0f)
		{
			m_pos2 -= 1.0f;
			if (m_style == 2 || m_style == 3)
				m_flip *= -1.0f;
		}
		if (m_pos2 < m_pw)
			pwmPos = m3*m_pos2;
		else
			pwmPos = m4*m_pos2 + b4;		
	}
}
