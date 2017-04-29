#ifndef _TABLES_H_
#define _TABLES_H_

#include <cmath>	
#include <stdio.h>
#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#endif

// Tables and misc. globals

namespace Tables
{
	void Init();
	
	// This goes from 0 to 1, unlike real sine
	// which goes from 0 to 2pi
	inline float Sin(float x);
	inline float Cos(float x);
    inline float Noise(float x);
    inline float dbToAmp(float x);
	
	inline float SoftClip(float x);
	inline void ZeroMem(float *buf, int len);
	
	const int kSinePoints = 8192;
    const int kPulsePoints = 4096;
    const float kNoiseRate = 30000;
    const int kNoisePoints = kNoiseRate * 4;
	extern float g_sinTab[kSinePoints+1];
	extern float g_cosTab[kSinePoints+1];
    extern float g_pulseTab[kPulsePoints];

    // 1 entry for each 16-bit dB (interpolated when reading)
    const int kDbPoints = 96;
    extern float g_dbTab[kDbPoints+1];
    
	const int kSoftClipPoints = 256;
	extern float g_softClipTab[kSoftClipPoints+1];
    extern float g_noiseTab[kNoisePoints+1];

#ifdef DIGITS_PRO
    const int kNumLFOs = 3;
#else
    const int kNumLFOs = 1;
#endif
};

inline float Tables::Noise(float x)
{
    float dist = x - floor(x);
    int i = x;
    return g_noiseTab[i] + dist*(g_noiseTab[i+1] - g_noiseTab[i]);
}

inline float Tables::Sin(float x)
{
 	float fIndex = x*kSinePoints;
	
	// This profiled to be faster than fmod
	while (fIndex >= kSinePoints) 
		fIndex -= kSinePoints;
	while (fIndex < 0) 
		fIndex += kSinePoints;
	
	// Short circuit this function-- this is nearest neighbor. With
	// a 8k-entry table, there doesn't seem to be noticible noise and
	// it uses less CPU than, say, a 256-entry table with interpolation
	// below (on an Intel i5)
	return g_sinTab[(int)fIndex];
	
#if 0
	float frac = fIndex - int(fIndex);
	int index = fIndex; 
	float ret = g_sinTab[index] + ( g_sinTab[index+1] - g_sinTab[index] )*frac;
	
	return ret;
#endif
}

inline float Tables::Cos(float x)
{
	// Cribbed from Sin()-- go see comments there	
	float fIndex = x*kSinePoints;
	
	// This profiled to be faster than fmod
	while (fIndex >= kSinePoints) 
		fIndex -= kSinePoints;
	while (fIndex < 0) 
		fIndex += kSinePoints;
   // if (x == NAN) { printf("NAN2 from x = %f\n", x); return 0; }

	
	return g_cosTab[(int)fIndex];
}

inline float Tables::SoftClip(float x)
{
    
    float fIndex = ((x + 1.0f)/2.0f) * (float)kSoftClipPoints;
    int index = (int)fIndex;
    float frac = fIndex - index;
    return g_softClipTab[index] + (g_softClipTab[index+1] - g_softClipTab[index]) * frac;
}

inline void Tables::ZeroMem(float *buf, int len)
{
#ifdef __APPLE__
	vDSP_vclr(buf, 1.0, len);
#else
	for (int i=0; i<len; i++)
		buf[i] = 0;
#endif
}

#endif
