//
//  NoiseGen.cpp
//  DrumVST
//
//  Created by Louis Gorenfeld on 10/16/16.
//
//

#include "NoiseGen.h"
#include "Tables.h"

NoiseGen::NoiseGen() :
    m_pos(0),
    m_delta(0)
{
    
}

NoiseGen::~NoiseGen()
{
    
}

void NoiseGen::SetSamplingRate(float sR)
{
    m_delta = Tables::kNoiseRate / sR;
}

void NoiseGen::Render(float *out, int len)
{
    for (int i=0; i < len; i++)
    {
        out[i] = Tables::Noise(m_pos);
        
        m_pos += m_delta;
        while (m_pos >= Tables::kNoisePoints)
            m_pos -= Tables::kNoisePoints;
    }
}

void NoiseGen::Trigger()
{
    m_pos = rand() & Tables::kNoisePoints;
}
