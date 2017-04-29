#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "Oversampler.h"


Oversampler::Oversampler(int kernelSize, int times) :
	m_kernelSize(kernelSize),
	m_bufPos(0),
    m_times(times)
	 
{
    float cutoff = .5 / (float)times;
    
	m_kernel = new float[kernelSize];
	m_ins = new float[kernelSize];
	for (int i=0; i < kernelSize; i++)
	{
		float idx = i - (kernelSize/2.0);
		float winPos = (float)i / (float)kernelSize;
	
		// Blackman window
		float window = .42 - .5*cos(2.0*M_PI*winPos) + .08*cos(4.0*M_PI*winPos);
		
		if (idx == 0)
			m_kernel[i] = 2.0*M_PI*cutoff;
		else
  			m_kernel[i] = sin(2.0*M_PI*cutoff*idx) / idx;

		m_kernel[i] *= window;
		m_ins[i] = 0;
	}

    // Normalize the kernels
	float sum = 0;
	for (int i=0; i < kernelSize; i++)
		sum += m_kernel[i];

	for (int i=0; i < kernelSize; i++)
		m_kernel[i] /= sum;
}

	
void Oversampler::render(float *in, float *out, int numInSamples)
{
	memset(out, 0, sizeof(float)*numInSamples);

    for (int i=0; i < numInSamples; i+=m_times)
	{
		m_ins[m_bufPos] = *in++;

		int pos = m_bufPos;
		for (int j=0; j < m_kernelSize; j++)
		{
            *out += m_ins[pos] * m_kernel[j];
			if (--pos < 0)
				pos += m_kernelSize;
		}
        
        out++;
		m_bufPos = (m_bufPos + 1) % m_kernelSize;
    }
}

Oversampler::~Oversampler()
{
	delete[] m_kernel;
	delete[] m_ins;
}



// For testing

// OPTIMIZE: If we're going down to 1/8th of sR, every 4th sample in the filter is 0 (1/4 of Nyquist)
// So, we can skip those. We could unroll it. We can also skip every 4th output sample.
/*
int main()
{
	//WinSincFilter filter(128, .015);
	Oversampler filter(64, .125);
	float noise[44100];
	float out[44100];

	float pos = 0;

	for (int i=0; i < 44100; i++)
	{
		noise[i] = sin(M_PI*2.0*pos);
        pos += 8000/44100.0;
	}

//	for (int i=0; i < 44100; i++)
//		noise[i] = ((rand() & 255) - 127) / 127.0f;

	filter.render(noise, out, 44100, 1);

	FILE *fp = fopen("test.raw", "wb");
	fwrite(out, sizeof(float), 44100, fp);

	fclose(fp);
}
*/
