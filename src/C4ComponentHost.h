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

/* Holds a single text file component from a group */

#ifndef INC_C4ComponentHost
#define INC_C4ComponentHost

#include "C4GroupSet.h"

class C4ComponentHost
	{
	public:
		C4ComponentHost();
		virtual ~C4ComponentHost();
		const char *GetFilePath() const { return FilePath; }
		void Default();
		void Clear();
		void Open();
		const char *GetData() { return Data.getData(); }
		size_t GetDataSize() { return Data.getLength(); }
		virtual void Close();
		BOOL Load(const char *szName, C4Group &hGroup, const char *szFilename, const char *szLanguage=NULL);
		BOOL Load(const char *szName, C4GroupSet &hGroupSet, const char *szFilename, const char *szLanguage=NULL);
		BOOL LoadEx(const char *szName, C4Group &hGroup, const char *szFilename, const char *szLanguage=NULL);
		BOOL LoadAppend(const char *szName, C4Group &hGroup, const char *szFilename, const char *szLanguage=NULL);
		BOOL Save(C4Group &hGroup);
		bool GetLanguageString(const char *szLanguage, class StdStrBuf &rTarget);
		void TrimSpaces();
	protected:
		StdCopyStrBuf Data;
		BOOL Modified;
		char Name[_MAX_FNAME+1];
		char Filename[_MAX_FNAME+1];
		char FilePath[_MAX_PATH+1];
		void CopyFilePathFromGroup(const C4Group &hGroup);
#ifdef _WIN32
		HWND hDialog;
		void InitDialog(HWND hDlg);
	friend BOOL CALLBACK ComponentDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
#endif
	};

#endif
