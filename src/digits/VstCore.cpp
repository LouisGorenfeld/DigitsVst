/* Phase Distortion Synthesizer (c) 2011 Louis Gorenfeld <louis.gorenfeld@gmail.com> */

#include <stdio.h>
#include <string>
#include <cmath>
#include <cassert>
#include "VstCore.h"
#include "Voice.h"
//#include "LGDebug.h"
#include "Tables.h"
#include "Patches.h"

#if !defined(FAUX_VST_HEADER) && !defined(NO_EDITOR)
#include "../vstgui/plugin-bindings/aeffguieditor.h" // createEditor
#endif

float VstCore::s_freqDelta[kOctaves*12];
const float kDelayMinimum = 50.0f;
const float kDelayRange = 400.0f;

const float kChorusWetMax = 1.5;
const float kChorusMinDelay = 5.0;
const float kChorusRange = 25.0;
const float kFlangerFeedbackMax = .8;
const float kFlangerMinDelay = .5;
const float kFlangerRange = 9.5;
// To the 9th power to make the curve easier to work with
const float kMinMaxFilterCurve = 9.0;

#ifdef DBGLOOP
#define DEBUG_CORELOOP(...) { fprintf(stderr, __VA_ARGS__); }
#define DEBUGFRAMESEPARATOR
#else
#define DEBUG_CORELOOP
#endif

#ifdef DEBUGFRAMESEPARATOR
#define DEBUG_FRAMESEPARATOR(...) { fprintf(stderr, __VA_ARGS__); }
#else
#define DEBUG_FRAMESEPARATOR
#endif

#if _MSC_VER
#define snprintf _snprintf
#endif

const char* VstCore::s_paramNames[] = { 
			  "O1Skew", "O1ShpMod", "O2Skew", "O2ShpMod", "Mix", "ResetOscs",
			  "ResoVol", "ResoWave", "AmpAtt", "AmpDec", "AmpSus", "AmpFade",
              "AmpRel", "ShpAtt", "ShpDec", "ShpSus", "ShpFade", "ShpRel",
              "ShpMin", "ShpMax", "FM", "Detune", "Coarse",
              "BasisWave", "PulseWidth", "VelShaper", "VelVolume", "ModShaper",
              "ModLFO1Amp",
#ifdef DIGITS_PRO
                "ATShaper", "ATLFO1Amp",
#endif
               "LFORate", "LFOType", "LFODelay",
              "LFORepeat", "LFOShp", "LFOAmp", "LFOFreq", "LFOPWM",
#ifdef DIGITS_PRO
    "LFO2Rate", "LFO2Type", "LFO2Delay", "LFO2Repeat", "LFO2Shp", "LFO2Amp", "LFO2Freq", "LFO2PWM",
    "LFO3Rate", "LFO3Type", "LFO3Delay", "LFO3Repeat", "LFO3Shp", "LFO3Amp", "LFO3Freq", "LFO3PWM",
#endif
              "Unison", "Del-L", "Del-R", "DelWet",
                "DelFB", "DelLoss",
            "ChrRate", "ChrDepth", "ChrDirt", "ChrTone", "ChrWet", "ChrFlang",
            "BitDiv", "BitBits", "BitWet",
#ifdef DIGITS_PRO
    "LockShaperAt", "FilterReso", "FilterMode", "MonoMode", "PortaSpd",
    "AmpRateScale", "ShpRateScale", "ShpLevScale",
    "PbendUp", "PbendDn",
#endif
                "Gain"
};

// List of parameter names for GetChunk/SetChunk routine. Then we
// can change what is displayed to the user without problems
// NOTE: ShpDec0 is Decay, ShpDec is Release. This is because the
// envelopes used to be trapezoidal but no longer are
const char* VstCore::s_paramNamesChunk[] = { 
	"O1Skew", "O1ShpMod", "O2Skew", "O2ShpMod", "Mix", "ResetOscs",
	"ResoVol", "ResoWave", "AmpAtt", "AmpDec0", "AmpSus", "AmpFade",
    "AmpDec", "ShpAtt", "ShpDec0", "ShpSus", "ShpFade", "ShpDec",
    "ShpMin", "ShpMax", "FM", "Detune", "Coarse",
    "BasisWave", "PulseWidth", "VelShaper", "VelVolume", "ModShaper",
    "ModLFO1Amp",
#ifdef DIGITS_PRO
    "ATShaper", "ATLFO1Amp",
#endif
    "LFORate", "LFOType", "LFODelay",
	"LFORepeat", "LFOShp", "LFOAmp", "LFOFreq", "LFOPWM", 
#ifdef DIGITS_PRO
    "LFO2Rate", "LFO2Type", "LFO2Delay",
	"LFO2Repeat", "LFO2Shp", "LFO2Amp", "LFO2Freq", "LFO2PWM",
    "LFO3Rate", "LFO3Type", "LFO3Delay",
	"LFO3Repeat", "LFO3Shp", "LFO3Amp", "LFO3Freq", "LFO3PWM",
#endif
    "Unison", "Del-L", "Del-R", "DelWet", "DelFB",
    "DelLoss",
    "ChrRate", "ChrDepth", "ChrDirt", "ChrTone", "ChrWet", "ChrFlang",
    "BitDiv", "BitBits", "BitWet",
#ifdef DIGITS_PRO
    "LockShaperAt", "FilterReso", "FilterMode", "MonoMode", "PortaSpd",
    "AmpRateScale", "ShpRateScale", "ShpLevScale",
    "PbendUp", "PbendDn",
#endif
    "Gain"
};

// --------- START OF DIGITS_MULTICORE
#ifdef DIGITS_MULTICORE
void* digits_helper(void* args)
{
    VstCore::HelperThread_t* helper = (VstCore::HelperThread_t*)args;
    helper->ThreadCore();
    pthread_exit((void*)0);
}

VstCore::HelperThread_t::HelperThread_t(VstCore* core, Voice::voiceBuffers_t* bufs, int rt_prio) :
    m_bufs(bufs),
    m_outBuf(0),
    m_chunkLen(0),
    m_vstCore(core),
    m_quitNow(false),
    m_didProcessing(false),
    m_curBuf(0)
{
    int err = pthread_mutex_init(&m_beginChunkMutex, 0);
    if (err)
        printf("Error creating mutex 1\n");
    if (!err)
        err = pthread_cond_init(&m_beginChunkCond, 0);
    if (err)
        printf("Error creating cond 1\n");

    if (!err)
    {
        err = pthread_create(&m_thread, 0, digits_helper, (void*)this);
        if (err)
            printf("Error creating helper thread: %d\n", err);
        struct sched_param param;
        param.sched_priority = rt_prio;
        DEBUG_MCORE("Helper thread priority %d (round-robin)\n", rt_prio);
        err = pthread_setschedparam(m_thread, SCHED_RR, &param);
    }
    else
    {
        printf("Error creating helper mutex: %d\n", err);
    }
}

VstCore::HelperThread_t::~HelperThread_t()
{
    pthread_join(m_thread, 0);
}

void VstCore::HelperThread_t::ThreadCore()
{
    while (!m_quitNow)
    {
        DEBUG_MCORE("h: get lock\n");
        pthread_mutex_lock(&m_beginChunkMutex);
        DEBUG_MCORE("h: wait on cond\n");
        pthread_cond_wait(&m_beginChunkCond, &m_beginChunkMutex);
        if (m_quitNow)
            break;
        
        if (m_curBuf != m_vstCore->CurBuf())
        {
            pthread_mutex_unlock(&m_beginChunkMutex);
            DEBUG_MCORE("h: Woke up too late; unlocking and sleeping\n");
            continue;
        }
        DEBUG_MCORE("h: doing processing for buf %u\n", m_curBuf);
        m_didProcessing = true;
        Tables::ZeroMem(m_outBuf, m_chunkLen);
        for (int i=0; i<VstCore::kNumVoices; i++)
        {
            if (!(m_vstCore->VoiceIsSubVoice(i) || m_vstCore->VoiceIsFree(i)))
            {
                // This will do the IsFree check
                m_vstCore->VoiceRender(i, m_bufs->m_voiceBuf->Data(), m_chunkLen, m_bufs, i, 1);
                //m_vstCore->VoiceRender(i, m_outBuf, m_chunkLen, m_bufs, i, 1);
                // Add in new chunk
                for (int j=0; j<m_chunkLen; j++)
                    m_outBuf[j] += m_bufs->m_voiceBuf->Data()[j];
            }
        }
        DEBUG_MCORE("h: release lock (no longer processing)\n");
        pthread_mutex_unlock(&m_beginChunkMutex);
    }
}

#endif
// --------- END OF DIGITS_MULTICORE

AudioEffect* createEffectInstance(audioMasterCallback audioMaster)
{
	return new VstCore(audioMaster);
}

VstCore::VstCore(audioMasterCallback audioMaster) :
	AudioEffectX(audioMaster, 128, kNumParams),
    m_isMono(false),
    m_alwaysGlide(false),
    m_sustainDown(false),
	m_timestampClock(0),
	m_nextSpan(0),
	m_patchData(0),
	m_lastStatus(0),
    m_phatLite(false),
    m_lastPitch(0),
    m_bendUpAmt(2),
    m_bendDnAmt(2),
    m_resetOscs(false),
    m_curBuf(0)
{
//	TODO: Needed?
//	setProgram(0);
    
#ifdef DIGITS_MULTICORE
    m_numCores = 2;
    for (int i=1; i<m_numCores; i++)
        m_helpers[i-1] = 0;
#else
    m_numCores = 1;
#endif

	if (audioMaster)
	{
		setNumInputs(0);
		setNumOutputs(2);
		canProcessReplacing();
		isSynth();
		setUniqueID('LGPd');
		programsAreChunks(true);
	}
	suspend();
	
	// Initialize the buffers before we do anything else,
	// or we'll get large allocations which will likely fail
	// (or be slow)
    printf("Setting initial block size...\n");
	setBlockSize(1024);
    printf("Init buffers...\n");
    for (int i=0; i<m_numCores; i++)
    {
        m_bufSets[i].m_voiceBuf = new SndBuf(m_manager, "voice scratch");
        m_bufSets[i].m_shpEnvBuf = new SndBuf(m_manager, "shaper contour");
        m_bufSets[i].m_ampEnvBuf = new SndBuf(m_manager, "amp contour");
        m_bufSets[i].m_lfoBufs[0] = new SndBuf(m_manager, "lfo1");
    #ifdef DIGITS_PRO
        m_bufSets[i].m_lfoBufs[1] = new SndBuf(m_manager, "lfo2");
        m_bufSets[i].m_lfoBufs[2] = new SndBuf(m_manager, "lfo3");
    #endif
        m_bufSets[i].m_fmScratchBuf = new SndBuf(m_manager, "fm scratch");
        
        m_helperOutBufs[0] = new SndBuf(m_manager, "helper1 out");
    }
    
    m_chorusDryBuf = new SndBuf(m_manager, "chorusDry");
	
    printf("Init tables...\n");
	Tables::Init();

    printf("Init voices...\n");
	for (int i=0; i<kNumVoices; i++)
	{
        m_voices[i] = new Voice(i);
	}

	m_midiEvents.reserve(128);
	
    printf("Setting patch to default...\n");
    ZeroPatch();

	// Refresh patch/bank caches
	bool ok;
    m_pbList.GetBankList();
    // Try to load the InitTone
    ok = (m_pbList.SetCurBankDir("DemoBank") > -1);
    m_pbList.GetPatchList();
    ok = (m_pbList.SetCurPatch("InitTone") > -1);
    m_curProgramName = "InitTone";
    if (!ok || !LoadFXP(m_pbList.GetCurBankDir() + m_pbList.GetCurPatchName() + ".fxp"))
    {
        // Set sane defaults
        setParameter(kPulseWidth, .5f);
        setParameter(kO2Vol, .5f);
        setParameter(kOctave, .25f);
        setParameter(kAmpSus, 1.0f);
        setParameter(kAmpFade, 1.0f);
        setParameter(kShpDec, .2f);
        setParameter(kShpSus, 1.0f);
        setParameter(kShpRel, .2f);
        setParameter(kShpMin, .5f);
        setParameter(kShpMax, 1.0f);
        setParameter(kO1ShpMod, .25f);
        setParameter(kO2ShpMod, .25f);
        setParameter(kLFORepeat, 1.0f);
        setParameter(kLFO2Repeat, 1.0f);
        setParameter(kLFO3Repeat, 1.0f);
        setParameter(kPitchbendUp, 2.0f/24.0f);
        setParameter(kPitchbendDn, 2.0f/24.0f);
        
		m_curProgram = m_pbList.GetCurPatch();
    }
    else
    {
        m_curProgram = m_pbList.GetCurPatch();
    }

    memset(m_noteBits, 0, sizeof(m_noteBits));
    memset(m_keyBits, 0, sizeof(m_keyBits));

    printf("Finishing up...\n");
	memset(m_dc_out, 0, sizeof(float)*2);
	memset(m_dc_in, 0, sizeof(float)*2);

	// Find DC filter coefficients for zero at 0 and
	// pole at close-to-0
	// H(z) = 1-R2cosAz^-1 + R2^2z^-2/1-R2cosBz^-1 + R2^2z^-2
	// TODO: This could probably happily be a 1-pole filter and save
	// some CPU
	// TODO: Frequency shouldn't vary with the sampling rate, but
	// it appears well-behaved enough at both 44khz and 96khz extremes.
	// If this changes, please retest it on a bassy system!!
	m_dc_a[0] = -2.0; // from -2Rcos(a) where R=1 and a=0
	m_dc_a[1] = 1.0f;
	m_dc_b[0] = -2.0f * .995f; // from -2Rcos(a) where R=.99 and a=0
	m_dc_b[1] = .995f * .995f;
	
	m_delL = 0;
	m_delR = 0;
	m_del_wL = 0;
	m_del_wR = 0;
	m_del_lastL[0] = 0;
	m_del_lastL[1] = 0;
	m_del_lastL[2] = 0;
	m_del_lastL[3] = 0;
	m_del_lastR[0] = 0;
	m_del_lastR[1] = 0;
	m_del_lastR[2] = 0;
	m_del_lastR[3] = 0;

	// Assume the sample rate is 44khz for compatibility
    // with Symbiosis AU wrapper (the VST doesn't seem to
    // get a request to change sample rate if it's 44khz)
	setSampleRate(44100);

    m_chorus[0].SetLFORate(.15);
    m_chorus[1].SetLFORate(.15);
    m_chorus[0].SetWet(.8);
    m_chorus[1].SetWet(.8);
    m_chorus[1].SetOffset(.4);
 
#if !defined(FAUX_VST_HEADER) && !defined(NO_EDITOR)
	if (!getenv("DIGITSVST_NO_EDITOR"))
	{
		extern AEffGUIEditor* createEditor (AudioEffectX*);
    	setEditor (createEditor (this));
	}
#endif
	
	printf("Extent of the Jam: Digits Copyright (c) 2016 Louis Gorenfeld <louis.gorenfeld@gmail.com>\nVersion %d.%d\nextentofthejam.com\n\n", VERSION_MAJ, VERSION_MIN);
}

void VstCore::setProgram(VstInt32 program)
{
    return;
    
    PatchBankList::fileList_t patchNames = m_pbList.GetPatchList();
    m_curProgram = program;
#ifdef BUILTINPATCHES
    setChunk(presetBank[program], 512);
#else
	if (program >= 0 && program < patchNames.size())
	{
        m_pbList.SetCurPatch(program);
        //printf("Loading %s\n", std::string(m_pbList.GetCurBankDir() + m_pbList.GetCurPatchName() + ".fxp").c_str());
        if (!LoadFXP(m_pbList.GetCurBankDir() + m_pbList.GetCurPatchName() + ".fxp"))
            printf("Failed to load %s\n", m_pbList.GetCurPatchName().c_str());

        m_curProgramName = patchNames[m_curProgram];
 	}
    else
    {
        m_curProgramName = "<empty>";
    }
#endif
}

void VstCore::SetChorusFlanger(bool isChorus)
{
    if (m_chorusMode == isChorus)
        return;

    m_chorusMode = isChorus;

    if (isChorus)
    {
        m_chorus[0].SetMinDelay(kChorusMinDelay);
        m_chorus[1].SetMinDelay(kChorusMinDelay);
        float amt = (m_chorus[0].GetRange()/kFlangerRange) * kChorusRange;
        m_chorus[0].SetRange(amt);
        m_chorus[1].SetRange(amt);
        // Set the regen as the wet/dry
        amt = (m_chorus[0].GetFeedback()/kFlangerFeedbackMax) * kChorusWetMax;
        m_chorus[0].SetWet(amt);
        m_chorus[1].SetWet(amt);
        m_chorus[0].SetFeedback(0);
        m_chorus[1].SetFeedback(0);
    }
    else
    {
        m_chorus[0].SetMinDelay(kFlangerMinDelay);
        m_chorus[1].SetMinDelay(kFlangerMinDelay);
        float amt = (m_chorus[0].GetRange()/kChorusRange) * kFlangerRange;
        m_chorus[0].SetRange(amt);
        m_chorus[1].SetRange(amt);
        // Set the wet/dry as the regen
        amt = (m_chorus[0].GetWet()/kChorusWetMax) * kFlangerFeedbackMax;
        m_chorus[0].SetFeedback(amt);
        m_chorus[1].SetFeedback(amt);
        m_chorus[0].SetWet(1.0);
        m_chorus[1].SetWet(1.0);
    }
}

void VstCore::setProgramName(char *name)
{
}

void VstCore::getProgramName(char *name)
{
    vst_strncpy(name, m_curProgramName.c_str(), kVstMaxProgNameLen);
}

void VstCore::ZeroPatch()
{
	memset(m_simpleProgram, 0, sizeof(float)*kNumParams);
	for (int i=0; i<kNumParams; i++)
		setParameter(i, 0);
	
	// Set gain to -3dB if it's not set
	setParameter(VstCore::kGain, .707f);
}


void VstCore::ApplyDC(float *in, int len)
{
	float sam;
	for (int i=0; i<len; i++)
	{
		// Denormal prevention: mix in very small random noise signal
		in[i] += (rand() & 63) * .0000000596f;

		sam = in[i] + m_dc_a[0]*m_dc_in[0] + m_dc_a[1]*m_dc_in[1]
			  - m_dc_b[0]*m_dc_out[0] - m_dc_b[1]*m_dc_out[1];
		
		m_dc_in[1] = m_dc_in[0];
		m_dc_in[0] = in[i];
		m_dc_out[1] = m_dc_out[0];
		m_dc_out[0] = sam;
		
		in[i] = sam;
	}
}

VstCore::~VstCore()
{
#ifdef DIGITS_MULTICORE
    // TODO: Support more than 2 cores
    if (m_helpers[0])
    {
        m_helpers[0]->Quit();
    }
#endif
    
    for (int i=0; i<m_numCores; i++)
    {
        delete m_bufSets[i].m_voiceBuf;
        delete m_bufSets[i].m_shpEnvBuf;
        delete m_bufSets[i].m_ampEnvBuf;
        delete m_bufSets[i].m_lfoBufs[0];
#ifdef DIGITS_PRO
        delete m_bufSets[i].m_lfoBufs[1];
        delete m_bufSets[i].m_lfoBufs[2];
#endif
        delete m_bufSets[i].m_fmScratchBuf;
	}
    
	for (int i=0; i<kNumVoices; i++) {
		if (m_voices[i])
			delete m_voices[i];
	}
	if (m_patchData)
		delete[] m_patchData;
	if (m_delL)
		delete[] m_delL;
	if (m_delR)
		delete[] m_delR;
}

void VstCore::CalculateFreqDeltas()
{
	// Initialize frequency table
	// Frequencies in hz for octave -2 (note=36 C)
	float raw[12] = { 65.4063913251, 69.2956577442, 73.4161919794, 77.7817459305,
		82.4068892282, 87.3070578583, 92.4986056779, 97.9988589954,
		103.8261743950, 110.0000000000, 116.5409403795, 123.47 };
	// Note 0 is at octave -5
	float octave = -4;
	for (int j=0; j<kOctaves; j++)
	{
		for (int i=0; i<12; i++)
		{
			int index = (j*12) + i;
			if (index > 127)
				break;
			s_freqDelta[index] = (raw[i] * pow(2, octave)) / m_manager.GetSamplingRate();
		}
		octave++;
	}
}

float VstCore::GetDelta(VstInt32 freq)
{
	if (freq >= kOctaves*12)
		freq = (kOctaves*12) - 1;
    if (freq < 0)
        freq = 0;
	
	return s_freqDelta[freq];
}

int VstCore::NextFreeVoice()
{
	// - Look for a completely free slot
	// - Look for the oldest note which is in release
	// - Failing that, steal the oldest note
	for (int i=0; i<kNumVoices; i++)
	{
		if (m_voices[i]->IsFree())
			return i;
	}

	//This is UINT64_MAX-- I thought it'd be more multiplatform-
	//friendly if I just hardcoded this one
	uint64_t oldestTime = 18446744073709551615ULL;
	uint64_t oldestVoice = kNumVoices;
	for (int i=0; i<kNumVoices; i++)
	{
		if (m_voices[i]->GetTimestamp() < oldestTime && m_voices[i]->IsReleased())
		{
			oldestTime = m_voices[i]->GetTimestamp();
			oldestVoice = i;
		}
	}
	if (oldestVoice < (unsigned int)kNumVoices)
		return oldestVoice;
	
	oldestTime = 18446744073709551615ULL;
	oldestVoice = 0;
	for (int i=0; i<kNumVoices; i++)
	{
		if (m_voices[i]->GetTimestamp() < oldestTime)
		{
			oldestTime = m_voices[i]->GetTimestamp();
			oldestVoice = i;
		}
	}
	
	return oldestVoice;
}

bool VstCore::IsKeyDown(int pitch)
{
    return (m_keyBits[pitch/32]) & (1 << (pitch & 31));
}

bool VstCore::IsNoteOn(int pitch)
{
    return (m_noteBits[pitch/32]) & (1 << (pitch & 31));
}

void VstCore::KeyDown(int pitch)
{
    int keyWord = pitch/32;
    uint32_t keyBitMask = 1 << (pitch&31);
    m_keyBits[keyWord] |= keyBitMask;
}

void VstCore::KeyUp(int pitch)
{
    int keyWord = pitch/32;
    uint32_t keyBitMask = ~(1 << (pitch&31));
    m_keyBits[keyWord] &= keyBitMask;
}

void VstCore::NoteOn(int pitch, int velocity)
{
	int i;
    float variance[4];
    if (m_phatLite && !m_isMono)
    {
        variance[0] = m_orch * .025 * GetDelta(pitch);
        variance[1] = -m_orch * .025 * GetDelta(pitch);
    }
    else
    {
        variance[0] = 0;
        variance[1] = m_orch * .025 * GetDelta(pitch);
        variance[2] = -m_orch * .025 * GetDelta(pitch);
        variance[3] = ((((rand()>>16)&65535)/32767.0f) - 1.0f) * m_orch * .025 * GetDelta(pitch);
    }

	if (!m_isMono)
    {
        // Turn off previous version of this voice if it exists
        for (i=0; i<kNumVoices; i++)
        {
            NoteOff(pitch);
        }
    }

	if (m_orch > 0)
	{
        if (m_isMono)
        {
            if (FindLowestNote() == -1)
            {
                // This isn't obeying m_phatLite -- any system can handle this
                for (int i=0; i<4; i++)
                {
                    if (m_alwaysGlide)
                    {
                        m_voices[i]->NoteOn(m_timestampClock, GetDelta(m_lastPitch-m_bendDnAmt) + variance[i], GetDelta(m_lastPitch+m_bendUpAmt) + variance[i], GetDelta(m_lastPitch) + variance[i], pitch, velocity, m_resetOscs);
                        m_voices[i]->InitiateGlide(GetDelta(pitch)+variance[i], GetDelta(pitch+m_bendUpAmt)+variance[i], GetDelta(pitch-m_bendDnAmt)+variance[i]);
                    }
                    else
                    {
                        m_voices[i]->NoteOn(m_timestampClock, GetDelta(pitch-m_bendDnAmt)+variance[i], GetDelta(pitch+m_bendUpAmt)+variance[i], GetDelta(pitch)+variance[i], pitch, velocity, m_resetOscs);
                    }
                }
                
                m_voices[0]->SetIsSubVoice(false);
                m_voices[0]->SetNextSubVoice(m_voices[1]);
                m_voices[1]->SetNextSubVoice(m_voices[2]);
                m_voices[2]->SetNextSubVoice(m_voices[3]);
                m_voices[3]->SetNextSubVoice(0);
            }
            else
            {
                for (int i=0; i<4; i++)
                {
                    m_voices[i]->InitiateGlide(GetDelta(pitch)+variance[i], GetDelta(pitch+m_bendUpAmt)+variance[i], GetDelta(pitch-m_bendDnAmt)+variance[i]);
                }
            }
        }
        else // orch on, but not mono
        {
            int i[4];
            int numVoices = m_phatLite? 2 : 4;
            for (int j=0; j<numVoices; j++)
            {
                i[j] = NextFreeVoice();
                if (m_alwaysGlide)
                {
                    m_voices[i[j]]->NoteOn(m_timestampClock, GetDelta(m_lastPitch-m_bendDnAmt)+variance[j], GetDelta(m_lastPitch+m_bendUpAmt)+variance[j], GetDelta(m_lastPitch), pitch, velocity, m_resetOscs);
                    m_voices[i[j]]->InitiateGlide(GetDelta(pitch)+variance[j], GetDelta(pitch+m_bendUpAmt)+variance[j], GetDelta(pitch-m_bendDnAmt)+variance[j]);
                }
                else
                {
                    m_voices[i[j]]->NoteOn(m_timestampClock, GetDelta(pitch-m_bendDnAmt)+variance[j], GetDelta(pitch+m_bendUpAmt)+variance[j], GetDelta(pitch)+variance[j], pitch, velocity, m_resetOscs);
                }
            }
            m_lastPitch = pitch;
            
            // Link 'em up so that the first voice points to the next subvoice (MUST GO FORWARDS because of how the
            // render loop works. Never change the render loop to process voices in a different order or this code
            // will have to be reworked too)
            m_voices[0]->SetIsSubVoice(false);
            m_voices[i[0]]->SetNextSubVoice(m_voices[i[1]]);
            if (m_phatLite)
            {
                m_voices[i[1]]->SetNextSubVoice(0);
            }
            else
            {
                m_voices[i[1]]->SetNextSubVoice(m_voices[i[2]]);
                m_voices[i[2]]->SetNextSubVoice(m_voices[i[3]]);
                m_voices[i[3]]->SetNextSubVoice(0);
            }
        }
	}
	else // orch not on
	{
        if (m_isMono)
        {
            if (FindLowestNote() == -1)
            {
                if (m_alwaysGlide)
                {
                    m_voices[0]->NoteOn(m_timestampClock, GetDelta(m_lastPitch-m_bendDnAmt), GetDelta(m_lastPitch+m_bendUpAmt), GetDelta(m_lastPitch), pitch, velocity, m_resetOscs);
                    m_voices[0]->InitiateGlide(GetDelta(pitch), GetDelta(pitch+m_bendUpAmt), GetDelta(pitch-m_bendDnAmt));
                }
                else
                {
                    m_voices[0]->NoteOn(m_timestampClock, GetDelta(pitch-m_bendDnAmt), GetDelta(pitch+m_bendUpAmt), GetDelta(pitch), pitch, velocity, m_resetOscs);
                }

            }
            else
            {
                m_voices[0]->InitiateGlide(GetDelta(pitch), GetDelta(pitch+m_bendUpAmt), GetDelta(pitch-m_bendDnAmt));
            }
            m_voices[0]->SetNextSubVoice(0);
        }
        else
        {
            int i = NextFreeVoice();
            fprintf(stderr, "next free voice %d. isSubVoice %d\n", i, (int)m_voices[i]->IsSubVoice());
            if (m_alwaysGlide) {
                m_voices[i]->NoteOn(m_timestampClock, GetDelta(m_lastPitch-m_bendDnAmt), GetDelta(m_lastPitch+m_bendUpAmt), GetDelta(m_lastPitch), pitch, velocity, m_resetOscs);
                m_voices[i]->InitiateGlide(GetDelta(pitch), GetDelta(pitch+m_bendUpAmt), GetDelta(pitch-m_bendDnAmt));
            }
            else {
                m_voices[i]->NoteOn(m_timestampClock, GetDelta(pitch-m_bendDnAmt), GetDelta(pitch+m_bendUpAmt), GetDelta(pitch), pitch, velocity, m_resetOscs);
            }
            m_voices[i]->SetNextSubVoice(0);
            m_lastPitch = pitch;
        }
	}
	
	// Toggle bit on for notes-sounding map
    int keyWord = pitch/32;
    uint32_t keyBitMask = 1 << (pitch&31);
	m_noteBits[keyWord] |= keyBitMask;
	m_timestampClock++;
}

void VstCore::NoteOff(int pitch)
{
	int keyWord = pitch/32;
    uint32_t keyBitMask = ~(1 << (pitch&31));
	m_noteBits[keyWord] &= keyBitMask;

    if (m_isMono)
    {
        float lowest = FindLowestNote();
        if (lowest != -1)
        {
            if (m_orch)
            {
                float variance = 0;
                
                m_voices[0]->InitiateGlide(GetDelta(lowest), GetDelta(lowest+m_bendUpAmt), GetDelta(lowest-m_bendDnAmt));
                variance = m_orch * .025 * GetDelta(lowest);
                m_voices[1]->InitiateGlide(GetDelta(lowest)+variance, GetDelta(lowest+m_bendUpAmt)+variance, GetDelta(lowest-m_bendDnAmt)+variance);
                variance = -m_orch * .025 * GetDelta(pitch);
                m_voices[2]->InitiateGlide(GetDelta(lowest)+variance, GetDelta(lowest+m_bendUpAmt)+variance, GetDelta(lowest-m_bendDnAmt)+variance);
                variance = ((((rand()>>16)&65535)/32767.0f) - 1.0f) * m_orch * .025 * GetDelta(pitch);
                m_voices[3]->InitiateGlide(GetDelta(lowest)+variance, GetDelta(lowest+m_bendUpAmt)+variance, GetDelta(lowest-m_bendDnAmt)+variance);
            }
            else
            {
                m_voices[0]->InitiateGlide(GetDelta(lowest), GetDelta(lowest+m_bendUpAmt), GetDelta(lowest-m_bendDnAmt));
            }
        }
        else
        {
            // Unconditional note off, just for safety
            m_voices[0]->NoteOff(-1);
        }
    }
    else
    {
        for (int i=0; i<kNumVoices; i++)
            m_voices[i]->NoteOff(pitch);
    }
}

void VstCore::ReleaseAllHeldNotes()
{
    for (int key = 0; key < 128; key++)
    {
        if (IsNoteOn(key) && !IsKeyDown(key))
            NoteOff(key);
    }
}

int VstCore::FindLowestNote()
{
	int i, j=0;
	bool found = false;
	for (i=0; i<4; i++)
	{
		if (m_noteBits[i] != 0)
		{
			int word = m_noteBits[i];
			for (j=0; j<32; j++)
			{
				if (word & 1<<j)
				{
					found = 1;
					break;
				}
			}
			if (found)
				break;
		}
	}
	
	if (i==4) {
		return -1;
	}
    
	return (i*32) + j;
}

void VstCore::PitchBend(float value)
{
	for (int i=0; i<kNumVoices; i++)
		m_voices[i]->PitchBend(value);
}

#ifdef DIGITS_MULTICORE
void VstCore::StartHelper(int helperIndex)
{
    int policy;
    struct sched_param param;
    
    pthread_getschedparam(pthread_self(), &policy, &param);
    m_helpers[helperIndex] = new HelperThread_t(this, &m_bufSets[helperIndex+1], param.sched_priority);
    m_helpers[helperIndex]->SetOutputBuffer(m_helperOutBufs[0]->Data());
}
#endif

void VstCore::HandleMidiEvent(char* midiData)
{
	VstInt32 status = midiData[0] & 0xf0;
    
	// Running status (see MIDI spec)
	if (status < 0x80)
	{
		midiData[2] = midiData[1];
		midiData[1] = midiData[0];
		midiData[0] = status = m_lastStatus;
	}
	
	switch (status)
	{
		case 0x80: // Note off 
		{
            VstInt32 key = midiData[1];
            if (!m_sustainDown)
			    NoteOff(key);
            KeyUp(key);
			break;
		}
		case 0x90: // Note on
		{
			VstInt32 key = midiData[1];
			VstInt32 vel = midiData[2];
			if (vel > 0)
            {
                // actual note-on event
                NoteOn(key, vel);
                KeyDown(key);
            }
            else
            {
                // "note-on" with vel=0 means "note-off"
                if (!m_sustainDown)
                    NoteOff(key);
                KeyUp(key);
            }
            break;
		}
        case 0xA0:  // Aftertouch (poly)
        {
            int note = midiData[1];
            float pressure = (float)midiData[2]/127.0f;
            for (int i=0; i<kNumVoices; i++)
            {
                if (m_voices[i]->Pitch() == note)
                    m_voices[i]->SetAftertouch(pressure);
            }
            break;
        }
		case 0xB0:  // CC
		{
			if (midiData[1] == 1) // Mod wheel
			{
                // Read modwheel backwards since the lowest position (midi 0) is neutral
               // m_modWheel = 1.0f - ((float)midiData[2]/127.0f);
                float modWheel = (float)midiData[2]/127.0f;
                for (int i=0; i<kNumVoices; i++)
                    m_voices[i]->SetModWheel(modWheel);
			}
			else if (midiData[1] == 0x07) // Volume
			{
				// TODO: FIX THIS
				float gain = (float)midiData[2] / 127.0f;
				setParameter(VstCore::kGain, gain);
			}
            else if (midiData[1] == 0x40) // sustain pedal
            {
                bool susDown = (midiData[2] > 0);
                if (!susDown && m_sustainDown)
                    ReleaseAllHeldNotes();
                m_sustainDown = susDown;
            }
            break;
		}
        case 0xC0: // PrgCh
        {
            setProgram(midiData[1]);
            break;
        }
        case 0xD0:  // Aftertouch (channel)
        {
            float pressure = (float)midiData[1]/127.0f;
            for (int i=0; i<kNumVoices; i++)
                m_voices[i]->SetAftertouch(pressure);
            break;
        }
        case 0xE0:	// Pitch bend
		{
			float value;
            
            if (midiData[2] == 127)
                value = 1.0f;
            else if (midiData[2] == 0)
                value = -1.0f;
            else
                value = (midiData[2] - 64) / 64.0f;
            
			PitchBend(value);
            break;
		}
		default:
			break;
	}
	
	if (status >= 0x80 && status <= 0xEF)
		m_lastStatus = status;
}

void VstCore::processReplacing(float **ins, float **outs, VstInt32 frames)
{
#ifdef DIGITS_BENCHMARK
	CPUBench cpuBench;
#endif
	static int framesElapsed = 0;
	framesElapsed += frames;
	
	float* wL = outs[0];
	float* wR = outs[1];
	
	int len;
	int framesLeft = frames;
	
	unsigned int numProcessed = 0;
	
	midiEvents_t::iterator it = m_midiEvents.begin();
	
	// Even out the gain for unison mode and filter mode
	float gain;
	if (m_orch)
		gain = .22f * m_gain;
	else
		gain = .5f * m_gain;
    
    if (m_voices[0]->ShaperLocked())
        gain *= 1.3;
    
    // Initialize helpers when needed
    // i=0 would be the main thread, which isn't a helper
#ifdef DIGITS_MULTICORE
    for (int i=1; i<m_numCores; i++)
    {
        if (m_helpers[i-1] == 0)
        {
            // This does heap allocation and starts a thread. Hopefully it's OK to do it once here.
            StartHelper(i-1);
        }
    }
#endif

    int voicesProcessed = 0;
    
    Tables::ZeroMem(wL, frames);

	do
	{
		if (m_writeLeft < 1)
		{
			// New control period
			m_writeLeft += m_manager.GetControlSize();
			// NextSpan is where this current control chunk ends and
			// the next one begins (used to see what MIDI evts to trigger)
			m_nextSpan += m_manager.GetControlSize();
		}
		len = m_writeLeft < framesLeft? m_writeLeft : framesLeft;
		
		// Check for events for this upcoming control chunk
		while (it != m_midiEvents.end())
		{
			if (it->deltaFrames > m_nextSpan)
				break;
			
			HandleMidiEvent(it++->midiData);
			numProcessed++;
		}
		
		float *bookmarkL = wL;
		float *bookmarkR = wR;
		
        
        DEBUG_FRAMESEPARATOR("\n\n");
        
        
		// Set all voices to needsRender
		for (int i=0; i<kNumVoices; i++)
			m_voices[i]->SetNeedsRender(true);

#ifdef DIGITS_MULTICORE
        // Tell helper to process this chunk
        // TODO: Support more than 2 cores
        if (m_helpers[0])
        {
            Tables::ZeroMem(m_helperOutBufs[0]->Data(), len);
            m_helpers[0]->StartProcessChunk(len, m_curBuf);
        }
#endif
	
        // For each voice, render into the left-channel buffer
        // This will be copied into the right either explicitly or
        // because it goes through the stereo delay (in the next stage)
        for (int i=0; i<kNumVoices; i++)
		{
			DEBUG_CORELOOP("Start chunk\n");

			wL = bookmarkL;
			
			if (m_voices[i]->IsFree() || m_voices[i]->IsSubVoice())
			{
				DEBUG_CORELOOP("Skip voice=%d: wL=%u wR=%u\n", i, wL-outs[0], wR-outs[1]);
				// Increment the pointers before looping. This was the cause of a
				// hard-to-track-down clicking bug (if wL/wR isn't incremented and
                // it's processing the last voice, the pointers will be incorrect).
				wL += len;
				continue;
			}
			
			voicesProcessed++;
			DEBUG_CORELOOP("Restored voice=%d: wL=%u wR=%u; Render to wR len=%d\n", i, wL-outs[0], wR-outs[1], len);
			m_voices[i]->Render(m_bufSets[0].m_voiceBuf->Data(), len, &m_bufSets[0], true, 0);
			
			// Copy sub-chunk for that voice in (slow?)
			for (int j=0; j<len; j++)
			{
				*wL++ += m_bufSets[0].m_voiceBuf->Data()[j] * gain;
			}
			DEBUG_CORELOOP("Finish chunk voice=%d: wL=%u wR=%u\n\n", i, wL-outs[0], wR-outs[1]);
		}

#ifdef DIGITS_MULTICORE
        // Merge in changes from helper if needed
        // TODO: Support more than 2 cores
        if (m_helpers[0])
        {
            DEBUG_MCORE("c: get lock\n");
            pthread_mutex_lock(m_helpers[0]->GetMutex());
            if (m_helpers[0]->DidProcessing())
            {
                DEBUG_MCORE("c: merging in changes\n");
                wL = bookmarkL;
                for (int j=0; j<len; j++)
                    *wL++ += m_helperOutBufs[0]->Data()[j] * gain; // copy in helper
            }
            DEBUG_MCORE("c: release lock\n");
            pthread_mutex_unlock(m_helpers[0]->GetMutex());
        }
#endif

        m_writeLeft -= len;
		framesLeft -= len;
        m_curBuf++;
	} while (framesLeft > 0);
	
	// Wrap nextSpan start for beginning of next buffer (frames = host's frame size)
	m_nextSpan -= frames;
	
	ApplyDC(outs[0], frames);

    // Run delay
	if (m_delWet > 0)
	{
        float delayIn;
        float inSamp;
        for (int i=0; i<frames; i++)
        {
            inSamp = outs[0][i];
            
            outs[1][i] = (m_delR[m_del_wR]*m_delWet) + inSamp;
            delayIn = (m_delR[m_del_wR] * m_del_filterA * m_delFB) + (m_del_lastR[0] * m_del_filterB);
            m_del_lastR[0] = delayIn;
            delayIn = (delayIn * m_del_filterA) + (m_del_lastR[1] * m_del_filterB);
            m_del_lastR[1] = delayIn;
            delayIn = (delayIn * m_del_filterA) + (m_del_lastR[2] * m_del_filterB);
            m_del_lastR[2] = delayIn;
            delayIn = (delayIn * m_del_filterA) + (m_del_lastR[3] * m_del_filterB);
            m_del_lastR[3] = delayIn;
            if (delayIn < .00001f && delayIn > -.00001f) delayIn = 0; // Gate FB to avoid denormal
            delayIn += inSamp;
            m_delR[m_del_wR] = delayIn;
            m_del_wR = (m_del_wR+1) % m_del_wrapR;

            outs[0][i] = (m_delL[m_del_wL]*m_delWet) + inSamp;
            delayIn = (m_delL[m_del_wL] * m_del_filterA * m_delFB) + (m_del_lastL[0] * m_del_filterB);
            m_del_lastL[0] = delayIn;
            delayIn = (delayIn * m_del_filterA) + (m_del_lastL[1] * m_del_filterB);
            m_del_lastL[1] = delayIn;
            delayIn = (delayIn * m_del_filterA) + (m_del_lastL[2] * m_del_filterB);
            m_del_lastL[2] = delayIn;
            delayIn = (delayIn * m_del_filterA) + (m_del_lastL[3] * m_del_filterB);
            m_del_lastL[3] = delayIn;
            if (delayIn < .00001f && delayIn > -.00001f) delayIn = 0; // Gate FB to avoid denormal
            delayIn += inSamp;
            m_delL[m_del_wL] = delayIn;
            m_del_wL = (m_del_wL+1) % m_del_wrapL;
        }
	}
	else
	{
		// Dupe left buffer for right (mono)
		for (int i=0; i<frames; i++)
			outs[1][i] = outs[0][i];
	}

    // Run Chorus
    
    if (m_chorusMode)
    {
        if (m_chorus[0].GetWet() != 0)
        {
            m_chorus[0].RenderWet(outs[0], m_chorusDryBuf->Data(), frames);
            for (int i=0; i < frames; i++)
                outs[0][i] += m_chorusDryBuf->Data()[i];
            m_chorus[1].RenderWet(outs[1], m_chorusDryBuf->Data(), frames);
            for (int i=0; i < frames; i++)
                outs[1][i] += m_chorusDryBuf->Data()[i];
        }
    }
    else
    {
        if (m_chorus[0].GetFeedback() != 0)
        {
            m_chorus[0].RenderWet(outs[1], m_chorusDryBuf->Data(), frames);
            for (int i=0; i < frames; i++)
                outs[0][i] = m_chorusDryBuf->Data()[i];
            m_chorus[1].RenderWet(outs[1], m_chorusDryBuf->Data(), frames);
            for (int i=0; i < frames; i++)
                outs[1][i] = m_chorusDryBuf->Data()[i];
        }
    }
    
 
	if (numProcessed != m_midiEvents.size())
	{
		printf("%lu events unprocessed (frame size=%d, m_writeLeft=%d):\n", m_midiEvents.size() - numProcessed, (int)m_manager.GetControlSize(), m_writeLeft);
		for (unsigned int i=0; i<m_midiEvents.size(); i++)
		{
			int deltaFrames = it->deltaFrames;
			printf("Frame=%d NextSpan=%d %02x %02x %02x\n", deltaFrames, m_nextSpan, it->midiData[0], it->midiData[1], it->midiData[2]);
		}
		printf("\n");
	}
	
	m_midiEvents.clear();
#ifdef DIGITS_BENCHMARK
	  //cpuBench.PrintLastMeasurement((m_manager.GetHostBlockSize()/m_manager.GetSamplingRate()) * 1000000);

		if (framesElapsed >= m_manager.GetSamplingRate())
		{
			framesElapsed = 0;
			cpuBench.Print((m_manager.GetHostBlockSize()/m_manager.GetSamplingRate()) * 1000000);
			cpuBench.Reset();
            int voicesInPlay = 0;
            for (int i=0; i<kNumVoices; i++)
                if (!m_voices[i]->IsFree()) voicesInPlay++;
            printf("Voices: %d\n", voicesInPlay);
            
		}
    
#endif
    
}

VstInt32 VstCore::processEvents(VstEvents *ev)
{
	for (VstInt32 i=0; i<ev->numEvents; i++)
	{
		if (ev->events[i]->type != kVstMidiType)
			continue;

		VstMidiEvent *evt = (VstMidiEvent*)ev->events[i];
		m_midiEvents.push_back(*evt);
	}
	
	return 1;
}

VstInt32 VstCore::canDo(char *text)
{
	if (strcmp(text, "receiveVstEvents") == 0)
		return 1;
	else if (strcmp(text, "receiveVstMidiEvent") == 0)
		return 1;
	
	return -1;
}

void VstCore::setParameter(VstInt32 index, float value) 
{
	if (index >= kNumParams)
    {
#if !defined(FAUX_VST_HEADER) && !defined(NO_EDITOR)
        if (editor && index != kDelLoss)
            ((AEffGUIEditor*)editor)->setParameter(index, value);
#endif
        return;
    }
	
    if (index == kResetOscs)
    {
#ifdef DIGITS_PRO
        m_resetOscs = value < .5? false : true;
#else
        m_resetOscs = false;
#endif
    }
	if (index == kOrch)
	{
        if ((m_orch > 0 && value == 0) || (m_orch == 0 && value > 0))
        {
            AllNotesOff();
            {
                // End all envelopes
                for (int i=0; i < kNumVoices; i++)
                    m_voices[i]->DisableSubVoices();
                
                // Set as non-subvoices
                for (int i=0; i < kNumVoices; i++)
                    m_voices[i]->SetIsSubVoice(false);
            }
        }

		m_orch = value;
	}
	else if (index == kDel_L)
	{
		m_del_wrapL = ((kDelayMinimum + (value * kDelayRange)) * m_manager.GetSamplingRate()) / 1000.0f;
		m_del_wL = 0;
	}
	else if (index == kDel_R)
	{
		m_del_wrapR = ((kDelayMinimum + (value * kDelayRange)) * m_manager.GetSamplingRate()) / 1000.0f;
		m_del_wR = 0;
	}
	else if (index == kDelWet)
	{
		m_delWet = value;
	}
	else if (index == kDelFB)
	{
		m_delFB = value;
	}
	else if (index == kDelLoss)
	{
		if (value == 0)
		{
			m_del_filterA = 1.0;
			m_del_filterB = 0;
		}
		else
		{
			// Around a 1200hz range for the filter
			value = 1.0f - value;
			value = 200.0f + (value * 10000.0f);
			value = pow(M_E, -2.0f * M_PI * (value/m_manager.GetSamplingRate()));
			m_del_filterB = value;
			m_del_filterA = 1 - value;
		}
	}
    else if (index == kChrDepth)
    {
        if (m_chorusMode)
        {
            m_chorus[0].SetRange(value * kChorusRange);
            m_chorus[1].SetRange(value * kChorusRange);
        }
        else
        {
            m_chorus[0].SetRange(value * kFlangerRange);
            m_chorus[1].SetRange(value * kFlangerRange);
        }
    }
    else if (index == kChrRate)
    {
        float rate = .1 + .4*value;
        m_chorus[0].SetLFORate(rate);
        m_chorus[1].SetLFORate(rate);
    }
    else if (index == kChrWetDry)
    {
        if (m_chorusMode)
        {
            m_chorus[0].SetWet(value*kChorusWetMax);
            m_chorus[1].SetWet(value*kChorusWetMax);
        }
        else
        {
            m_chorus[0].SetFeedback(value*kFlangerFeedbackMax);
            m_chorus[1].SetFeedback(value*kFlangerFeedbackMax);
        }
    }
    else if (index == kChrDirtyClean)
    {
        float n = value*(1/256.0);
        float b = 512 + 512*(1.0-value);
        m_chorus[0].SetNoiseAmount(n);
        m_chorus[1].SetNoiseAmount(n);
        m_chorus[0].SetBucketBins(b);
        m_chorus[1].SetBucketBins(b);
    }
    else if (index == kChrTone)
    {
        float prelp = 4000 + (4000 * value);
        float postlp = 2000 + (4000 * value);
        float hp = 20 + (1000 * value);
        
        m_chorus[0].SetTone(prelp, postlp, hp);
        m_chorus[1].SetTone(prelp, postlp, hp);
    }
    else if (index == kChrFlang)
    {
        SetChorusFlanger(value < .5f);
    }
    else if (index == VstCore::kMonoMode)
    {
        if (value < .5f) {
            m_isMono = false;
            if (value >= .25)
                m_alwaysGlide = true;
            else
                m_alwaysGlide = false;
        }
        else {
            if (value >= .75)
                m_alwaysGlide = true;
            else
                m_alwaysGlide = false;
            m_isMono = true;
        }
        
        // All notes off
        for (int i=0; i<kNumVoices; i++)
            m_voices[i]->NoteOff(-1);

        // Still set the individual voices up
		for (int i=0; i<kNumVoices; i++)
			m_voices[i]->SetParameter(index, value);
    }
    else if (index == kPitchbendUp)
    {
        m_bendUpAmt = 1 + (int)(value*24);
    }
    else if (index == kPitchbendDn)
    {
        m_bendDnAmt = 1 + (int)(value*24);
    }
	else if (index == kGain)
	{
		// TODO : dB?
		m_gain = value;
	}
	else
	{
		for (int i=0; i<kNumVoices; i++)
			m_voices[i]->SetParameter(index, value);
	}

	m_simpleProgram[index] = value;

#if !defined(FAUX_VST_HEADER) && !defined(NO_EDITOR)
	if (editor && index != kDelLoss)
		((AEffGUIEditor*)editor)->setParameter(index, value);
#endif
	
	return;
}

float VstCore::getParameter(VstInt32 index)
{
	if (index >= kNumParams)
		return 0;
    
	return m_simpleProgram[index];
}

void VstCore::getParameterLabel(VstInt32 index, char *text)
{
	vst_strncpy(text, "", kVstMaxParamStrLen);
}

void VstCore::getParameterDisplay(VstInt32 index, char *text)
{
	if (index >= kNumParams)
	{
		strncpy(text, "INVALID", kVstMaxParamStrLen);
		return;
	}
		
	paramNums_t param = (paramNums_t)index;
	float value = m_simpleProgram[index];
	
	
	//float2string(value*100.0f, text, kVstMaxParamStrLen);
	snprintf(text, kVstMaxParamStrLen, "%d%%", (int)(value*100.0f));
	
	switch (param)
	{
		case kO1Skew:
			break;
		case kO1ShpMod:
			if (value < .25)
				vst_strncpy(text, "Square", kVstMaxParamStrLen);
			else if (value < .5)
				vst_strncpy(text, "Saw", kVstMaxParamStrLen);
			else if (value < .75)
				vst_strncpy(text, "Angry", kVstMaxParamStrLen);
			else
				vst_strncpy(text, "Angrier", kVstMaxParamStrLen);
			break;
		case kO2Skew:
			break;
		case kO2ShpMod:
			if (value < .25)
				vst_strncpy(text, "Square", kVstMaxParamStrLen);
			else if (value < .5)
				vst_strncpy(text, "Saw", kVstMaxParamStrLen);
			else if (value < .75)
				vst_strncpy(text, "Angry", kVstMaxParamStrLen);
			else
				vst_strncpy(text, "Angrier", kVstMaxParamStrLen);
			break;			
		case kO2Vol:
            break;
        case kResetOscs:
            if (value < .5)
				vst_strncpy(text, "Free", kVstMaxParamStrLen);
            else
				vst_strncpy(text, "Reset", kVstMaxParamStrLen);
            break;
		case kResoVol:
			break;
		case kResoWave:
			if (value < .25)
				vst_strncpy(text, "Saw Qrtr", kVstMaxParamStrLen);
			else if (value < .5)
				vst_strncpy(text, "Saw Half", kVstMaxParamStrLen);
			else if (value < .75)
				vst_strncpy(text, "Sqr Qrtr", kVstMaxParamStrLen);
			else
				vst_strncpy(text, "Sqr Half", kVstMaxParamStrLen);
			break;		
		case kAmpAtt:
//			float2string(Voice::kAttackMinimumAmp + (value * Voice::kAttackRange), text, kVstMaxParamStrLen);
			break;
		case kAmpDec:
//			float2string(Voice::kDecayMinimum + (value * Voice::kAttackRange), text, kVstMaxParamStrLen);
			break;
		case kAmpFade:
			if (value == 0)
				vst_strncpy(text, "Off", kVstMaxParamStrLen);
			else if (value == 1.0f)
				vst_strncpy(text, "On", kVstMaxParamStrLen);
			else
				snprintf(text, kVstMaxParamStrLen, "%d%%Fade", (int)(value*100.0f));
			break;
		case kShpAtt:
//			float2string(Voice::kAttackMinimumShaper + (value * Voice::kAttackRange), text, kVstMaxParamStrLen);
			break;
		case kShpDec:
//			float2string(Voice::kDecayMinimum + (value * Voice::kAttackRange), text, kVstMaxParamStrLen);
			break;
		case kShpFade:
			if (value == 0)
				vst_strncpy(text, "Off", kVstMaxParamStrLen);
			else if (value == 1.0f)
				vst_strncpy(text, "On", kVstMaxParamStrLen);
			else
				snprintf(text, kVstMaxParamStrLen, "%d%%Fade", (int)(value*100.0f));

			break;
		case kShpMin:
		case kShpMax:
		case kFM:
			break;
		case kDetune:
			break;
		case kOctave:
			snprintf(text, kVstMaxParamStrLen, "%d", (int)(value*6) - 1);
			break;
		case kBasisWave:
			if (value < .5)
				vst_strncpy(text, "Cosine", kVstMaxParamStrLen);
			else
				vst_strncpy(text, "Sine", kVstMaxParamStrLen);
			break;
		case kLFOType:
#ifdef DIGITS_PRO
		case kLFO2Type:
		case kLFO3Type:
#endif
			if (value < .25)
				vst_strncpy(text, "Sine", kVstMaxParamStrLen);
			else if (value < .5)
				vst_strncpy(text, "Noise", kVstMaxParamStrLen);
			else if (value < .75)
				vst_strncpy(text, "RampUp", kVstMaxParamStrLen);
			else
				vst_strncpy(text, "RampDn", kVstMaxParamStrLen);
			break;
        case kLFORate:
        case kLFO2Rate:
        case kLFO3Rate:
        {
            // Sorry; there's cut n paste here from sheer laziness :)
            // Copied from Voice::ExpShape
            float base = 100.0f;
            float range = base - 1.0f;
            value = (pow(base, value) - 1.0f)/range;
            // 1000 is LFORange from Voice module
            snprintf(text, kVstMaxParamStrLen, "%dHz", (int)(value*1000.0f));
            break;
        }
        case kLFORepeat:
        case kLFO2Repeat:
        case kLFO3Repeat:
            if (value < 1.0f)
                snprintf(text, kVstMaxParamStrLen, "%dx", (int)(1+value*4.0f));
            else
                vst_strncpy(text, "On", kVstMaxParamStrLen);
            break;
        case kMonoMode:
			if (value < .25)
				vst_strncpy(text, "Poly", kVstMaxParamStrLen);
			else if (value < .5)
				vst_strncpy(text, "PolyGlide", kVstMaxParamStrLen);
			else if (value < .75)
				vst_strncpy(text, "MonoHeld", kVstMaxParamStrLen);
			else
				vst_strncpy(text, "MonoGlide", kVstMaxParamStrLen);
            break;
        case kPortaSpeed:
            char ms[80];
            snprintf(ms, 80, "%dms", int(value*500.0f));
            vst_strncpy(text, ms, kVstMaxParamStrLen);
            break;
        case kLockShaperForFilter:
            if (value == 0)
            {
                vst_strncpy(text, "FilterOff", kVstMaxParamStrLen);
            }
            break;
        case kFilterMode:
            if (value < .25)
				vst_strncpy(text, "4poleLP", kVstMaxParamStrLen);
			else if (value < .5)
				vst_strncpy(text, "3poleLP", kVstMaxParamStrLen);
			else if (value < .75)
				vst_strncpy(text, "2poleLP", kVstMaxParamStrLen);
			else
				vst_strncpy(text, "1poleLP", kVstMaxParamStrLen);
            break;
        case kPitchbendDn:
        case kPitchbendUp:
            snprintf(text, kVstMaxParamStrLen, "%d", 1 + (int)(value*24));
            break;
		case kPulseWidth:
		case kVelShaper:
		case kVelVolume:
		case kLFODelay:
		case kLFOShp:
		case kLFOAmp:
		case kLFOFrq:
		case kLFOPWM:
		case kOrch:
		case kDel_L:
		case kDel_R:
		case kDelWet:
		case kDelFB:
		case kDelLoss:
            break;
		case kNumParams:
			break;
	}
}

void VstCore::getParameterName(VstInt32 index, char *text)
{
	if (index >= kNumParams)
	{
		strncpy(text, "INVALID", kVstMaxParamStrLen);
		return;
	}

	vst_strncpy(text, s_paramNames[index], kVstMaxParamStrLen);
}

void VstCore::setSampleRate(float sR)
{
    // TODO: Support more than 2 cores
#ifdef DIGITS_MULTICORE
    bool wasHelper0;
    if (m_helpers[0])
    {
        wasHelper0 = true;
        m_helpers[0]->Quit();
        delete m_helpers[0];
        m_helpers[0] = 0;
    }
#endif
    
	m_manager.SetSamplingRate(sR);
	CalculateFreqDeltas();

	m_writeLeft = 0;
	
	// Recalculate delays
	// Converting to milliseconds is important!
	int size = ((kDelayMinimum + kDelayRange + 1) * (m_manager.GetSamplingRate()+1))/1000;
	if (m_delL)
		delete[] m_delL;
	m_delL = new float[size];
	memset(m_delL, 0, sizeof(float)*size);	
	if (m_delR)
		delete[] m_delR;
	m_delR = new float[size];
	memset(m_delR, 0, sizeof(float)*size);
    
    // Set delay pointers to 0 to avoid a crash when downsizing
    m_del_wL = 0;
    m_del_wR = 0;
    
    // Let the filter on all the voices know the new sampling rate
    for (int i=0; i<kNumVoices; i++)
    {
        m_voices[i]->SetSamplingRate(sR);
        m_voices[i]->UpdateFilterSamplingRate(sR);
    }
    
    m_chorus[0].SetSamplingRate(sR);
    m_chorus[1].SetSamplingRate(sR);
    m_chorus[0].RecalcDelays();
    m_chorus[1].RecalcDelays();
    SetChorusFlanger(true);
    
    // Reload the patch
    for (int i=0; i<kNumParams; i++)
        setParameter(i, m_simpleProgram[i]);
    
    // TODO: Reinitialize helper here, or it could potentially slow down
    // on next processReplacing call
}

void VstCore::setBlockSize(VstInt32 blockSize)
{
#ifdef DIGITS_MULTICORE
    bool wasHelper0;
    if (m_helpers[0])
    {
        wasHelper0 = true;
        m_helpers[0]->Quit();
        delete m_helpers[0];
        m_helpers[0] = 0;
    }
#endif

	m_manager.SetHostBlockSize(blockSize);
	m_writeLeft = 0;
    
    // TODO: Reinitialize helper here, or it could potentially slow down
    // on next processReplacing call
}

bool VstCore::getOutputProperties(VstInt32 index, VstPinProperties* prop)
{
	if (index > 1)
		return false;
	
	vst_strncpy(prop->label, index?"EjDg 2":"EjDg 1", 63);
	prop->flags = kVstPinIsStereo;
	prop->flags = kVstPinIsActive;
	if (index == 0)
		prop->flags |= kVstPinIsStereo;
	return true;
}

bool VstCore::getEffectName(char *name)
{
	vst_strncpy(name, "Digits", kVstMaxEffectNameLen);
	return true;
}

bool VstCore::getVendorString(char *text)
{
	vst_strncpy(text, "Extent of the Jam", kVstMaxVendorStrLen);
	return true;
}

bool VstCore::getProductString(char *text)
{
	vst_strncpy(text, "Digits", kVstMaxProductStrLen);
	return true;
}

// Save/Load routines. TODO:
// - Make envelope speeds in terms of decay to .001 (try log base <POLE> of .001)
// - Set any boolean to 0 or 1.0, not in-between. Maybe setParameter is able to do that

VstInt32 VstCore::getChunk(void** data, bool isPreset)
{
	if (!isPreset)
	{
		fprintf(stderr, "Banks are unsupported by this plugin; getting a bank of 1 preset\n");
	}
	
	int len = 2 /* maj/min version bytes */;
	for (int i=0; i<kNumParams; i++)
		len += strlen(s_paramNamesChunk[i]) + 1 /* terminator */ + 4 /* float value */;
	if (m_patchData == 0)
    {
		m_patchData = new char[len];
        fprintf(stderr, "Allocated %d bytes for patch storage\n", len);
    }
	*data = m_patchData;
	
	char *p = (char*)*data;
	*p++ = VERSION_PAT_MAJ; // Maj version
	*p++ = VERSION_PAT_MIN; // Min version
	for (int i=0; i<kNumParams; i++)
	{
		strcpy(p, s_paramNamesChunk[i]);
		p += strlen(s_paramNamesChunk[i]) + 1;
		
		*(float*)p = m_simpleProgram[i]; 
		p += 4;
	}

	return len;
}

void write_int(VstInt32 val, FILE *fp)
{
	fputc((val>>24)&0xff, fp);
	fputc((val>>16)&0xff, fp);
	fputc((val>>8)&0xff, fp);
	fputc(val&0xff, fp);
}

bool VstCore::SaveFXP(const std::string& fn)
{
	char *chunkData;
	VstInt32 chunkLen = getChunk((void**)&chunkData, true);
	
	FILE *fp = fopen(fn.c_str(), "wb");
	
	if (!fp)
		return false;
	
	fwrite("CcnK", 4, 1, fp); // chunkMagic
	write_int(chunkLen + 60 - 8, fp); // size of rest of file
	fwrite("FPCh", 4, 1, fp); //fxMagic
	write_int(1, fp);  // format version
	fwrite("LGPd", 4, 1, fp); // vst id
	write_int(1, fp);  // fx version
	write_int(VstCore::kNumParams, fp); // 23 on existing patches (why?)
	char name[28];
	memset(name, 0, 28);
	fwrite(name, 28, 1, fp);  // name of patch
	write_int(chunkLen, fp);
	fwrite(chunkData, 1, chunkLen, fp);	// chunk data
	
	fclose(fp);
	return true;
}

bool VstCore::LoadFXP(const std::string& fn)
{
    static const int kGutsOffset = 60;
    int len = -1;
    FILE *fp = fopen(fn.c_str(), "rb");
    
    if (!fp)
        return false;
    
    fseek(fp, 0, SEEK_END);
    len = ftell(fp) - kGutsOffset;
    fseek(fp, kGutsOffset, SEEK_SET);
    
    char* fileBuf = new char[len];
    fread(fileBuf, 1, len, fp);
    setChunk(fileBuf, len, true);
    delete[] fileBuf;
    
    fclose(fp);
    return true;
}

VstInt32 VstCore::setChunk(void* data, VstInt32 byteSize, bool isPreset)
{
    // Changed in 1.1:
	bool fadeUnsupported = false;
    // Changed in 1.2:
    bool halfRangeShapers = false;
    bool lfoRepeatUnsupported = false;
    bool twoLFOTypes = false;
    bool trapezoidal = false;
    // In old patches (pre 1.4 Digits version), the FM amount was
    // coupled with the mix amount because the oscillator volumes
    // were adjusted before applying the FM.
    // Not only this, but this made the FM range huge. The new
    // FM range is half that (we assume FM mix is at .5).
    bool fmCoupledWithMix = false;
    
    // Save this for fmCoupledWithMix
    float mixAmount = .5f;
    float fmAmount = 0;

	if (!isPreset)
	{
		fprintf(stderr, "Banks are unsupported by this plugin; setting a bank of 1 preset\n");
	}
	
	ZeroPatch();

	char *p = (char*)data;
	int bytesRead = 0;
	
	// Get version #
	unsigned char maj = *p++;
	unsigned char min = *p++;
	bytesRead += 2;
	
	// First versions of Digits only had sustain on or off, no fading
	if (maj == 1 && min == 0)
		fadeUnsupported = true;
    // Second revision of the patch format did not let the shapers go all the way to sine,
    // among other things
    if (maj == 1 && min < 2)
    {
        halfRangeShapers = true;
        lfoRepeatUnsupported = true;
        twoLFOTypes = true;
        trapezoidal = true;
        fmCoupledWithMix = true;
    }
	if (maj != 1 || min > VERSION_PAT_MIN)
	{
		fprintf(stderr, "Unknown version %d.%d; bailing on patch load!\n", maj, min);
		return 0;
	}
	
	char name[64];
	while (bytesRead < byteSize)
	{
		// Get parameter name
		int i = 0;
		do 
		{
			if (i >= 64)
				break;
			name[i++] = *p;
			bytesRead++;
		} while (*p++ != '\0' && bytesRead < byteSize);

		// If it's a null-length string, skip it. This is to allow
		// us to pad the end of the patch with 0s (for built-ins)
		if (name[0] == '\0')
			continue;
		
		// Get 32-bit floating point value
//		float val = *(float*)p; <- do not do it this way or it'll crash on ARM
        // Instead, do it like this:
		float val;
        memcpy(&val, p, 4);
		p+=4;
		bytesRead+=4;
		
		// Find matching parameter name
		for (i=0; i<kNumParams; i++)
		{
			if (strcmp(s_paramNamesChunk[i], name) == 0)
				break;
		}
		if (i == kNumParams)
		{
			fprintf(stderr, "Unable to match parameter '%s' at offset %d; skipping\n", name, bytesRead);
			continue;
		}
		else if (fadeUnsupported && ((strcmp(name, "AmpSus") == 0 || strcmp(name, "ShpSus") == 0)))
		{
			if (val > 0)
				val = 1.0f;
		}
        else if (halfRangeShapers && ((strcmp(name, "ShpMin") == 0) || strcmp(name, "ShpMax") == 0))
        {
            val = (val * .5f) + .5f;
        }
        else if (twoLFOTypes && strcmp(name, "LFOType") == 0)
        {
            val = val * .5f;
            if (val == .5f)
                val -= .01; // .5 and up is a newer wave
        }
        else if (strcmp(name, "Mix") == 0)
        {
            mixAmount = val;
        }
        else if (strcmp(name, "FM") == 0)
        {
            fmAmount = val;
        }
        
        // Adapt the old trapezoidal envelopes to the new parameters:
        // Sustain level is always 1.0. The sus knob used to be what
        // fade is now.
        if (trapezoidal && strcmp(name, "AmpSus") == 0)
        {
            setParameter(VstCore::kAmpSus, 1.0f);
            setParameter(VstCore::kAmpDec, 0);
            setParameter(VstCore::kAmpFade, val);
        }
        else if (trapezoidal && strcmp(name, "ShpSus") == 0)
        {
            setParameter(VstCore::kShpSus, 1.0f);
            setParameter(VstCore::kShpDec, 0);
            setParameter(VstCore::kShpFade, val);
        }
        else
        {
            // Set parameter accordingly
            setParameter(i, val);
        }
	}
    
    if (lfoRepeatUnsupported)
        setParameter(kLFORepeat, 1.0f); // infinite repeat
    
    if (fmCoupledWithMix)
        setParameter(kFM, (1.0f-mixAmount)*fmAmount*2.0f);
	
	for (int i=0; i<kNumVoices; i++)
		m_voices[i]->ResetPhases();

	return 1;
}

void VstCore::ResetPhases()
{
    for (int i=0; i<kNumVoices; i++)
        m_voices[i]->ResetPhases();
}

void VstCore::AllNotesOff()
{
    for (int i=0; i<127; i++)
    {
        if (IsNoteOn(i))
            NoteOff(i);
    }
}

void VstCore::ResetEngine()
{
    AllNotesOff();
    ResetPhases();
}
