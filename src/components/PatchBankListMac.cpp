/*
 *  PatchBankListMac.cpp
 *  PDVST
 *
 *  Created by Louis Gorenfeld on 8/18/12.
 *  Copyright 2012 Extent of the Jam. All rights reserved.
 *
 */

#include <algorithm>	// sort
#include <dirent.h>
#include "PatchBankListMac.h"

PatchBankList::PatchBankList()
{
	m_bankPrefix = "/Applications/Digits/Presets";
}

PatchBankList::~PatchBankList()
{
}

PatchBankList::fileList_t PatchBankList::GetBankList()
{
	return GetFileList(m_bankPrefix, true);
}

PatchBankList::fileList_t PatchBankList::GetPatchList()
{
	PatchBankList::fileList_t files = GetFileList(m_bankPrefix + "/" + m_curBankDir);
    m_numPatches = files.size();
    return files;
}

PatchBankList::fileList_t PatchBankList::GetFileList(std::string dirStr, bool isBankList)
{
	PatchBankList::fileList_t* files;
	if (isBankList)
    {
        m_bankListCache.clear();
        files = &m_bankListCache;
    }
    else
    {
        m_patchListCache.clear();
        files = &m_patchListCache;
    }
    
	DIR* dir = opendir(dirStr.c_str());
	struct dirent* file;
	
	if (!dir)
    {
        files->push_back("<error>");
		return *files;
	}
    
	while ((file = readdir(dir)))
	{
		if ((!isBankList && file->d_type == DT_REG) || (isBankList && file->d_type == DT_DIR))
		{
            std::string stdname = file->d_name;
            if (stdname.at(0) == '.')
                continue;
            
            // strip '.fxp'
            if (!isBankList)
            {
                size_t pos = stdname.find(".fxp");
                if (pos != std::string::npos)
                    stdname = stdname.substr(0, pos);
            }
            
			files->push_back(stdname);
		}
	}
    
    if (files->size() == 0)
        files->push_back("<empty>");

	std::sort(files->begin(), files->end());

	closedir(dir);
	return *files;
}
