/*
 *  PatchBankListMac.h
 *  PDVST
 *
 *  Created by Louis Gorenfeld on 8/18/12.
 *  Copyright 2012 LouisGorenfeld. All rights reserved.
 *
 */

#ifndef _PATCHBANKLISTMAC_H_
#define _PATCHBANKLISTMAC_H_

#include "PatchBankList.h" // interface

class PatchBankList : public PatchBankListBase
{
public:
	PatchBankList();
	~PatchBankList();

	virtual PatchBankListBase::fileList_t GetBankList();
	virtual PatchBankListBase::fileList_t GetPatchList();

private:
	static const char* s_banksPath;
	PatchBankListBase::fileList_t GetFileList(std::string dir, bool isBankList=false);
};

#endif