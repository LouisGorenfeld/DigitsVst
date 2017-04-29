/* Phase Distortion Synthesizer (c) 2011 Louis Gorenfeld <louis.gorenfeld@gmail.com> */

#include "BufferManager.h"
#include "SndBuf.h"
#include "Tables.h"

SndBuf::SndBuf(BufferManager &manager, std::string name) :
	m_manager(manager),
	m_name(name)
{
	m_len = manager.GetHostBlockSize();
	m_buf = new sam_t[m_len];
	m_manager.AddBuffer(this);	
}
/*
SndBuf::SndBuf(BufferManager &manager, int len) :
	m_manager(manager),
	m_len(len)
{
	m_buf = new sam_t[len];
	memset(m_buf, 0, sizeof(sam_t)*len);
	m_manager.AddBuffer(this);	
}
*/
SndBuf::~SndBuf()
{
	m_manager.RemoveBuffer(this);	
	delete[] m_buf;
}

SndBuf::SndBuf(const SndBuf& b) :
	m_manager(b.m_manager)
{
	m_len = b.m_len;
	memcpy(m_buf, b.m_buf, sizeof(sam_t)*b.m_len);
}


void SndBuf::Resize(int newSize)
{
	if (m_buf)
		delete[] m_buf;
	m_buf = new sam_t[newSize];
	Tables::ZeroMem(m_buf, newSize);
	m_len = newSize;
}

float SndBuf::GetSamplingRate()
{ 
	return m_manager.GetSamplingRate();
}



