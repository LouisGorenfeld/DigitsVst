/* Phase Distortion Synthesizer (c) 2011 Louis Gorenfeld <louis.gorenfeld@gmail.com> */

#include "Voice.h"
#include "VstCore.h"

#ifdef DIGITS_MULTICORE
#include <errno.h> // EBUSY for trylock
#include <pthread.h>
#endif

float Voice::kAttackRange = 1000.0f;
float Voice::kAttackMinimumAmp = 1.0f;
float Voice::kAttackMinimumShaper = 1.0f;
float Voice::kDecayRange = 1000.0f;
float Voice::kDecayMinimum = 20.0f;
float Voice::kReleaseRange = 1000.0f;
float Voice::kReleaseMinimum = 20.0f;
float Voice::kFadeRange = 3000.0f;
float Voice::kFadeMinimum = 500.0f;
float Voice::kDetuneMaximum = .01f;
float Voice::kLFOMinimum = .25f; //hz
float Voice::kLFORange = 1000.0f;	//well into audio range
float Voice::kPWMMinimum = .1f;
float Voice::kPWMRange = .8f;

#ifdef DBGMCORE
#define DEBUG_MCORE(...) { fprintf(stderr, __VA_ARGS__); }
#else
#define DEBUG_MCORE
#endif

// This is to shape parameter space so, for example, the slider for
// an envelope has more "short decay" options than long ones. This
// makes controls with a lot of range much easier to use.
inline float ExpShape(float base, float x)
{
	float range = base - 1;
	return ((pow(base, x) - 1.0f)/range);
}

Voice::Voice(int voiceNum) :
	m_pdOsc1(),
	m_pdOsc2(),
    m_pitch(0),
    m_detune(0),
    m_octave(1.0f),
    m_mix(.5f),
    m_resoVol(0),
    m_pulseWidth(.5f),
    m_delta(0),
    m_deltaLo(0),
    m_deltaHi(0),
    m_delta2(0),
    m_deltaLo2(0),
    m_deltaHi2(0),
    m_deltaBlend(0),
    m_glideSpeed(0),
    m_glideMode(kGlide_PolyNoGlide),
    m_bend(0),
    m_modWheel(0),
    m_aftertouch(0),
    m_velocity(0),
    m_basisWave(0),
    m_shpMin(0),
    m_shpMax(0),
    m_velShp(0),
    m_velAmp(0),
    m_modShp(0),
    m_modLFO1Amp(0),
    m_atouchShp(0),
    m_atouchLFO1Amp(0),
	m_nextSubVoice(0),
	m_needsRender(false),
    m_isSubVoice(false),
    m_lockShaperAmt(0),
    m_sR(0),
    m_voiceNumber(voiceNum),
    m_ampRateScale(0),
    m_ampLevelScale(0),
    m_ampAttack(0),
    m_ampDecay(0),
    m_ampRelease(0),
    m_shpRateScale(0),
    m_shpLevelScale(0),
    m_shpAttack(0),
    m_shpDecay(0),
    m_shpRelease(0),
    m_bitCrushPos(0),
    m_bitCrushDelta(0),
    m_bitCrushBlend(0),
    m_bitCrushDivider(0),
    m_bitCrushBits(0)
{
#ifdef DIGITS_MULTICORE
    pthread_mutex_init(&m_mutex, 0);
#endif

    m_delta = m_deltaHi = m_deltaLo = 0;
    
	m_pdOsc1.SetSkew(.5);
	m_pdOsc2.SetSkew(.99);
    
    for (int i=0; i<Tables::kNumLFOs; i++)
    {
        m_lfoIsAssigned[i] = 0;
        m_lfoAmplitude[i] = 0;
    }
    
    m_bitCrushLastSample = 0;
}

Voice::~Voice()
{
}

void Voice::NoteOn(uint64_t timestamp, float deltaLo, float deltaHi, float delta, int midiKey, int velocity, bool reset)
{
	m_timestamp = timestamp;
	m_pitch = midiKey;
	m_velocity = (float)velocity/127.0f;
	
    // Set up envelope rates
    // The rate scaling is set around middle-A (midi note 60)
    float rateScale = 1.0/pow(2.0, (float(midiKey-60)/12.0f)*m_ampRateScale);
    m_ampEnv.SetAttackExponential(m_ampAttack * rateScale, m_sR);
    m_ampEnv.SetDecayExponential(m_ampDecay * rateScale, m_sR);
    m_ampEnv.SetReleaseExponential(m_ampRelease * rateScale, m_sR);
    rateScale = 1.0/pow(2.0, (float(midiKey-60)/12.0f)*m_shpRateScale);
    m_shaperEnv.SetAttackExponential(m_shpAttack * rateScale, m_sR);
    m_shaperEnv.SetDecayExponential(m_shpDecay * rateScale, m_sR);
    m_shaperEnv.SetReleaseExponential(m_shpRelease * rateScale, m_sR);

	m_ampEnv.SetMaximum((1.0 - (1.0-m_velocity)*m_velAmp));
    
    float levelScale = 1.0f/pow(2.0, (float(midiKey-60)/12.0f)*m_shpLevelScale);
	if (levelScale > 1.0f) levelScale = 1.0f;
    
	// TODO: Don't pass delta in; might as well do it all at once

	if (reset)
	{
		ResetPhases();
	}
	m_ampEnv.GateOn(false);
	m_shaperEnv.GateOn(false);
    for (int i=0; i<Tables::kNumLFOs; i++)
        m_lfos[i].Reset();
	
	m_delta = delta;
	m_deltaLo = deltaLo;
	m_deltaHi = deltaHi;
    m_delta2 = delta;
    m_deltaLo2 = deltaLo;
    m_deltaHi2 = deltaHi;
    m_deltaBlend = 0;
	CalcPitch();
    
    if (m_bitCrushDivider)
    {
        m_bitCrushPos = 0;
        m_bitCrushLastSample = 0;
        m_bitCrushDelta = delta * pow(2.0f, m_bitCrushDivider);
    }
	
    // Apply automatic keyscaling for bandlimiting purposes. This is different from the
    // keyscaling set in the patch whose purpose is to simulate the properties of acoustic
    // instruments.
	float keyScale;
    float x = m_pdOsc1.GetDelta()*m_sR;
	if (x >= 400)
	{
		// Intercept (400, 1) and (1200, 0)
		
		keyScale = 1.0 - (x - 400.0f)/6000.0f;
		if (keyScale < 0) keyScale = 0;
	}
	else {
		keyScale = 1.0f;
	}
	
	m_pdOsc1.SetKeyScale(keyScale*levelScale);
	
    x = m_pdOsc2.GetDelta()*m_sR;
	if (x >= 400)
	{
		// Intercept (400, 1) and (1200, 0)
		
		keyScale = 1.0 - (x - 400.0f)/6000.0f;
		if (keyScale < 0) keyScale = 0;
	}
	else {
		keyScale = 1.0f;
	}
	
	m_pdOsc2.SetKeyScale(keyScale*levelScale);
}

void Voice::NoteOff(int key)
{
	if (key != m_pitch && key >= 0)
		return;
	
	m_ampEnv.GateOff();
	m_shaperEnv.GateOff();
}

void Voice::PitchBend(float value)
{
	m_bend = value;
	CalcPitch();
}

void Voice::CalcPitch()
{
	float pitch;
    float pitch2;

	if (m_bend < 0)
	{
		pitch = m_delta + ((m_delta-m_deltaLo)*m_bend);
		pitch2 = m_delta2 + ((m_delta2-m_deltaLo2)*m_bend);
	}
	else
	{
		pitch = m_delta + ((m_deltaHi-m_delta)*m_bend);
		pitch2 = m_delta2 + ((m_deltaHi2-m_delta2)*m_bend);
	}
    
    pitch = (1.0f-m_deltaBlend)*pitch + (m_deltaBlend)*pitch2;
	
	m_pdOsc2.SetPitch((pitch) - (pitch*m_detune));
	m_pdOsc1.SetPitch((pitch * m_octave) + (pitch * m_detune));
	m_resoGen.SetPitch(pitch);
}

void Voice::SetBasisWaves()
{
	m_pdOsc1.SetBasis(m_basisWave);
	m_pdOsc2.SetBasis(m_basisWave);

}

void Voice::SetPulseWidths()
{
	m_pdOsc1.SetPulseWidth(m_pulseWidth);
	m_pdOsc2.SetPulseWidth(m_pulseWidth);
	m_resoGen.SetPulseWidth(m_pulseWidth);	
}

void Voice::Render(float *out, int len, Voice::voiceBuffers_t* bufs, bool doContourGens, int thread)
{
    // If the mutex is busy, it means another thread has already grabbed
    // this voice and its subvoices, so skip it
#ifdef DIGITS_MULTICORE
    if (pthread_mutex_trylock(&m_mutex) == EBUSY)
    {
        DEBUG_MCORE("%s: Lock was busy on voice %d\n", thread==0? "c" : "h", m_voiceNumber);
        Tables::ZeroMem(out, len);
        return;
    }
#endif
    
	if (IsFree() || !m_needsRender)
    {
        Tables::ZeroMem(out, len);
#ifdef DIGITS_MULTICORE
        pthread_mutex_unlock(&m_mutex);
#endif
        m_needsRender = false;
		return;
    }
    
    DEBUG_MCORE("%s: processing voice %d\n", thread==0? "c": "h", m_voiceNumber);
    m_needsRender = false;
    
    // Initialize buffers (previously InitBuffers, pre-multicore)
    m_pdOsc1.SetBuffers(bufs->m_ampEnvBuf, bufs->m_shpEnvBuf, bufs->m_lfoBufs);
    m_pdOsc2.SetBuffers(bufs->m_ampEnvBuf, bufs->m_shpEnvBuf, bufs->m_lfoBufs);
	m_resoGen.SetBuffers(bufs->m_ampEnvBuf, bufs->m_shpEnvBuf, bufs->m_lfoBufs);

    // Lock (or unlock) shaper to the specified setting. 0 is off.
    m_pdOsc1.LockShaperTo(m_lockShaperAmt);
    m_pdOsc2.LockShaperTo(m_lockShaperAmt);

	if (doContourGens)
	{
        float shpMod = (1.0f-m_velocity) * m_velShp;
        
        // TODO: Do we want to just set ctlAmt to this? Maybe. Might be more consistent.
        m_shaperEnv.SetMaximum(m_shpMax - m_shpMax*shpMod);
        m_shaperEnv.SetMinimum(m_shpMin - m_shpMin*shpMod);

        float ctlAmt = m_modWheel*m_modShp + m_aftertouch*m_atouchShp;
        if (ctlAmt > 1.0f)
            ctlAmt = 1.0f;
        m_shaperEnv.SetControlInput(1.0f-ctlAmt);
        
		m_shaperEnv.Render(bufs->m_shpEnvBuf->Data(), len);
		m_ampEnv.Render(bufs->m_ampEnvBuf->Data(), len);
        
		// TODO: LFOs are surprisingly costly, so we should only
		// render this if the LFO is assigned. This should be determined
		// when the LFO levels are adjusted, not determined each frame!
        for (int i=0; i<Tables::kNumLFOs; i++)
        {
            if (m_lfoIsAssigned[i])
                m_lfos[i].Render(bufs->m_lfoBufs[i]->Data(), len);
        }
        
        // Apply LFO1 attenuation if enabled
        // TODO: vDSP this sucker!
        if (m_atouchLFO1Amp || m_modLFO1Amp)
        {
            float influence = (1.0f-m_aftertouch)*m_atouchLFO1Amp + (1.0f-m_modWheel)*m_modLFO1Amp;
            for (int i=0; i<len; i++)
                bufs->m_lfoBufs[0]->Data()[i] *= 1.0f - influence;
        }
	}
	//... but the noise LFO is rendered regardless
	else
    {
        for (int i=0; i<Tables::kNumLFOs; i++)
        {
            if (m_lfos[i].GetType() == LFO::kLFO_Noise)
                m_lfos[i].Render(bufs->m_lfoBufs[i]->Data(), len);
        }
    }
    
    // Animate glide
    if (m_deltaBlend > 0)
    {
        CalcPitch();
        m_deltaBlend += 1.0f / m_glideSpeed;
        if (m_deltaBlend >= 1.0f)
        {
            m_deltaBlend = 0;
            m_delta = m_delta2;
            m_deltaHi = m_deltaHi2;
            m_deltaLo = m_deltaLo2;
            CalcPitch();
        }
    }

    // If doContourGens is false, we assume it's because we're rendering a subvoice
    if (doContourGens)
        Tables::ZeroMem(out, len);
    
	float pdVol = 1.0f - m_resoVol;
	if (pdVol > 0)
	{
		Tables::ZeroMem(bufs->m_fmScratchBuf->Data(), len);

        // Don't add in the volume here because it affects the FM indexing
		m_pdOsc1.Render(0, bufs->m_fmScratchBuf->Data(), len, 1.0f);
		m_pdOsc2.Render(bufs->m_fmScratchBuf->Data(), out, len, m_mix);

		// Scale the volume afterwards so it doesn't interfere with 
		// the FM indexing, and also apply the envelope here since it's
		// a unified envelope for both oscillators
		// TODO: Fold this into the volume specified in the pdOsc render function
		// and save a mul
        // TODO: vDSP this stuff
        float osc1mix = 1.0f - m_mix;
		for (int i=0; i<len; i++)
        {
			out[i] += bufs->m_fmScratchBuf->Data()[i] * osc1mix;
			out[i] *= pdVol * bufs->m_ampEnvBuf->Data()[i];
		}
	}
	if (pdVol < 1.0f)
	{
		m_resoGen.Render(out, len, m_resoVol);
	}

	// Render the subvoices (without contour generator)
	if (m_nextSubVoice)
	{
		if (m_ampEnv.IsDone())
			m_nextSubVoice->DisableSubVoices();
		m_nextSubVoice->Render(out, len, bufs, false, thread);
	}
    
	// TODO: Find a way to unify volumes so this can be done in the loop above
	// TODO: Or at least do this only once for all subvoices
	if (doContourGens)
	{
        // If shaper is locked, that means the filter is engaged (and is responsible
        // for brightness control)
        if (m_lockShaperAmt)
        {
            m_trueFilter.SetFrequency(18000.0f * bufs->m_shpEnvBuf->Data()[0]);
            m_trueFilter.Render(out, len);
        }

		for (int i=0; i<len; i++)
		{
#ifdef DIGITS_PRO
            float lfoInfluence = bufs->m_lfoBufs[0]->Data()[i]*m_lfoAmplitude[0]
                + bufs->m_lfoBufs[1]->Data()[i]*m_lfoAmplitude[1]
                + bufs->m_lfoBufs[2]->Data()[i]*m_lfoAmplitude[2];
#else
            float lfoInfluence = bufs->m_lfoBufs[0]->Data()[i]*m_lfoAmplitude[0];
#endif
			out[i] -= (out[i]*lfoInfluence);
		}
	}

    if (m_bitCrushBlend)
    {
        for (int i=0; i < len; i++)
        {
            m_bitCrushPos += m_bitCrushDelta;
            if (m_bitCrushPos >= 1.0f)
            {
                m_bitCrushLastSample = (int)(out[i] * m_bitCrushBits);
                m_bitCrushPos -= 1.0f;
            }
            out[i] = ((float)m_bitCrushLastSample / (float)m_bitCrushBits)*m_bitCrushBlend + out[i]*(1.0-m_bitCrushBlend);
        }
    }
    
#ifdef DIGITS_MULTICORE
    pthread_mutex_unlock(&m_mutex);
#endif
}

void Voice::SetParameter(int index, float value)
{
   // Obtain Mutex
    
	VstCore::paramNums_t num = (VstCore::paramNums_t)index;
    int lfoSelect = -1;

	switch(num)
	{
		case VstCore::kO1Skew:
			m_pdOsc1.SetSkew(.5f + (value * .5f));
			break;
		case VstCore::kO1ShpMod:
		{
			float val;
			if (value < .25)
				val = .5f;
			else if (value < .5)
				val = 1.0f;
			else if (value < .75)
				val = 2.0f;
			else
				val = 3.0f;
			m_pdOsc1.SetShaperMod(val);
			break;
		}
		case VstCore::kO2Skew:
			m_pdOsc2.SetSkew(.5f + (value * .5f));
			break;
		case VstCore::kO2ShpMod:
		{
			float val;
			if (value < .25)
				val = .5f;
			else if (value < .5)
				val = 1.0f;
			else if (value < .75)
				val = 2.0f;
			else
				val = 3.0f;
			m_pdOsc2.SetShaperMod(val);
			break;
		}			
		case VstCore::kO2Vol:
			m_mix = value;
			break;
		case VstCore::kResoVol:
			m_resoVol = value;
			break;
		case VstCore::kResoWave:
			if (value < .25)
				m_resoGen.SetStyle(ResoGen::kSawQuarter);
			else if (value < .5)
				m_resoGen.SetStyle(ResoGen::kSawHalf);
			else if (value < .75)
				m_resoGen.SetStyle(ResoGen::kSquareQuarter);
			else 
				m_resoGen.SetStyle(ResoGen::kSquareHalf);
			break;
		case VstCore::kAmpAtt:
            m_ampAttack = value * kAttackRange + kAttackMinimumAmp;
			m_ampEnv.SetAttackExponential(m_ampAttack, m_sR);
			break;
        case VstCore::kAmpDec:
            m_ampDecay = value * kDecayRange + kDecayMinimum;
			m_ampEnv.SetDecayExponential(m_ampDecay, m_sR);
			break;
		case VstCore::kAmpRel:
            m_ampRelease = value * kReleaseRange + kReleaseMinimum;
			m_ampEnv.SetReleaseExponential(m_ampRelease, m_sR);
			break;
		case VstCore::kAmpSus:
            m_ampEnv.SetSustainLevel(value);
            break;
        case VstCore::kAmpFade:
			if (value == 1.0f)
				m_ampEnv.SetFadeDirect(1.0f);				
			else if (value > 0)
				m_ampEnv.SetFadeExponential(value * kFadeRange + kFadeMinimum, m_sR);
			else
				m_ampEnv.SetFadeDirect(0);
			break;
		case VstCore::kShpAtt:
            m_shpAttack = value * kAttackRange + kAttackMinimumShaper;
			m_shaperEnv.SetAttackExponential(m_shpAttack, m_sR);
			break;
        case VstCore::kShpDec:
            m_shpDecay = value * kDecayRange + kDecayMinimum;
			m_shaperEnv.SetDecayExponential(m_shpDecay, m_sR);
			break;
		case VstCore::kShpRel:
            m_shpRelease = value * kReleaseRange + kReleaseMinimum;
			m_shaperEnv.SetReleaseExponential(m_shpRelease, m_sR);
			break;
		case VstCore::kShpSus:
            m_shaperEnv.SetSustainLevel(value);
            break;
        case VstCore::kShpFade:
			if (value == 1.0f)
				m_shaperEnv.SetFadeDirect(1.0);
			else if (value > 0)
				m_shaperEnv.SetFadeExponential(1.0f * kFadeRange + kFadeMinimum, m_sR);
			else
				m_shaperEnv.SetFadeDirect(0);
			break;
		case VstCore::kShpMin:
			// TODO: Shouldn't these be in the envelope section?
			if (value == 0)
            {
				m_shaperEnv.NoAttack();
			}
			else
            {
				m_shaperEnv.NoAttack(false);
	//			m_shaperEnv.SetMinimum(value);
			}
			
			m_shpMin = value;
			break;
		case VstCore::kShpMax:
            m_shpMax = value;
			break;
		case VstCore::kFM:
			m_pdOsc2.SetFMInputAmt(4.0f * value);
			break;
		case VstCore::kDetune:
			m_detune = value * kDetuneMaximum;
			break;
		case VstCore::kOctave: {
			int oct = (int)(value*6.0f) - 1;
			m_octave = pow((float)2, (float)oct);
/*
			if (value < .25)
				m_octave = .5;
			else if (value < .5)
				m_octave = 1.0;
			else if (value < .75)
				m_octave = 2.0;
			else
				m_octave = 4.0;*/
			}
			break;
		case VstCore::kBasisWave:
			m_basisWave = value < .5? 0 : 1;
			SetBasisWaves();
			break;
		case VstCore::kPulseWidth:
			m_pulseWidth = kPWMMinimum + (value * kPWMRange);
			SetPulseWidths();
			break;
		case VstCore::kVelShaper:
			SetVelocityShp(value);
			break;
		case VstCore::kVelVolume:
			SetVelocityAmp(value);
			break;
		case VstCore::kLFORate:
            if (lfoSelect == -1) lfoSelect = 0;
#ifdef DIGITS_PRO
		case VstCore::kLFO2Rate:
            if (lfoSelect == -1) lfoSelect = 1;
		case VstCore::kLFO3Rate:
            if (lfoSelect == -1) lfoSelect = 2;
#endif
			value = ExpShape(100.0f, value);
			m_lfos[lfoSelect].SetRate(kLFOMinimum + (value * kLFORange), m_sR);
			break;
		case VstCore::kLFOType:
            if (lfoSelect == -1) lfoSelect = 0;
#ifdef DIGITS_PRO
		case VstCore::kLFO2Type:
            if (lfoSelect == -1) lfoSelect = 1;
		case VstCore::kLFO3Type:
            if (lfoSelect == -1) lfoSelect = 2;
#endif
            if (value < .25)
                m_lfos[lfoSelect].SetType(LFO::kLFO_Sin);
            else if (value < .5)
                m_lfos[lfoSelect].SetType(LFO::kLFO_Noise);
            else if (value < .75)
                m_lfos[lfoSelect].SetType(LFO::kLFO_RampDn);
            else
                m_lfos[lfoSelect].SetType(LFO::kLFO_RampUp);
			break;
		case VstCore::kLFODelay:
            if (lfoSelect == -1) lfoSelect = 0;
#ifdef DIGITS_PRO
		case VstCore::kLFO2Delay:
            if (lfoSelect == -1) lfoSelect = 1;
		case VstCore::kLFO3Delay:
            if (lfoSelect == -1) lfoSelect = 2;
#endif
			// Up to two seconds
			m_lfos[lfoSelect].SetDelay(value * 2000.0f, m_sR);
			break;
        case VstCore::kLFORepeat:
            if (lfoSelect == -1) lfoSelect = 0;
#ifdef DIGITS_PRO
        case VstCore::kLFO2Repeat:
            if (lfoSelect == -1) lfoSelect = 1;
        case VstCore::kLFO3Repeat:
            if (lfoSelect == -1) lfoSelect = 2;
#endif
            if (value == 1.0f)
                m_lfos[lfoSelect].SetRepeat(-1);
            else
                m_lfos[lfoSelect].SetRepeat(1+(value*4));
            break;
		case VstCore::kLFOShp:
            if (lfoSelect == -1) lfoSelect = 0;
#ifdef DIGITS_PRO
 		case VstCore::kLFO2Shp:
            if (lfoSelect == -1) lfoSelect = 1;
 		case VstCore::kLFO3Shp:
            if (lfoSelect == -1) lfoSelect = 2;
#endif
 			m_pdOsc1.SetLFOShaper(lfoSelect, value);
			m_pdOsc2.SetLFOShaper(lfoSelect, value);
			m_resoGen.SetLFOShaper(lfoSelect, value);
            if (value > 0)
                m_lfoIsAssigned[lfoSelect] |= kLFOa_toShaper;
            else
                m_lfoIsAssigned[lfoSelect] &= ~kLFOa_toShaper;
			break;
		case VstCore::kLFOAmp:
            if (lfoSelect == -1) lfoSelect = 0;
#ifdef DIGITS_PRO
		case VstCore::kLFO2Amp:
            if (lfoSelect == -1) lfoSelect = 1;
		case VstCore::kLFO3Amp:
            if (lfoSelect == -1) lfoSelect = 2;
#endif
			m_lfoAmplitude[lfoSelect] = value;
            if (value > 0)
                m_lfoIsAssigned[lfoSelect] |= kLFOa_toAmp;
            else
                m_lfoIsAssigned[lfoSelect] &= ~kLFOa_toAmp;

			break;
		case VstCore::kLFOFrq:
            if (lfoSelect == -1) lfoSelect = 0;
#ifdef DIGITS_PRO
		case VstCore::kLFO2Frq:
            if (lfoSelect == -1) lfoSelect = 1;
		case VstCore::kLFO3Frq:
            if (lfoSelect == -1) lfoSelect = 2;
#endif
			m_pdOsc1.SetLFOPitch(lfoSelect, value * .75f);
			m_pdOsc2.SetLFOPitch(lfoSelect, value * .75f);
			m_resoGen.SetLFOPitch(lfoSelect,  value);
            if (value > 0)
                m_lfoIsAssigned[lfoSelect] |= kLFOa_toPitch;
            else
                m_lfoIsAssigned[lfoSelect] &= ~kLFOa_toPitch;
			break;
		case VstCore::kLFOPWM:
            if (lfoSelect == -1) lfoSelect = 0;
#ifdef DIGITS_PRO
		case VstCore::kLFO2PWM:
            if (lfoSelect == -1) lfoSelect = 1;
		case VstCore::kLFO3PWM:
            if (lfoSelect == -1) lfoSelect = 2;
#endif
			m_pdOsc1.SetLFOPWM(lfoSelect, value);
			m_pdOsc2.SetLFOPWM(lfoSelect, value);
			m_resoGen.SetLFOPWM(lfoSelect, value);
            if (value > 0)
                m_lfoIsAssigned[lfoSelect] |= kLFOa_toPWM;
            else
                m_lfoIsAssigned[lfoSelect] &= ~kLFOa_toPWM;

			break;
#if DIGITS_PRO
        case VstCore::kLockShaperForFilter:
            m_lockShaperAmt = value;
            break;
        case VstCore::kFilterReso:
            // fudge factor for higher sampling rates-- dermined by ear
            if (m_sR >= 88200)
                value *= .93f;
            m_trueFilter.SetSharpness(3.5f * value);
            break;
        case VstCore::kFilterMode:
            if (value < .25)
                m_trueFilter.SetPoles(4);
            else if (value < .5)
                m_trueFilter.SetPoles(3);
            else if (value < .75)
                m_trueFilter.SetPoles(2);
            else
                m_trueFilter.SetPoles(1);
            break;
        case VstCore::kMonoMode:
            if (value < .25)
                m_glideMode = kGlide_PolyNoGlide;
            else if (value < .5)
                m_glideMode = kGlide_PolyGlideAlways;
            else if (value < .75)
                m_glideMode = kGlide_MonoGlideWhenHeld;
            else
                m_glideMode = kGlide_MonoGlideAlways;
            break;
        case VstCore::kPortaSpeed:
            if (value == 0)
                m_glideSpeed = 1.0f;
            else
                m_glideSpeed = value * 500.0f;
            break;
#endif
        case VstCore::kModShaper:
            SetModWheelShaper(value);
            break;
        case VstCore::kModLFO1Amp:
            SetModWheelLFO1Amp(value);
            break;
        case VstCore::kAftertouchShaper:
            SetAftertouchShaper(value);
            break;
        case VstCore::kAftertouchLFO1Amp:
            SetAftertouchLFO1Amp(value);
            break;
#if DIGITS_PRO
        case VstCore::kAmpRateScale:
            m_ampRateScale = value;
            break;
        case VstCore::kShpRateScale:
            m_shpRateScale = value;
            break;
        case VstCore::kShpLevelScale:
            m_shpLevelScale = value * .25f;
            break;
        case VstCore::kBitDiv:
            m_bitCrushPos = 0;
            m_bitCrushDivider = floor(value * 9.0f) + 1.0;
            break;
        case VstCore::kBitBit:
            m_bitCrushPos = 0;
            m_bitCrushBits = floor(value * 14.0f) + 2.0;
            break;
        case VstCore::kBitBlend:
            m_bitCrushBlend = value;
            break;
#endif
		default:
			break;
	}
}

void Voice::InitiateGlide(float delta, float deltaHi, float deltaLo)
{
    float unblend = 1.0f - m_deltaBlend;
    
    m_delta = m_delta*unblend + m_delta2*m_deltaBlend;
    m_deltaHi = m_deltaHi*unblend + m_deltaHi2*m_deltaBlend;
    m_deltaLo = m_deltaLo*unblend + m_deltaLo2*m_deltaBlend;
    
    m_delta2 = delta;
    m_deltaHi2 = deltaHi;
    m_deltaLo2 = deltaLo;
    m_deltaBlend = .001f; // Turn it on
}

