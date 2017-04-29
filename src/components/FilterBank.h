//
//  FilterBank.h
//  PDVST
//
//  Created by Louis Gorenfeld on 11/16/12.
//
//

#ifndef __PDVST__FilterBank__
#define __PDVST__FilterBank__

//
//  FilterBank.cpp
//  PDVST
//
//  Created by Louis Gorenfeld on 11/16/12.
//
//

#include <math.h>
#include "FilterBank.h"

class FilterBase
{
public:
    FilterBase() {}
    ~FilterBase() {}
    inline void SetSharpness() {}   // aka bandwidth, q, resonance
    inline void SetFrequency() {};
    virtual void SetSamplingRate(float f) { m_sR = f; }
    
    // If a filter has a control buffer (e.g., brightness), set it with this
    virtual void SetControlBuffer(float *buf) { }

protected:
    float m_sR;
};

// No-resonance 1-pole lowpass filter
class LowPass1p : public FilterBase
{
public:
    LowPass1p() : m_a(0), m_b(0), m_lastOut(0) {}
    ~LowPass1p() {}
    //    virtual void SetControlBuffer(float *buf) { m_freqContour = buf; }
    inline void SetFrequency(float f)
    {
        // TODO: Table this?
        m_frequency = f;
        m_b = exp(-2.0f * M_PI * (f/m_sR));
        m_a = 1.0f - m_b;
        
    }
    inline float Tick(float in)
    {
        return m_lastOut = m_a*in + m_b*m_lastOut;
    }
    virtual void SetSamplingRate(float f)
    {
        m_sR = f;
        SetFrequency(m_frequency);
    }
    void Render(float* buf, int len)
    {
        for (int i=0; i<len; i++)
        {
            buf[i] = Tick(buf[i]);
        }
    }
    virtual void Reset()
    {
        m_lastOut = 0;
    }
    
protected:
    float m_a;
    float m_b;
    float m_lastOut;
    //    int m_detailCounter;
    float *m_freqContour;
    float m_frequency;
    
};


// Two-pole resonator. This is not good for sweeping.
class TwoPole : public FilterBase
{
public:
    TwoPole() : m_frequency(0), m_q(0)
    {
        for (int i=0; i < 2; i++)
        {
            m_b[i] = 0;
            m_lastOut[i] = 0;
        }
    }
    ~TwoPole() {}
    
    inline void SetFrequencyAndQ(float frequency, float qq)
    {
        float f = 2.0 * M_PI * (frequency/m_sR);
        float dist = 1.0 - qq;
        float q = dist * (44100.0/m_sR);
        q = 1.0 - q;
        m_a = (1.0 - q) * sqrt(1.0 - 2.0*q*cos(2*f) + q*q);
        m_b[0] = -2.0*q*cos(f);
        m_b[1] = q*q;
        m_frequency = frequency;
        m_q = qq;
    }

    inline float Tick(float in)
    {
        double out = m_a*in - m_b[0]*m_lastOut[0] - m_b[1]*m_lastOut[1];
        m_lastOut[1] = m_lastOut[0];
        m_lastOut[0] = out;
        return out;
    }
    
    virtual void SetSamplingRate(float f)
    {
        m_sR = f;
        SetFrequencyAndQ(m_frequency, m_q);
    }
    
    virtual void Reset()
    {
        m_b[0] = m_b[1] = m_lastOut[0] = m_lastOut[1] = 0;
    }

private:
    double m_b[2];
    double m_lastOut[2];
    double m_a; // gain
    
    double m_frequency;
    double m_q;
};


// General-purpose biquad
class BiQuad : public FilterBase
{
public:
    BiQuad() : m_pfrequency(0), m_pq(0),
               m_zfrequency(0), m_zq(0)
    {
        for (int i=0; i < 2; i++)
        {
            m_b[i] = 0;
            m_lastOut[i] = 0;
            m_lastIn[i] = 0;
        }
        for (int i=0; i < 3; i++)
            m_a[i] = 0;
    }
    ~BiQuad() {}
    
    inline void SetFrequencyAndQ(float pfrequency, float pq,
                                 float zfrequency, float zq)
    {
        float pf = 2.0 * M_PI * (pfrequency/m_sR);
        float zf = 2.0 * M_PI * (zfrequency/m_sR);
        m_pq = pq;
        m_zq = zq;
        
/*        float dist = 1.0 - pq;
        pq = dist * (44100.0/m_sR);
        pq = 1.0 - pq;
        dist = 1.0 - zq;
        zq = dist * (44100.0/m_sR);
        zq = 1.0 - zq;
  */
        m_pfrequency = pfrequency;
        m_zfrequency = zfrequency;

        m_a[0] = ((1.0 - pq) * sqrt(1.0 - 2.0*pq*cos(2*pf) + pq*pq))
                / ((1.0 - zq) * sqrt(1.0 - 2.0*zq*cos(2*zf) + zq*zq));
        m_a[1] = -2.0*zq*cos(zf);
        m_a[2] = zq*zq;
        m_b[0] = -2.0*pq*cos(pf);
        m_b[1] = pq*pq;
    }
    
    /*
     inline void SetFrequencyAndQ(float pfrequency, float pq,
     float zfrequency, float zq)
     {
     float pf = 2.0 * M_PI * (pfrequency/m_sR);
     float zf = 2.0 * M_PI * (zfrequency/m_sR);
     float dist = 1.0 - pq;
     m_pq = pq;
     m_zq = zq;
     m_pfrequency = pfrequency;
     m_zfrequency = zfrequency;
     
     pq = dist * (44100.0/m_sR);
     pq = 1.0 - pq;
     zq = dist * (44100.0/m_sR);
     zq = 1.0 - zq;
     
     m_a[0] = ((1.0 - pq) * sqrt(1.0 - 2.0*pq*cos(2*pf) + pq*pq))
     / ((1.0 - zq) * sqrt(1.0 - 2.0*zq*cos(2*zf) + zq*zq));
     m_a[1] = -2.0*zq*cos(zf);
     m_a[2] = zq*zq;
     m_b[0] = -2.0*pq*cos(pf);
     m_b[1] = pq*pq;
     }
*/
    inline float Tick(float in)
    {
        double out = m_a[0]*in + m_a[1]*m_lastIn[0] + m_a[2]*m_lastIn[1]
                - m_b[0]*m_lastOut[0] - m_b[1]*m_lastOut[1];
        m_lastOut[1] = m_lastOut[0];
        m_lastOut[0] = out;
        m_lastIn[0] = m_lastIn[1];
        m_lastIn[1] = in;
        return out;
    }
    
    virtual void SetSamplingRate(float f)
    {
        m_sR = f;
        SetFrequencyAndQ(m_pfrequency, m_pq,
                         m_zfrequency, m_zq);
    }
    
    virtual void Reset()
    {
        m_lastOut[0] =  m_lastOut[1] = 0;
        m_lastIn[0] =  m_lastIn[1] = 0;
    }

    
private:
    double m_b[2];
    double m_lastOut[2];
    double m_lastIn[2];
    double m_a[3]; // index 0 is gain
    
    double m_pfrequency;
    double m_pq;
    double m_zfrequency;
    double m_zq;
};



class Bandpass : public FilterBase
{
public:
    Bandpass() : m_frequency(0), m_q(0)
    {
        for (int i=0; i < 2; i++)
        {
            m_b[i] = 0;
            m_a[i] = 0;
            m_lastOut[i] = 0;
            m_lastIn[i] = 0;
        }
        m_a[2] = 0;
    }
    ~Bandpass() {}
    
    inline void SetFrequencyAndQ(float frequency, float q)
    {
        float f = 2.0 * M_PI * (frequency/m_sR);
        m_a[0] = (1.0 - q) * sqrt(1.0 - 2.0*q*cos(2*f) + q*q) / sqrt(2-2.0*cos(f));
        m_a[1] = 0;
        m_a[2] = -1.0;
        m_b[0] = -2.0*q*cos(f);
        m_b[1] = q*q;
        m_frequency = frequency;
        m_q = q;
    }
    
    inline float Tick(float in)
    {
        double out = m_a[0]*in + m_a[1]*m_lastIn[0] + m_a[2]*m_lastIn[1]
                    - m_b[0]*m_lastOut[0] - m_b[1]*m_lastOut[1];
        m_lastOut[1] = m_lastOut[0];
        m_lastOut[0] = out;
        m_lastIn[1] = m_lastIn[0];
        m_lastIn[0] = in;
        return out;
    }
    
    virtual void SetSamplingRate(float f)
    {
        m_sR = f;
        SetFrequencyAndQ(m_frequency, m_q);
    }
    
private:
    double m_b[2];
    double m_lastOut[2];
    double m_lastIn[2];
    double m_a[3]; // gain is index 0
    
    double m_frequency;
    double m_q;
};



// No-resonance highpass filter
class HighPass : public LowPass1p
{
public:
    HighPass() {}
    ~HighPass() {}
    //    virtual void SetControlBuffer(float *buf) { m_freqContour = buf; }
/*    inline void SetFrequency(float f)
    {
        // TODO: Table this?
        m_frequency = f;
        m_a = exp(-2.0f * M_PI * (f/m_sR));
        m_a2 = 1.0f + m_a;
//        m_a = .5;
//        m_a2 = 1.0 - m_a;
    }*/
    inline float Tick(float in)
    {
//        in = (rand() & 65535) / 32767.0;
//        in -= 1.0;
        return in - LowPass1p::Tick(in);
    }
    void Render(float* buf, int len)
    {
        for (int i=0; i<len; i++)
        {
            buf[i] = Tick(buf[i]);
        }
    }
    
protected:
};

// Variation of 1p. Build off of it.
class LowPassReso : public LowPass1p
{
public:
    LowPassReso() : m_poles(3), m_reso(0)
    {
        m_lastOuts[0] = m_lastOuts[1] = m_lastOuts[2] = m_lastOuts[3] = 0;
        m_lbFilter.SetFrequency(6000.0f);
        m_oversampling = 4;
    }
    ~LowPassReso() {}
    inline float Clip(float x)
    {
        if (x > 1.0f) return 1.0f;
        else if (x < -1.0f) return -1.0f;
        return x;
    }
    virtual void SetSamplingRate(float f)
    {
        m_sR = f * m_oversampling;
        m_lbFilter.SetSamplingRate(f);
        SetFrequency(m_frequency);
    }

    inline void SetSharpness(float x) { m_reso = x; }
    inline float Tick(float in)
    {
        for (int i=0; i < m_oversampling; i++)
        {
            m_lastOuts[0] = Clip(m_a*in + m_b*m_lastOuts[0] - ((m_reso*m_a)*(m_lbFilter.Tick(m_lastOuts[3]))));
            m_lastOuts[1] = Clip(m_a*m_lastOuts[0] + m_b*m_lastOuts[1]);
            m_lastOuts[2] = Clip(m_a*m_lastOuts[1] + m_b*m_lastOuts[2]);
            m_lastOuts[3] = Clip(m_a*m_lastOuts[2] + m_b*m_lastOuts[3]);
        }

        return m_lastOuts[m_poles];
    }
    void Render(float* buf, int len)
    {
        for (int i=0; i<len; i++)
        {
            buf[i] = Tick(buf[i]);
        }
    }

    // False if out of range
    bool SetPoles(int poles)
    {
        if (poles < 1 || poles > 4)
            return false;
        
        m_poles = poles - 1;
        return true;
    }
    void SetOversampling(int times) { m_oversampling = times; }

private:
    //    virtual void SetFrequency(float f)
    
    float m_lastOuts[4];
    int m_poles; // 0 (6db) to 3 (24db) 
    
    float m_reso;
    float m_oversampling;
    LowPass1p m_lbFilter;
};

#endif /* defined(__PDVST__FilterBank__) */

