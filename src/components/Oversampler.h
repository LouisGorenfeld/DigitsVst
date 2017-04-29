#ifndef _OVERSAMPLER_H_
#define _OVERSAMPLER_H_

// Straight-forward non-convolving sinc filter
class Oversampler
{
public:
    Oversampler(int kernelSize, int times);
    ~Oversampler();
    
    void render(float *in, float *out, int numInSamples);
    
private:
    int m_kernelSize;
    int m_bufPos;
    int m_times;
    float *m_kernel;
    float *m_ins;
};

#endif