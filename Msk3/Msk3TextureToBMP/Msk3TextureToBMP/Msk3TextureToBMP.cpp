// Msk3TextureToBMP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>>
#include <string>

#pragma pack(1)
typedef struct _MSK_TEXTURE
{
	DWORD Magic;
	WORD  Version;
	WORD  Offset;     //0
	WORD  Unknown[2];
	WORD  Width;      //need padding
	WORD  Height;
	DWORD CompLen;
}MSK_TEXTURE, *PMSK_TEXTURE;
#pragma pack()


int __fastcall Decompress(BYTE *Dest, int BufLen, BYTE *Src, int SrcLen, BYTE Shift)
{
	BYTE *v5; // r7@1
	BYTE *v6; // r6@1
	unsigned int v7; // r5@1
	int v8; // r1@2
	int v9; // t1@4
	char v10; // t1@7
	BYTE *v11; // r1@8
	unsigned int v12; // r2@9
	bool v13; // zf@9
	BYTE *v14; // r3@9
	int v15; // r12@9
	char v16; // t1@10

	v5 = Dest;
	v6 = &Src[SrcLen];
	v7 = 0;
	while (1)
	{
		while (1)
		{
			v8 = ((v7 >> 9) & 1) << 8;
			v7 >>= 1;
			if (!v8)
			{
				if (Src >= v6)
					return v5 - Dest;
				v9 = (unsigned __int8)*Src++;
				v7 = v9 | 0xFF00;
			}
			if (v7 & 1)
				break;
			if (Src >= v6)
				return v5 - Dest;
			v10 = *Src++;
			*v5++ = v10;
		}
		v11 = Src + 2;
		if (Src + 2 > v6)
			break;
		v12 = (unsigned __int8)Src[1] | ((unsigned __int8)*Src << 8);
		v13 = v12 >> Shift == -3;
		v14 = &v5[-(v12 & ((1 << Shift) - 1)) - 1];
		v15 = (v12 >> Shift) + 2;
		Src = v11;
		if (!v13)
		{
			do
			{
				v16 = *v14++;
				*v5++ = v16;
				v13 = v15-- == 0;
			} while (!v13);
			Src = v11;
		}
	}
	return v5 - Dest;
}

int wmain(int Argc, PWSTR* Argv)
{
	FILE*        Reader;
	FILE*        Writer;
	ULONG        Size, OutSize;
	PBYTE        Buffer;
	PBYTE        OutBuffer;
	PMSK_TEXTURE Header;
	DWORD        Offset;
	DWORD        RealWidth;

	if (Argc != 2)
		return 0;

	Reader = _wfopen(Argv[1], L"rb");
	fseek(Reader, 0, SEEK_END);
	Size = ftell(Reader);
	rewind(Reader);

	Buffer = new BYTE[Size];
	fread(Buffer, 1, Size, Reader);
	fclose(Reader);

	OutBuffer = new BYTE[0x2800000];
	Header = (PMSK_TEXTURE)Buffer;
	Offset = 0x20;
	OutSize = Decompress(OutBuffer, 0x2800000, Buffer + Offset, Header->CompLen, 12);

	RealWidth = (Header->Width + 15) & 0xFFFFFFF0;


	auto Save8BitBmp = [&](LPCWSTR FileName)
	{
		BITMAPFILEHEADER FileHeader;
		BITMAPINFOHEADER BmpHeader;
		int              i;
		BYTE             p[4], *pCur;

		Writer = _wfopen(FileName, L"wb");

		FileHeader.bfType = ((WORD)('M' << 8) | 'B');
		FileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 256 * 4L;//2个头结构后加调色板
		FileHeader.bfSize = FileHeader.bfOffBits + RealWidth * Header->Height;
		FileHeader.bfReserved1 = 0;
		FileHeader.bfReserved2 = 0;
		fwrite(&FileHeader, 1, sizeof(FileHeader), Writer);

		BmpHeader.biSize = 40;
		BmpHeader.biWidth = RealWidth; //
		BmpHeader.biHeight = Header->Height;
		BmpHeader.biPlanes = 1;
		BmpHeader.biBitCount = 8;
		BmpHeader.biCompression = 0;
		BmpHeader.biSizeImage = 0;
		BmpHeader.biXPelsPerMeter = 0;
		BmpHeader.biYPelsPerMeter = 0;
		BmpHeader.biClrUsed = 0;
		BmpHeader.biClrImportant = 0;
		fwrite(&BmpHeader, 1, sizeof(BmpHeader), Writer);
		
		for (i = 0, p[3] = 0; i<256; i++)
		{
			p[0] = p[1] = p[2] = 255 - i; // blue,green,red;
			fwrite(p, 1, 4, Writer);
		}

		fwrite(OutBuffer, 1, OutSize, Writer);
		fclose(Writer);
	};


	auto ReplaceFileExtensionName = [](LPCWSTR FileName, LPCWSTR ExtensionName)->std::wstring
	{
		std::wstring InFileName, OutFileName;
		size_t       Index;

		InFileName = FileName;
		Index = InFileName.find_last_of(L'.');
		if (Index == std::wstring::npos)
			return InFileName + ExtensionName;

		OutFileName = InFileName.substr(0, Index);
		OutFileName += ExtensionName;

		return OutFileName;
	};
	
	Save8BitBmp(ReplaceFileExtensionName(Argv[1], L".bmp").c_str());

	delete[]  OutBuffer;
	delete[]  Buffer;
    return 0;
}

