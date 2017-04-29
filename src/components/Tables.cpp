#include "Tables.h"

float Tables::g_sinTab[kSinePoints+1];
float Tables::g_cosTab[kSinePoints+1];
float Tables::g_softClipTab[kSoftClipPoints+1];
float Tables::g_dbTab[kDbPoints+1];
float Tables::g_pulseTab[kPulsePoints];
float Tables::g_noiseTab[kNoisePoints+1];

void Tables::Init()
{
	for (int i=0; i<kSinePoints; i++)
	{
		g_sinTab[i] = sin(2.0*M_PI*((float)i/(float)kSinePoints));
		g_cosTab[i] = cos(2.0*M_PI*((float)i/(float)kSinePoints));
	}
    for (int i=0; i<kPulsePoints; i++)
    {
        // TODO
    }
	g_sinTab[kSinePoints] = g_sinTab[0];
	g_cosTab[kSinePoints] = g_cosTab[0];

    // Go from -96db to 0
    for (int i=0; i<kDbPoints; i++)
    {
        g_dbTab[kDbPoints] = powf(10, -(float)i/20.0f);
    }
    g_dbTab[kDbPoints] = g_dbTab[kDbPoints-1];

	for (int i=0; i<kSoftClipPoints; i++)
	{
		float sam = (i<<1) - kSoftClipPoints;
		sam /= (float)kSoftClipPoints;
		g_softClipTab[i] = sam - ((sam*sam*sam)/3.0f);
	}
    g_softClipTab[kSoftClipPoints] = g_softClipTab[kSoftClipPoints-1];
    
    for (int i=0; i < kNoisePoints; i++)
    {
        g_noiseTab[i] = (float)((rand() & 255) - 127) / 127.0f;
    }
    g_noiseTab[kNoisePoints] = g_noiseTab[0];
}

