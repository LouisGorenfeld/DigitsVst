#ifndef _LFO_H_
#define _LFO_H_

#include <stdio.h>
#include <stdlib.h>
#include "Tables.h"

class LFO
{
public:
	enum lfoType_t
	{
		kLFO_Sin,
		kLFO_Noise,
        kLFO_RampUp,
        kLFO_RampDn
	};
	
	LFO() : m_type(kLFO_Sin), m_pos(0), m_thisNoise(0), m_lastNoise(0), m_delay(0), m_repeatCountDn(-1), m_repeatSetting(-1) {}
	~LFO() {}
	void Render(float *out, int len) 
	{
		for (int i=0; i<len; i++)
		{
            if (m_repeatCountDn == 0)
            {
				out[i] = 0;
            }
			else if (m_delay > 0)
			{
				out[i] = 0;
				m_delay--;
			}
			else if (m_type == kLFO_Sin)
			{
				out[i] = Tables::Sin(m_pos);
				m_pos += m_delta;
			}
            else if (m_type == kLFO_Noise)
			{
				// If the rate is above 40hz, start smoothing it (TODO, get rate :))
				// TODO: Use better equation (too lazy to think right now)
				out[i] = (m_lastNoise *	(1.0f-m_pos)) + (m_thisNoise * m_pos);
				m_pos += m_delta;
			}
            else if (m_type == kLFO_RampDn)
            {
                out[i] = (m_pos - 1.0f);
                m_pos += m_delta;
            }
            else if (m_type == kLFO_RampUp)
            {
                out[i] = (1.0f - m_pos);
                m_pos += m_delta;
            }

			while (m_pos >= 1.0f)
			{
				if (m_type == 1)
				{
					m_lastNoise = m_thisNoise;
					// Normal distribution
					m_thisNoise = (((rand())&65535)/32767.0f)-1.0f;
					m_thisNoise += (((rand())&65535)/32767.0f)-1.0f;
					m_thisNoise /= 2.0f;
				}
                // Don't go past 0 (see comparison at beginning of fn)
                if (m_repeatCountDn > 0)
                    m_repeatCountDn--;
				m_pos -= 1.0f;
			}
		}
	}
	void Reset() { m_pos = 0; m_delay = m_delayInit; m_repeatCountDn = m_repeatSetting; }
	void SetRate(float rate, float sR) { m_delta = rate/sR; }
	void SetDelay(float delayMs, float sR) { m_delayInit = m_delay = (delayMs*sR)/1000.0f; }
	void SetType(lfoType_t type) { m_type = type; }
    void SetRepeat(int repeat) { m_repeatSetting = repeat; }
	lfoType_t GetType() { return m_type; }
	
private:
	lfoType_t m_type;
	float m_delta;
	float m_pos;
	float m_thisNoise;
	float m_lastNoise;
	int m_delay;
	int m_delayInit;
    // -1 for infinite
    int m_repeatCountDn;
    int m_repeatSetting;
};

#endif
