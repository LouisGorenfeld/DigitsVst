/*
 *  PatchBankList.h
 *  PDVST
 *
 *  Created by Louis Gorenfeld on 8/18/12.
 *  Copyright 2012 Extent of the Jam. All rights reserved.
 *
 */

// This is a generic interface for the patch bank list
// Use a subclass to use this!

// The class' job is to return a list of filenames. Upon
// an error, it should return a blank list.

// For legacy reasons, this module is called PatchBankList.
// But, the implementation-specific PatchBankList files
// (PatchBankListMac, PatchBankListWin) actually are the
// ones that supply the PatchBankList class. These only
// supply PatchBankListBase.

#ifndef _PATCHBANKLIST_H_
#define _PATCHBANKLIST_H_

#include <vector>
#include <string>

class PatchBankListBase
{
public:
    typedef std::vector<std::string> fileList_t;

	PatchBankListBase() : m_curPatch(0), m_curBank(0), m_numPatches(0) {}
	~PatchBankListBase() {}

	void SetBankDir(std::string dir)
	{
		m_curBankDir = dir;
	}
    
    void SetBankDir(int index)
    {
        m_curBankDir = m_bankListCache[index];
    }

	virtual PatchBankListBase::fileList_t GetBankList() = 0;
	virtual PatchBankListBase::fileList_t GetPatchList() = 0;
    PatchBankListBase::fileList_t m_bankListCache;
    PatchBankListBase::fileList_t m_patchListCache;
    std::string GetCurBankDir() { return m_bankPrefix + "/" + m_curBankDir + "/"; }
	std::string GetUserBankDir() { return m_bankPrefix + "/" + "User"; }
    int GetCurBank() { return m_curBank; }
    std::string GetCurPatchName()
	{
		if (m_curPatch == -1 || m_patchListCache.size() == 0) 
			return "";  
		else
			return m_patchListCache[m_curPatch];
	}
    int GetCurPatch() { return m_curPatch; }
    int SetCurBankDir(std::string newDir)
    {
        m_curBankDir = newDir;
        
        unsigned int index = 0;
        for (PatchBankListBase::fileList_t::iterator it = m_bankListCache.begin();
             it != m_bankListCache.end();
             it++, index++)
        {
            if (*it == m_curBankDir)
                break;
        }
        
        if (index < m_bankListCache.size())
            m_curBank = index;
        else
            m_curBank = -1;
		
		return m_curBank;
    }
    void SetCurBankDir(int index) { m_curBankDir = m_bankListCache[index]; }
    void SetCurPatch(int index)
	{
		if (m_patchListCache.size() > 0)
		{
			m_curPatch = index;
			m_curPatchName = m_patchListCache[index]; 
		}
	}
    // When set like this, there's no knowledge of the patch index number. The EditorWindow will
    // try to correct it when it first opens. This function is only to be used during VstCore init.
	// Return now-current patch
    int SetCurPatch(std::string newPatch)
    {
        m_curPatchName = newPatch;

        unsigned int index = 0;
        for (PatchBankListBase::fileList_t::iterator it = m_patchListCache.begin();
             it != m_patchListCache.end();
             it++, index++)
        {
            if (*it == m_curPatchName)
                break;
        }

        if (index < m_patchListCache.size())
            m_curPatch = index;
        else
            m_curPatch = -1;
		
		return m_curPatch;
    }
    // Get the *cached* number of patches. A bank must be set and a patch
    // list obtained before this is valid.
    int GetNumPatches() { return m_numPatches; }
    
protected:
	// Path to the currently-selected bank directory
	std::string m_curBankDir;
    std::string m_curPatchName;
    // Path to where the banks can be found
    std::string m_bankPrefix;
    // Index of the current patch and bank
    int m_curPatch;
    int m_curBank;
    // Cache of the number of patches in the bank (to avoid
    // getting a file list each time)
    int m_numPatches;
};

#ifdef _MSC_VER
#include "PatchBankListWin.h"
#elif __APPLE__
#include "PatchBankListMac.h"
#else
#warning "Creating empty PatchBankList implementation for this platform!"

class PatchBankList : public PatchBankListBase
{
public:
	PatchBankList() {}
	~PatchBankList() {}
	virtual PatchBankListBase::fileList_t GetBankList()
	{
		return PatchBankListBase::fileList_t();
	}
	virtual PatchBankListBase::fileList_t GetPatchList()
	{
		return PatchBankListBase::fileList_t();
	}
};
#endif
#endif // header compile-once
