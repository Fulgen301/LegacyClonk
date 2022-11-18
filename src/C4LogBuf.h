/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
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

// a buffer holding a log history

#pragma once

#include "C4ForwardDeclarations.h"

#include <cstddef>
#include <cstdint>

// circular log buffer to holding line-wise log data
class C4LogBuffer
{
private:
	struct LineData
	{
		CStdFont *pFont; // line font
		uint32_t dwClr; // line clr
		bool fNewParagraph; // if set, this line marks a new paragraph (and is not the wrapped line of a previous par)
	};

	char *szBuf; // string buffer
	LineData *pLineDataBuf; // line data buffer
	size_t iBufSize; // size of string buffer
	int iFirstLinePos, iAfterLastLinePos; // current string buffer positions
	int iLineDataPos, iNextLineDataPos; // current line data buffer positions
	size_t iMaxLineCount; // max number of valid lines - size of line data buffer
	size_t iLineCount; // number of valid lines in buffer
	int iLineBreakWidth; // line breaking width
	char *szIndent; // chars inserted as indent space
	bool fDynamicGrow; // if true, lines are always added to the buffer. If false, the buffer is used circular and old lines removed
	bool fMarkup; // if set, '|' is treated as linebreak

	void GrowLineCountBuffer(size_t iGrowBy);
	void GrowTextBuffer(size_t iGrowBy);
	void DiscardFirstLine(); // discard oldest line in buffer
	void AppendSingleLine(const char *szLine, size_t iLineLength, const char *szIndent, CStdFont *pFont, uint32_t dwClr, bool fNewParagraph); // append given string as single line

public:
	C4LogBuffer(size_t iSize, size_t iMaxLines, int iLBWidth, const char *szIndentChars = "    ", bool fDynamicGrow = false, bool fMarkup = true);
	~C4LogBuffer();

	void AppendLines(const char *szLine, CStdFont *pFont, uint32_t dwClr, CStdFont *pFirstLineFont = nullptr); // append message line to buffer; overwriting old lines if necessary
	const char *GetLine(int iLineIndex, CStdFont **ppFont, uint32_t *pdwClr, bool *pNewParagraph) const; // get indexed line - negative indices -n return last-n'th-line
	void Clear(); // clear all lines

	void SetLBWidth(int iToWidth);
};
