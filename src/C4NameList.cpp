/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2019, The LegacyClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

/* A static list of strings and integer values, i.e. for material amounts */

#include <C4NameList.h>

#include "StdAdaptors.h"
#include "StdCompiler.h"

C4NameList::C4NameList()
{
	Clear();
}

void C4NameList::Clear()
{
	std::memset(this, 0, sizeof(C4NameList));
}

void C4NameList::CompileFunc(StdCompiler *pComp, bool fValues)
{
	bool fCompiler = pComp->isCompiler();
	for (int32_t cnt = 0; cnt < C4MaxNameList; cnt++)
		if (fCompiler || Name[cnt][0])
		{
			if (cnt) pComp->Separator(StdCompiler::SEP_SEP2);
			// Name
			pComp->Value(mkDefaultAdapt(mkStringAdapt(Name[cnt], C4MaxName, StdCompiler::RCT_Idtf), ""));
			// Value
			if (fValues)
			{
				pComp->Separator(StdCompiler::SEP_SET);
				pComp->Value(mkDefaultAdapt(Count[cnt], 0));
			}
		}
}
