/* Phase Distortion Synthesizer (c) 2011 Louis Gorenfeld <louis.gorenfeld@gmail.com> */

#ifndef _BUFFER_MANAGER_H_
#define _BUFFER_MANAGER_H_

#include <list>
#include "SndBuf.h"

/* The SndBuf class is responsible for allocating and deallocating its
 * buffer, but the BufferManager class will automatically resize every
 * buffer when the block size changes in the host */

class BufferManager
{
public:
	BufferManager() { /* Safe defaults for out-of-order init */ SetSamplingRate(96000); }
	~BufferManager() {}

	void AddBuffer(SndBuf *buf) { m_bufList.push_back(buf); }
	void RemoveBuffer(SndBuf *buf) { m_bufList.remove(buf); }
	
	float GetSamplingRate() { return m_samplingRate; }
	float GetControlSize() { return m_ctlSize; }
	static float GetControlRate() { return kCtlRate; }
	float GetHostBlockSize() { return m_blkSize; }
	
	void SetSamplingRate(float rate);	
	void SetHostBlockSize(float blkSize);
	
private:
	float m_samplingRate;
	float m_ctlSize;
	float m_blkSize;

	// Control rate in hz
	// TODO: This can be low, but we need to generate envelope signals
	static const float kCtlRate;

	// Buffers are managed by static routines so that they can
	// all be resized easily
	typedef std::list<SndBuf*> bufList_t;
	bufList_t m_bufList;	
};

#endif