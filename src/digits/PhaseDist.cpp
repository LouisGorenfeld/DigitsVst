/* Phase Distortion Synthesizer (c) 2011 Louis Gorenfeld <louis.gorenfeld@gmail.com> */

#include <math.h>
#include "PhaseDist.h"
#include "Voice.h" // PWM min/max defines
#include "Tables.h"

PhaseDist::PhaseDist() :
    m_ampBuf(0),
    m_shpBuf(0),
    m_fmInputAmt(0),
    m_fmInputAmt1(0),
    m_shaperMod(0),
    m_skew(0),
    m_delta(0),
    m_deltaDest(0),
    m_glide(0),
    m_glide1(0),
    m_pos(0),
    m_basisWave(0),
    m_pwmWidth(.5),
	m_keyScale(1.0f),
    m_lockShaperAmt(0)
{
	m_lfoShaper[0] = m_lfoShaper[1] = m_lfoShaper[2] = 0;
	m_lfoPitch[0] = m_lfoPitch[1] = m_lfoPitch[2] = 0;
	m_lfoPWM[0] = m_lfoPWM[1] = m_lfoPWM[2] = 0;
    m_lfoBufs[0] = m_lfoBufs[1] = m_lfoBufs[2] = 0;
}

void PhaseDist::Render(float *fmIn, float *out, int len, float volume)
{
	float *brightContour = m_shpBuf;
	float *ampContour = m_ampBuf;
	
	if (!ampContour || !out)
		return;

	float lastContour;
	if (brightContour)
	{
        // TODO: Make compatible with non-pro (see top of PhaseDist::Render)
#ifdef DIGITS_PRO
        float lfoInfluence =  ((m_lfoBufs[0][0]+1.0f)*.5f) * m_lfoShaper[0]
                            + ((m_lfoBufs[1][0]+1.0f)*.5f) * m_lfoShaper[1]
                            + ((m_lfoBufs[2][0]+1.0f)*.5f) * m_lfoShaper[2];
#else
        float lfoInfluence =  ((m_lfoBufs[0][0]+1.0f)*.5f) * m_lfoShaper[0];
#endif
        float bright;
        if (m_lockShaperAmt)
            bright = m_lockShaperAmt;
        else
            bright = brightContour[0] - (brightContour[0] * lfoInfluence);
		lastContour = m_skew - (bright * m_skew * m_keyScale);
	}
	else
		lastContour = m_skew - (.90f * m_skew);

	float m1 = m_skew / lastContour;
	float m2 = (1.0f - m_skew) / (1.0f - lastContour);
	float b2 = 1.0f - m2;
	bool squareMode = m_shaperMod == .5f;
	if (squareMode)
		b2 *= .5f;
	
	// Calculate PWM shaper
    float lfoInfluence = m_lfoPWM[0] * m_lfoBufs[0][0]
                        + m_lfoPWM[1] * m_lfoBufs[1][0]
                        + m_lfoPWM[2] * m_lfoBufs[2][0];
	float pwmWidth = m_pwmWidth + lfoInfluence;
	if (pwmWidth > Voice::kPWMMinimum + Voice::kPWMRange) pwmWidth = Voice::kPWMMinimum + Voice::kPWMRange;
	if (pwmWidth < Voice::kPWMMinimum) pwmWidth = Voice::kPWMMinimum;
	float m3 = .5f / pwmWidth;
	float m4 = (.5f) / (1.0f - pwmWidth);
	float b4 = 1.0f - m4;	
	float pwmPos;
	if (m_pos < pwmWidth)
		pwmPos = m3*m_pos;
	else
		pwmPos = m4*m_pos + b4;
	
	float fmInputAmtDelta = (m_fmInputAmt1 - m_fmInputAmt) / (float)len;
	
	// Reset detail counter so this isn't happening twice in a row. This is a poor man's
	// optimization: This stuff should ONLY happen when m_detailCounter hits 0! Needs some
	// more thought.
	m_detailCounter = kDetailLevel;

	for (int i=0; i<len; i++)
	{
		float index;
	
		if (squareMode) 
		{
			// Run shaper twice per cycle for square mode
			if (pwmPos < .5f)
			{
				if (pwmPos < lastContour*.5f)
					index = m1*pwmPos;
				else
					index = m2*pwmPos + b2;					
			}
			else
			{
				float pos = pwmPos - .5f;
				if (pos < lastContour*.5f)
					index = .5f + (m1*pos);
				else
					index = m2*pos + b2 + .5f;
			}
		}
		else
		{
			if (pwmPos < lastContour)
				index = m1*pwmPos;
			else
				index = m2*pwmPos + b2;
		}
		
		if (m_shaperMod > 1.0f)
			index *= m_shaperMod;

		if (m_fmInputAmt)
			if (m_basisWave == 0)
				out[i] += Tables::Cos(index + (m_fmInputAmt * fmIn[i])) * volume * ampContour[i];
			else
				out[i] += Tables::Sin(index + (m_fmInputAmt * fmIn[i])) * volume * ampContour[i];
		else 
			if (m_basisWave == 0)
				out[i] += Tables::Cos(index) * volume; // Apply amp contour as last step since it's unified
			else
				out[i] += Tables::Sin(index) * volume; 
		
		if (m_fmInputAmt != m_fmInputAmt1)
			m_fmInputAmt += fmInputAmtDelta;
		
		// Wrap oscillator
        float lfoInfluence = 0;
        if (m_lfoPitch[0])
            lfoInfluence += m_lfoBufs[0][i] * m_lfoPitch[0];
        if (m_lfoPitch[1])
            lfoInfluence += m_lfoBufs[1][i] * m_lfoPitch[1];
        if (m_lfoPitch[2])
            lfoInfluence += m_lfoBufs[2][i] * m_lfoPitch[2];
		m_pos += m_delta + (m_delta * lfoInfluence);
		while (m_pos >= 1.0f)
		{
			m_pos -= 1.0f;
		}
		
		// Recalculate linear equation
		if (--m_detailCounter <= 0)
		{
			if (brightContour)
			{
#ifdef DIGITS_PRO
                float lfoInfluence =  ((m_lfoBufs[0][i]+1.0f)*.5f) * m_lfoShaper[0]
                                    + ((m_lfoBufs[1][i]+1.0f)*.5f) * m_lfoShaper[1]
                                    + ((m_lfoBufs[2][i]+1.0f)*.5f) * m_lfoShaper[2];
                if (lfoInfluence > 1.0f) lfoInfluence = 1.0f;
#else
                float lfoInfluence =  ((m_lfoBufs[0][i]+1.0f)*.5f) * m_lfoShaper[0];
#endif
                float bright;
                if (m_lockShaperAmt)
                    bright = m_lockShaperAmt;
                else
                    bright = brightContour[i] * (1.0f - lfoInfluence);

				//float bright = brightContour[i] - (brightContour[i] * ((m_lfoBuf[i]+1.0f)*.5f) * m_lfoShaper);
				lastContour = m_skew - (bright * m_skew * m_keyScale);
			}
			else
				lastContour = m_skew - (.9f * m_skew);
			m1 = m_skew / lastContour;
			m2 = (1.0f - m_skew) / (1.0f - lastContour);
			b2 = 1.0f - m2;
			if (squareMode)
				b2 *= .5f;			

			// Move PWM
#ifdef DIGITS_PRO
            float lfoInfluence = m_lfoPWM[0] * m_lfoBufs[0][0]
                                + m_lfoPWM[1] * m_lfoBufs[1][0]
                                + m_lfoPWM[2] * m_lfoBufs[2][0];
#else
            float lfoInfluence = m_lfoPWM[0] * m_lfoBufs[0][0];
#endif
			float pwmWidth = m_pwmWidth + lfoInfluence;
			if (pwmWidth > Voice::kPWMMinimum + Voice::kPWMRange) pwmWidth = Voice::kPWMMinimum + Voice::kPWMRange;
			if (pwmWidth < Voice::kPWMMinimum) pwmWidth = Voice::kPWMMinimum;
			
			m_detailCounter = kDetailLevel;			
		}

		// Shape position using pwm shaper
		if (m_pos < pwmWidth)
			pwmPos = m3*m_pos;
		else
			pwmPos = m4*m_pos + b4;
		
		// Animate glide
//		m_delta = (m_glide*m_delta) + (m_glide1*m_deltaDest);
	}
	
	m_fmInputAmt = m_fmInputAmt1;
}


