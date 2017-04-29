//
//  Compressor.cpp
//  DrumVST
//
//  Created by Louis Gorenfeld on 10/2/16.
//
//

#include "Compressor.h"

Compressor::Compressor() :
    m_envFollowPos(0),
    m_attack(0),
    m_release(0),
    m_threshold(1.0),
    m_ratio(1.0/3.0)
{
}

void Compressor::SetSamplingRate(float sR)
{
    m_sR = sR;
}

void Compressor::Render(float *buf, int len)
{
    float level;
    float peakLevel = 0;
    
    for (int i=0; i < len; i++)
    {
        level = buf[i];
        if (level > peakLevel)
            peakLevel = level;
        else
            peakLevel *= .9999;
        
        if (peakLevel > m_threshold)
        {
            // Instant attack; release determined in part by peaklevel fall
            buf[i] *= 1.0 + ((peakLevel - m_threshold) * m_ratio);
        }
    }
}
