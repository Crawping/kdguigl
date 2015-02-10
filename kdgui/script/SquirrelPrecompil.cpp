/*
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2006 Jon Shier (jshier@iastate.edu)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reseved.
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 *  Copyright (C) 2009 Google Inc. All rights reseved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "windows.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "SquirrelPrecompil.h"

#define		KEY_FUNCTION		L"function"
#define		KEY_THIS_P			L"this."
#define		KEY_P_PROTOTYPE_P	L".prototype."
#define		KEY_NEW				L"new"

#define		MAX_VAR_NAME_LENGTH		256

int MyWcscpy(wchar_t * dest, WCHAR* destEnd, const wchar_t * src);
void WriteSpace(WCHAR* pCode, WCHAR* pEnd);

//////////////////////////////////////////////////////////////////////////
// static PBEcode SquirrelPrecompil(PWCHAR pIn, int nInLen, PWCHAR pOut, int nOutLen) {
//     PBEcode	 rt	   = EnumPBSuccess;
// 
//     if (nInLen > 7 && 0 == wcsncmp(pIn, L"//#skip", 7)) {
//         pOut[0] = 0;
//         goto Exit0;
//     }
// 
//     rt = StrReplaceCase_0(pIn, nInLen); // ȥ��ע��
//     if ( rt != EnumPBSuccess )
//         goto Exit0;
// 
//     OutputDebugStringW(pIn);
// 
//     rt = StrReplaceCase_1(pIn, nInLen, pOut, nOutLen); // ʶ��class
//     if ( rt != EnumPBSuccess )
//         goto Exit0;
// 
//     OutputDebugStringW(pOut);
// 
//     rt = StrReplaceCase_2(pOut, nOutLen); // ʶ��.prototype.
//     if ( rt != EnumPBSuccess )
//         goto Exit0;
// 
//     OutputDebugStringW(pOut);
// 
//     rt = StrReplaceCase_3(pOut, nOutLen); // ȥ��new 
//     if ( rt != EnumPBSuccess )
//         goto Exit0;
// 
//     OutputDebugStringW(pOut);
// 
// Exit0:
//     return rt;
// }
//////////////////////////////////////////////////////////////////////////

PBEcode StrReplaceCase_0(WCHAR* pCode, int nCodeLen) { // ɾ��ע��
    WCHAR* pPos = pCode;
    WCHAR* pTemPos = 0;
    WCHAR* pEnd = pCode + nCodeLen;
    WCHAR* pLineEnd = pCode;
    WCHAR* pTem = 0;

    if (nCodeLen < 2)
    {return EnumPBSuccess;}

    for (; pPos < pEnd; ++pPos) {
        pLineEnd = (WCHAR*)wcsnstr(pPos, L"\n", pEnd - pPos); // ����ɨ��
        if (0 == pLineEnd)
        {pLineEnd = pEnd;}

        for (pTemPos = pPos; pTemPos < pLineEnd - 1; ++pTemPos) {
            if (L'\\' == *(pTemPos) && L'\"' == *(pTemPos + 1)) {
                pTemPos++;
                pPos = pTemPos;
                continue;
            }
            if (L'\"' == *(pTemPos)) { // ����ҵ�˫���ţ����ƽ����¸�˫����
                bool bFind = false;
                for (pTem = pTemPos + 1; pTemPos < pLineEnd - 1; ++pTem) {
                    if (L'\\' == *(pTem) && L'\"' == *(pTem + 1)) { // ����ת��˫����
                        pTem++;
                        continue;
                    }
                    if (L'\"' == *pTem)
                    {bFind = true;break;}
                }
                if (false == bFind)
                {return EnumPBError;}
                pPos = pTem;
                pTemPos = pPos;
                continue;
            }
            if (L'/' == *pTemPos && L'*' == *(pTemPos + 1)) { // ����ҵ�����ע�ͣ����ƽ����¸�����ע��
                pTem = (WCHAR*)wcsnstr(pTemPos, L"*/", pEnd - pTemPos);
                if (0 == pTem)
                {return EnumPBError;}

                WriteSpace(pTemPos, pTem + 2);
                pPos = pTem + 2;
                pTemPos = pPos;
                continue;
            }
            if (L'/' == *pTemPos && L'/' == *(pTemPos + 1)) { // ����ҵ�����ע�ͣ����ƽ����¸�����
                WriteSpace(pTemPos, pLineEnd);
                pPos = pLineEnd;
                pTemPos = pPos;
                continue;
            }
        }
    }

    return EnumPBSuccess;
}

const WCHAR* wcsnstr(const WCHAR* src, const WCHAR* target, int n) {
    int j = 0;
	int i = 0;
    for (i = 0; i < n || src[i] != L'\0'; i++) {
		j = 0;
        if (target[0] == src[i]) {
            while (target[j] != L'\0' && src[i+j] != L'\0' && i + j < n) {
                j++;
                if (target[j] != src[i+j]) {
                    break;
                }
            }
        }

        if (target[j] == L'\0')
            {return &src[i];}

		//i += j;
    }
    return 0;
	
}

bool IsCchar(const WCHAR c) { // �Ƿ���c�ĺ�����������
	if ((c >= L'A' && c <= L'Z') ||
		(c >= L'a' && c <= L'z') ||
		(c >= L'0' && c <= L'9') ||
		(c == L'$') || (c == L'_'))
		{return true;}

	return false;
}

#define FUNC_NAME_LEN 8 // L"function"�ĳ���

const WCHAR* FindToken(const WCHAR* pCode, int nCodeLen, const WCHAR* pToken) {
	const WCHAR* pPos = pCode;
	const WCHAR* pEnd = pPos + nCodeLen;
	const WCHAR* pFuncEnd = 0; // �����Ľ�β
	int nToken = wcslen(pToken);

	for (; pPos < pEnd; ++pPos) {
		pPos = wcsnstr(pPos, pToken, pEnd - pPos);
		if (0 == pPos)
			{return 0;}
		if (pPos == pCode && !IsCchar(*(pPos + nToken)) ||
			pPos != pCode && !IsCchar(*(pPos - 1)) && !IsCchar(*(pPos + nToken)))
			{return pPos;}

		pPos += nToken;
    }

	return 0;
}

// ��߲���token���ұ߿����ǣ�Ҳ���Բ���
const WCHAR* FindRightIsNotToken(const WCHAR* pCode, int nCodeLen, const WCHAR* pToken) {
	const WCHAR* pPos = pCode;
	const WCHAR* pEnd = pPos + nCodeLen;
	const WCHAR* pFuncEnd = 0; // �����Ľ�β
	int nToken = wcslen(pToken);
	
	for (; pPos < pEnd; ++pPos) {
		pPos = wcsnstr(pPos, pToken, pEnd - pPos);
		if (0 == pPos)
		{return 0;}
		if (pPos == pCode ||
			pPos != pCode && !IsCchar(*(pPos - 1)))
		{return pPos;}
		
		pPos += nToken;
    }
	
	return 0;
}

const WCHAR* SkipSpace(const WCHAR* pCode, int nCodeLen) {
	const WCHAR* pPos = pCode;
	const WCHAR* pEnd = pPos + nCodeLen;
	for (; pPos < pEnd; ++pPos) {
		if (L' ' == *pPos || L'\r' == *pPos || L'\t' == *pPos || L'\n' == *pPos)
			{continue;}
		break;
	}
	return pPos;
}

void WriteSpace(WCHAR* pCode, WCHAR* pEnd) {
	WCHAR* pPos = pCode;
	for (; pPos < pEnd; ++pPos) {
		if (L'\n' == *pPos)
			{continue;}
		*pPos = L' ';
	}
}

const WCHAR* SkipPram(const WCHAR* pCode, int nCodeLen) { // ���������Ĳ���var xx, vaxxx...
	const WCHAR* pPos = pCode;
	const WCHAR* pEnd = pPos + nCodeLen;
	for (; pPos < pEnd; ++pPos) {
		if (L')' != *pPos)
			{continue;}
		break;
	}
	return pPos;
}

const WCHAR* SkipFuncBody(const WCHAR* pCode, int nCodeLen) { // ���������ı���{xxx = xx}...
	const WCHAR* pPos = pCode;
	const WCHAR* pEnd = pPos + nCodeLen;
	if (L'{' != pCode[0])
		{return 0;}

	int nEndFlag = 1; // �Ҵ����ŵ�����
	for (pPos++; pPos < pEnd; ++pPos) {
		if (L'}' == *pPos)
			{nEndFlag--;}

		if (0 == nEndFlag && L'}' == *pPos)
			{return pPos;}

		if (L'{' == *pPos)
			{nEndFlag++;}
	}

	return 0;
}

const WCHAR* SkipToken(const WCHAR* pCode, int nCodeLen) { // ����һ��token
	const WCHAR* pPos = pCode;
	const WCHAR* pEnd = pPos + nCodeLen;
	for (; pPos < pEnd; ++pPos) {
		if (IsCchar(*pPos))
			{continue;}
		break;
	}
	return pPos;
}

int MyWcsncpy(WCHAR *dest, WCHAR* destEnd, const WCHAR *src, const WCHAR * srcEnd) {
	WCHAR* pPos = dest;
	const WCHAR* pSrcPos = src;
	int i = 0;
	for (; pPos < destEnd && pSrcPos < srcEnd; ++pPos, ++pSrcPos, ++i) {
		*pPos = *pSrcPos;
	}
	return i;
}

int MyWcscpy(wchar_t * dest, WCHAR* destEnd, const wchar_t * src) {
	WCHAR* pPos = dest;
	const WCHAR* pSrcPos = src;
	int i = 0;
	for (; pPos < destEnd && 0 != *pSrcPos; ++pPos, ++pSrcPos, ++i) {
		*pPos = *pSrcPos;
	}
	return i;
}

#define ToKenLen 50 // �����������������ܳ���50���ַ�
#define MaxMemSize 60 // һ�������ֻ��60����Ա����

bool ScanFuncThisAndSave(const WCHAR* pCode, int nCodeLen, WCHAR pSave[MaxMemSize][ToKenLen], int* pnMemNameSize) {
    const WCHAR* pPos = pCode;
    //WCHAR pSave[30][ToKenLen] = {0};
    const WCHAR* pEnd = pPos + nCodeLen;
    //int nMemNameSize = 0;
    *pnMemNameSize = 0;

    for (; pPos < pEnd; ++pPos) {
        pPos = FindToken(pPos, pEnd - pPos, L"this");
        if (0 == pPos)
        {return true;}

        pPos += 4;
        if (L'.' != *pPos++)
        {continue;}

        const WCHAR* pMemNameEnd = SkipToken(pPos, pEnd - pPos);
        if (pMemNameEnd - pPos >= ToKenLen - 6 || *pnMemNameSize >= MaxMemSize) // L"=null;"
        {return false;}

        wcsncpy(pSave[*pnMemNameSize], pPos, pMemNameEnd - pPos);
        wcscat(pSave[*pnMemNameSize], L"=null;");
        (*pnMemNameSize)++;
    }

    return true;
}

wchar_t * __cdecl mywcsncpy (
	wchar_t * dest,
	const wchar_t * source,
	size_t count
	)
{	
	while (count && (*dest++ = *source++))    /* copy string */
		count--;
	
	if (count)                              /* pad out with zeroes */
		while (--count)
			*dest++ = L'\0';
		
	return(dest);
}

// KQuery.prototype.bind = function (props, dur) {} -> function KQuery::bind (props, dur) {}
PBEcode StrReplaceCase_2(WCHAR* pCode, int nCodeLen) {
	WCHAR* pPos = pCode;
	WCHAR* pEnd = pPos + nCodeLen;
	WCHAR* pTemPos = 0;
	
	for (; pPos < pEnd - 1; ++pPos) {

		WCHAR pClassName[ToKenLen] = {0};
		pPos = (WCHAR*)wcsnstr(pPos, L".prototype.", pEnd - pPos);
		if (0 == pPos)
			{return EnumPBSuccess;}

        OutputDebugStringW(pPos);

		*(pPos + 9) = L':';
		*(pPos + 10) = L':';
		
		for (pTemPos = pPos - 1; ; --pTemPos) {
			if (pTemPos < pCode)
				{return EnumPBError;}
			if (!IsCchar(*pTemPos))
				{break;}
		}
		pTemPos++;
		memmove(pTemPos + 9, pTemPos, (pPos - pTemPos)*sizeof(WCHAR));
		MyWcscpy(pTemPos, pEnd, L"function ");

		pPos = (WCHAR*)wcsnstr(pPos, L"=", pEnd - pPos);
		if (0 == pPos)
			{return EnumPBSuccess;}
		MyWcscpy(pPos, pEnd, L" ");

		pPos = (WCHAR*)FindToken(pPos, pEnd - pPos, L"function");
		if (0 == pPos)
			{return EnumPBError;}

		WriteSpace(pPos, pPos + 8);
	}

	return EnumPBSuccess;
}

// ��new���滻

PBEcode ChangeToken(WCHAR* pCode, int nCodeLen, const WCHAR* pToken, const WCHAR* pReplaceToken) {
	WCHAR* pPos = pCode;
	WCHAR* pEnd = pPos + nCodeLen;
	const WCHAR* pTemPos = 0;

	int nTokenLen = wcslen(pToken);
	if (nTokenLen != (int)wcslen(pReplaceToken))
		{return EnumPBError;}
	
	for (; pPos < pEnd - 1; ++pPos) {
		pPos = (WCHAR*)FindToken(pPos, pEnd - pPos, pToken);
		if (0 == pPos) 
			{return EnumPBSuccess;}

		wcsncpy(pPos, pReplaceToken, nTokenLen);
	}

	return EnumPBSuccess;
}

PBEcode StrReplaceCase_3(WCHAR* pCode, int nCodeLen) {
	ChangeToken(pCode, nCodeLen, L"new Array()", L"[         ]");
	ChangeToken(pCode, nCodeLen, L"new", L"   ");

	return EnumPBSuccess;
}

// �滻for in ����foreach
PBEcode StrReplaceCase_4(WCHAR* pCode, int nCodeLen) {
    WCHAR* pPos = pCode;
    WCHAR* pEnd = pPos + nCodeLen;
    WCHAR* pLineEnd = 0;
    WCHAR* pTemPos = 0;

    for (; pPos < pEnd; ++pPos) {
        pPos = (WCHAR*)FindToken(pPos, pEnd - pPos, L"    for");
        if (0 == pPos)
        {return EnumPBSuccess;}

        pLineEnd = (WCHAR*)wcsnstr(pPos, L"\n", nCodeLen);
        if (0 ==  pLineEnd)
        {pLineEnd = pEnd;}

        pTemPos = (WCHAR*)FindToken(pPos, pLineEnd - pPos, L"in");
        if (0 == pTemPos)
        {continue;}

        wcsncpy(pPos, L"foreach", 7);
    }

    return EnumPBSuccess;
}

// �滻����class
PBEcode StrReplaceCase_1(const WCHAR* pCode, int nCodeLen, PWCHAR pOut, int nOutLen) {
	const WCHAR* pPos = pCode;
	const WCHAR* pEnd = pPos + nCodeLen;
	const WCHAR* pFuncEnd = 0; // �����Ľ�β
	PWCHAR pOutPos = pOut;
	const WCHAR* pTemPos = 0;

	for (; pPos < pEnd; ++pPos) {
		pTemPos = FindToken(pPos, pEnd - pPos, L"function");
		if (0 == pTemPos) { // TODO_!!!
			MyWcsncpy(pOutPos, pOut + nOutLen, pPos, pEnd);
			return EnumPBSuccess;
		}

		pOutPos += MyWcsncpy(pOutPos, pOut + nOutLen, pPos, pTemPos);

		pPos = pTemPos + 8; // L"function"
		if (pPos == pEnd)
			{return EnumPBError;}

		pPos = SkipSpace(pPos, pEnd - pPos);
		if (pPos == pEnd) // ���ﺯ�������ƴ� 
			{return EnumPBError;}

		if (L'(' == *pPos) { // ���û�к��������������������
			pPos = SkipPram(pPos, pEnd - pPos);
			if (pPos == pEnd || L')' != *pPos++)
				{return EnumPBError;}

			pPos = SkipSpace(pPos, pEnd - pPos); // �����������������
			if (pPos == pEnd || L'{' != *pPos)
				{return EnumPBError;}

			pPos = SkipFuncBody(pPos, pEnd - pPos); // �����������Ҵ�����
			if (pPos == pEnd || L'}' != *pPos)
				{return EnumPBError;}
			// todo__
			pOutPos += MyWcsncpy(pOutPos, pOut + nOutLen, pTemPos, pPos + 1);
			continue;
		}

		const WCHAR* pFuncNameBegin = pPos;
		pPos = SkipToken(pPos, pEnd - pPos);
		const WCHAR* pFuncNameEnd = pPos;

		pPos = SkipSpace(pPos, pEnd - pPos); // ��������������������֮��Ŀո�
		if (pPos == pEnd)
			{return EnumPBError;}

		const WCHAR* pPramNameBegin = ++pPos;
		pPos = SkipPram(pPos, pEnd - pPos);
		if (pPos == pEnd || L')' != *pPos)
			{return EnumPBError;}
		const WCHAR* pPramNameEnd = pPos++;

		pPos = SkipSpace(pPos, pEnd - pPos); // �����������������
		if (pPos == pEnd || L'{' != *pPos)
			{return EnumPBError;}
		
		const WCHAR* pFuncBegin = pPos;

		const WCHAR* pTest = pPos;
		pPos = SkipFuncBody(pPos, pEnd - pPos); // �����������Ҵ�����
		if (pPos == pEnd || L'}' != *pPos)
			{return EnumPBError;}
		const WCHAR* pFuncEnd = pPos;

		pTemPos = FindRightIsNotToken(pFuncBegin, pFuncEnd - pFuncBegin, L"this.");
		if (0 == pTemPos) {// �������û��this ������ͨ����
			pOutPos += MyWcsncpy(pOutPos, pOut + nOutLen, pFuncNameBegin - 9, pFuncEnd + 1);
		} else {
			WCHAR pSave[MaxMemSize][ToKenLen] = {0};
			int nMemNameSize = 0;
			if (!ScanFuncThisAndSave(pFuncBegin, pFuncEnd - pFuncBegin, pSave, &nMemNameSize))
			{return EnumPBError;}
			
			WCHAR pNewCode[0x2001] = {0}; // �µĴ��룬���ȹ̶�
			PWCHAR pNewPos = pNewCode;
			PWCHAR pNewPosEnd = pNewCode + 0x2000;
			
			pNewPos += MyWcscpy(pNewPos, pNewPosEnd, L"class ");
			pNewPos += MyWcsncpy(pNewPos, pNewPosEnd, pFuncNameBegin, pFuncNameEnd);
			pNewPos += MyWcscpy(pNewPos, pNewPosEnd, L" { ");
			
			for (int i = 0; i < nMemNameSize; ++i) {
				int nMemNameLen = wcslen(pSave[i]);
				if (pNewPos + nMemNameLen >= pNewPosEnd)
				{return EnumPBError;}
				
				pNewPos += MyWcsncpy(pNewPos, pNewPosEnd, pSave[i], pSave[i] + nMemNameLen);
				//pNewPos += MyWcscpy(pNewPos, pNewPosEnd, L"; ");
			}
			
			pNewPos += MyWcscpy(pNewPos, pNewPosEnd, L" constructor(");
			pNewPos += MyWcsncpy(pNewPos, pNewPosEnd, pPramNameBegin, pPramNameEnd);
			pNewPos += MyWcscpy(pNewPos, pNewPosEnd, L") ");
			pNewPos += MyWcsncpy(pNewPos, pNewPosEnd, pFuncBegin, pFuncEnd);
			pNewPos += MyWcscpy(pNewPos, pNewPosEnd, L"}}");
            if (pNewPos == pNewPosEnd)
            {return EnumPBError;}
			
			pOutPos += MyWcscpy(pOutPos, pOut + nOutLen, pNewCode);
		}
    }

	return EnumPBSuccess;
}
