#ifndef _PATCHBANKLISTWIN_H_
#define _PATCHBANKLISTWIN_H_

#include <string>
#include "PatchBankList.h"

class PatchBankList : public PatchBankListBase
{
public:
	PatchBankList();
	~PatchBankList();
	virtual PatchBankListBase::fileList_t GetBankList();
	virtual PatchBankListBase::fileList_t GetPatchList();
	PatchBankListBase::fileList_t GetFileList(std::string dirStr, bool isBankList);
};

#endif