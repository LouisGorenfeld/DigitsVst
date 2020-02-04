#include <algorithm>	// sort
#include <windows.h>
#include <ShlObj.h> // SHGetKnownFolderPath
#include <string>
#include "PatchBankListWin.h"

PatchBankList::PatchBankList()
{
	TCHAR path[MAX_PATH];
	if (SHGetFolderPath(0, CSIDL_COMMON_DOCUMENTS, 0, NULL, path) == S_OK)
	{
		m_bankPrefix = (char*)path;
		m_bankPrefix += "/Extent of the Jam/Digits/Patches";
	}
}

PatchBankList::~PatchBankList()
{
}

PatchBankList::fileList_t PatchBankList::GetBankList()
{
	return GetFileList(m_bankPrefix + "/*", true);
}
	
PatchBankList::fileList_t PatchBankList::GetPatchList()
{
	PatchBankList::fileList_t files = GetFileList(m_bankPrefix + "/" + m_curBankDir + "/*", false);
    m_numPatches = files.size();
    return files;
}

PatchBankList::fileList_t PatchBankList::GetFileList(std::string dirStr, bool isBankList)
{
	PatchBankListBase::fileList_t fileList;
	WIN32_FIND_DATA findData;
	HANDLE hFind;

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

	hFind = FindFirstFile(dirStr.c_str(), &findData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do 
		{
			if ((isBankList && findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				|| (!isBankList && !(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)))
			{

				std::string stdname = findData.cFileName;
				if (stdname == "." || stdname == "..")
					continue;
            
				// strip '.fxp'
				if (!isBankList)
				{
					size_t pos = stdname.find(".fxp");
					if (pos != std::string::npos)
						stdname = stdname.substr(0, pos);
				}

				files->push_back(stdname.c_str());
			}
		} while (FindNextFile(hFind, &findData));
	}

	std::sort(files->begin(), files->end());

	return *files;
}

