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

/* A handy wrapper class to gzio files */

#pragma once

#include <StdFile.h>
#include <StdGzCompressedFile.h>

#include <memory>
#include <cstdio>

const int CStdFileBufSize = 4096;

class CStdStream
{
public:
	virtual bool Read(void *pBuffer, size_t iSize) = 0;
	virtual bool Advance(int iOffset) = 0;
	// Get size. compatible with c4group!
	virtual int AccessedEntrySize() = 0;
	virtual ~CStdStream() {}
};

class CStdFile : public CStdStream
{
public:
	CStdFile();
	~CStdFile();
	bool Status;
	char Name[_MAX_PATH + 1];

protected:
	FILE *hFile;
	std::shared_ptr<StdGzCompressedFile::Read> readCompressedFile;
	std::shared_ptr<StdGzCompressedFile::Write> writeCompressedFile;
	uint8_t Buffer[CStdFileBufSize];
	int BufferLoad, BufferPtr;
	bool ModeWrite;

public:
	bool Create(const char *szFileName, bool fCompressed = false, bool fExecutable = false);
	bool Open(const char *szFileName, bool fCompressed = false);
	bool Append(const char *szFilename); // append (uncompressed only)
	bool Close();
	bool Default();
	bool Read(void *pBuffer, size_t iSize) { return Read(pBuffer, iSize, 0); }
	bool Read(void *pBuffer, size_t iSize, size_t *ipFSize);
	bool Write(const void *pBuffer, int iSize);
	bool WriteString(const char *szStr);
	bool Rewind();
	bool Advance(int iOffset);
	// Single line commands
	bool Load(const char *szFileName, uint8_t **lpbpBuf,
		int *ipSize = nullptr, int iAppendZeros = 0,
		bool fCompressed = false);
	bool Save(const char *szFileName, const uint8_t *bpBuf,
		int iSize,
		bool fCompressed = false);
	// flush contents to disk
	inline bool Flush() { if (ModeWrite && BufferLoad) return SaveBuffer(); else return true; }
	int AccessedEntrySize();

protected:
	void ClearBuffer();
	int LoadBuffer();
	bool SaveBuffer();
};

int UncompressedFileSize(const char *szFileName);
