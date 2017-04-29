/* Phase Distortion Synthesizer (c) 2011 Louis Gorenfeld <louis.gorenfeld@gmail.com> */

#include <stdio.h>
#include "BufferManager.h"
#include <cassert>

void BufferManager::SetSamplingRate(float rate)
{
	m_samplingRate = rate;
    m_ctlSize = rate;
}

void BufferManager::SetHostBlockSize(float blkSize) 
{
	m_blkSize = blkSize; 

	if (m_bufList.empty())
		return;	
	
	// Resize all the buffers so that they match the
	// block size
	for (bufList_t::iterator it = m_bufList.begin();
		 it != m_bufList.end();
		 ++it)
	{
		if (*it)
    			(*it)->Resize(m_blkSize);
	}
	printf("Block size %f\n", blkSize);
}

