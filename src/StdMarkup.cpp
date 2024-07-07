/*
 * LegacyClonk
 *
 * Copyright (c) RedWolf Design
 * Copyright (c) 2001, Sven2
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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

// markup tags for fonts

#include <StdMarkup.h>
#include <StdColors.h>
#include <StdDDraw2.h>

void CMarkupTagItalic::Apply(CBltTransform &rBltTrf, bool fDoClr, uint32_t &dwClr)
{
	// do sheering
	rBltTrf.mat[1] -= 0.3f;
}

void CMarkupTagColor::Apply(CBltTransform &rBltTrf, bool fDoClr, uint32_t &dwClr)
{
	// set color
	if (fDoClr) dwClr = this->dwClr;
}

bool CMarkup::Read(const char **ppText, bool fSkip)
{
	char Tag[50]; CMarkupTag *pNewTag = nullptr;
	// get tag
	if (!SCopyEnclosed(*ppText, '<', '>', Tag, 49)) return false;
	const auto iTagLen = SLen(Tag);
	// split tag to name and pars
	char *szPars = nullptr; int iSPos;
	if ((iSPos = SCharPos(' ', Tag)) > -1)
	{
		Tag[iSPos] = 0;
		szPars = Tag + iSPos + 1;
	}
	// closing tag?
	if (Tag[0] == '/')
	{
		// no parameters
		if (szPars) return false;
		if (!fSkip)
		{
			// is this the tag to be closed?
			if (!pLast) return false;
			if (!SEqual(pLast->TagName(), Tag + 1)) return false;
			// close it
			delete Pop();
		}
	}
	// italic
	else if (SEqual(Tag, "i"))
	{
		// no parameters
		if (szPars) return false;
		// create italic tag
		if (!fSkip) pNewTag = new CMarkupTagItalic();
	}
	// colored
	else if (SEqual(Tag, "c"))
	{
		// no parameters?
		if (!szPars) return false;
		if (const auto iParLen = SLen(szPars); iParLen > 8)
		{
			return false;
		}
		else if (!fSkip)
		{
			// get color value by parameter
			uint32_t dwClr = 0;
			for (int i = 0; i < iParLen; ++i)
			{
				uint8_t b;
				if (szPars[i] >= '0' && szPars[i] <= '9') b = szPars[i] - '0';
				else if (szPars[i] >= 'a' && szPars[i] <= 'f') b = szPars[i] - 'a' + 10;
				else return false;
				dwClr |= (b << ((iParLen - i - 1) * 4));
			}
			// adjust alpha if not given
			if (iParLen <= 6) dwClr |= 0xff000000;
			dwClr = InvertRGBAAlpha(dwClr);
			// create color tag
			pNewTag = new CMarkupTagColor(dwClr);
		}
	}
	// unknown tag
	else return false;
	// add created tag
	if (pNewTag) Push(pNewTag);
	// advance past tag
	*ppText += iTagLen + 2;
	// success
	return true;
}

bool CMarkup::SkipTags(const char **ppText)
{
	// read tags as long as found
	while (**ppText == '<') if (!Read(ppText, true)) break;
	// return whether end is reached
	return !**ppText;
}

std::string CMarkup::ToMarkup()
{
	std::string result;
	for (CMarkupTag *pTag = pTags; pTag; pTag = pTag->pNext) result += pTag->ToMarkup();
	return result;
}

std::string CMarkup::ToCloseMarkup()
{
	std::string result;
	for (CMarkupTag *pTag = pLast; pTag; pTag = pTag->pPrev) result += std::format("</{}>", pTag->TagName());
	return result;
}

bool CMarkup::StripMarkup(char *szText)
{
	// skip any tags and inline-images
	CMarkup mkup(false);
	const char *szRead = szText, *szPos2;
	do
	{
		mkup.SkipTags(&szRead);
		if (szRead[0] == '{' && szRead[1] == '{' && szRead[2] != '{') // skip at {{{, because {{{id}} should be parsed as { {{id}} }.
		{
			if (szPos2 = SSearch(szRead + 2, "}}"))
				// valid {{blub}}-tag
				szRead = szPos2;
			else
				// invalid {{-tag
				szRead += 2;
		}
		else if (szRead[0] == '}' && szRead[1] == '}')
			// invalid }}-tag
			szRead += 2;
	} while (*szText++ = *szRead++);
	return szText != szRead;
}

bool CMarkup::StripMarkup(StdStrBuf *sText)
{
	// strip any markup codes from given text buffer
	char *buf = sText->GrabPointer();
	if (!buf) return false;
	bool fSuccess = StripMarkup(buf);
	sText->Take(buf);
	return fSuccess;
}
