//
//  NoiseGen.h
//  DrumVST
//
//  Created by Louis Gorenfeld on 10/16/16.
//
//

#ifndef NoiseGen_h
#define NoiseGen_h

class NoiseGen
{
public:
    NoiseGen();
    ~NoiseGen();
    void Render(float *out, int len);
    void SetSamplingRate(float sR);
    void Trigger();
    
private:
    float m_pos;
    float m_delta;
};


#endif /* NoiseGen_h */
