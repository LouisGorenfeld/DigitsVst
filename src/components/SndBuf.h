/* Phase Distortion Synthesizer (c) 2011 Louis Gorenfeld <louis.gorenfeld@gmail.com> */

/* The SndBuf class is responsible for allocating and deallocating its
 * buffer, but the BufferManager class will automatically resize every
 * buffer when the block size changes in the host */

#ifndef _SNDBUF_H_
#define _SNDBUF_H_

#include <cstring>
#include <list>
#include <string>
#include "Tables.h"

class BufferManager;

class SndBuf
{
public:
	typedef float sam_t;
	
	SndBuf(BufferManager &manager, std::string name);
	//SndBuf(BufferManager &manager, int len);
	~SndBuf();
	SndBuf(const SndBuf& buf);

	int Len() { return m_len; }
	sam_t* Data() { return m_buf; }
	void Resize(int newSize);
	// Return sampling rate from manager
	float GetSamplingRate();
	// Zero out buffer
	inline void Zero()
	{
		Tables::ZeroMem(m_buf, m_len);
	}
	
private:
	BufferManager &m_manager;
	// Length of the buffer in frames
	int m_len;
	// Sample data for the buffer
	sam_t *m_buf;
	// Name of the buffer to make debugging less hellish
	std::string m_name;
};
#endif

