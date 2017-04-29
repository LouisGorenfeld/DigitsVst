 /* Phase Distortion Synthesizer (c) 2011 Louis Gorenfeld <louis.gorenfeld@gmail.com> */

#include <cassert>
#include "SndBuf.h"
#include "Contour.h"

Contour::Contour() :
	m_curValue(0),
	m_attack(1.0),
    m_decay(1.0),
	m_release(1.0),
	m_fade(0),
	m_sustain(false),
    m_sustainLevel(1.0),
	m_min(0),
	m_max(1.0),
	m_phase(kFinished),
    m_ctlIn(1.0f),
    m_ctlVal(1.0f),
    m_lastOut(0),
	m_noAttack(false)
{
    m_decay1TimesSusLevel = m_decay1 * m_sustainLevel;
}

inline float Contour::Clip(float x)
{
	if (x < 0)
		return 0;
	else if (x > 1.0f)
		return 1.0f;
	
	return x;
}


void Contour::Render(float *out, int len)
{ 
	float range = m_max - m_min;
	float min = m_min;
	
	if (!out)
		return;
	
	assert(m_attack != 0);
	assert(m_release != 0);

	for (int i=0; i<len; i++)
	{
		if (m_phase == kFinished)
		{
		}
		else if (m_phase == kAttack)
		{
			m_curValue = (m_attack*m_lastOut) + (m_attack1);
			m_lastOut = m_curValue;
			
			if (m_curValue >= .98)
			{
				m_curValue = .98;
                m_phase = kDecay;
			}
		}
        else if (m_phase == kDecay)
        {
            m_curValue = (m_decay*m_lastOut) + m_decay1TimesSusLevel;
            m_lastOut = m_curValue;
            
            if (m_curValue <= m_sustainLevel)
            {
                if (m_sustain)
                {
                    m_phase = kSustain;
                }
                else
                {
                    m_phase = kRelease;
                }
            }
        }
		else if (m_phase == kSustain)
		{
			m_curValue = (m_fade*m_lastOut);
			m_lastOut = m_curValue;
			if (m_curValue <= 0.001)
			{
				m_phase = kFinished;
				m_curValue = 0;
                m_lastOut = 0;
			}
		}
		else if (m_phase == kRelease)
		{
			m_curValue = (m_release*m_lastOut);
			m_lastOut = m_curValue;
			
			if (m_curValue <= 0.001)
			{
				m_phase = kFinished;
				m_curValue = 0;
                m_lastOut = 0;
			}
		}
        
        // TODO: Vary this with the sampling rate. It's really just
        // a smoothing function, so it's probably OK for now
        m_ctlVal = m_ctlVal*.99 + m_ctlIn*.01;
		
		*out++ = Clip(min + m_curValue * range * m_ctlVal);
	}
}

