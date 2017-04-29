//
//  Compressor.h
//  DrumVST
//
//  Created by Louis Gorenfeld on 10/2/16.
//
//

#ifndef Compressor_h
#define Compressor_h

class Compressor
{
public:
    Compressor();
    ~Compressor();
    void SetSamplingRate(float sR);
    void Render(float *buf, int len);
    void SetThreshold(float threshold) { m_threshold = threshold; }
    void SetRatio(float ratio) { m_ratio = ratio; }
    
private:
    float m_envFollowPos;
    float m_attack;
    float m_release;
    float m_threshold;  // dB
    float m_ratio;
};

#endif /* Compressor_h */
