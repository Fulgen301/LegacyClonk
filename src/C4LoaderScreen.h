/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2003, Sven2
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

// startup screen

#pragma once

#include <C4FacetEx.h>

class C4LoaderScreen
{
public:
	CStdFont &TitleFont; // font used for title output
	CStdFont &LogFont;   // font used for logging
	C4FacetExSurface fctBackground; // background image

public:
	C4LoaderScreen();
	~C4LoaderScreen() = default;

	bool Init(const char *szLoaderSpec); // inits and loads from global C4Game-class
	int SeekLoaderScreens(C4Group &rFromGrp, const char *szWildcard, int iLoaderCount, char *szDstName, C4Group **ppDestGrp);

	void Draw(C4Facet &cgo, int iProgress = 0, class C4LogBuffer *pLog = nullptr, int Process = 0); // draw loader screen (does not page flip!)
};
