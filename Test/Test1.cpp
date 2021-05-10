#include "pch.h"
#include "Test1.h"

#include <iostream>
#include <stdio.h>
#include <Windows.h>

typedef int(WINAPI* pfMessageBoxA)(HWND, LPCSTR, LPCSTR, UINT);
pfMessageBoxA OldMessageBoxA = NULL;

int WINAPI MyMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	return OldMessageBoxA(hWnd, "hello lyshark", lpCaption, uType);
}

PIMAGE_NT_HEADERS GetLocalNtHead()
{
	DWORD dwTemp = NULL;
	PIMAGE_DOS_HEADER pDosHead = NULL;
	PIMAGE_NT_HEADERS pNtHead = NULL;
	HMODULE ImageBase = GetModuleHandle(NULL);                              // ȡ����ImageBase
	//pDosHead = (PIMAGE_DOS_HEADER)(DWORD)ImageBase;                         // ȡpDosHead��ַ

	pDosHead = (PIMAGE_DOS_HEADER)ImageBase;                         // ȡpDosHead��ַ
	dwTemp = (DWORD)pDosHead + (DWORD)pDosHead->e_lfanew;
	//dwTemp = (DWORD)pDosHead + (DWORD)pDosHead->e_lfanew;

	pNtHead = (PIMAGE_NT_HEADERS)dwTemp;                                    // ȡ��NtHeadͷ��ַ
	return pNtHead;
}

void IATHook()
{
	PVOID pFuncAddress = NULL;
	pFuncAddress = GetProcAddress(GetModuleHandleA("user32.dll"), "MessageBoxA");  // ȡHook������ַ
	OldMessageBoxA = (pfMessageBoxA)pFuncAddress;                                  // ����ԭ����ָ��
	PIMAGE_NT_HEADERS pNtHead = GetLocalNtHead();                                  // ��ȡ����������NtHead
	PIMAGE_FILE_HEADER pFileHead = (PIMAGE_FILE_HEADER)&pNtHead->FileHeader;
	PIMAGE_OPTIONAL_HEADER pOpHead = (PIMAGE_OPTIONAL_HEADER)&pNtHead->OptionalHeader;

	DWORD dwInputTable = pOpHead->DataDirectory[1].VirtualAddress;    // �ҳ������ƫ��
	DWORD dwTemp = (DWORD)GetModuleHandle(NULL) + dwInputTable;
	PIMAGE_IMPORT_DESCRIPTOR   pImport = (PIMAGE_IMPORT_DESCRIPTOR)dwTemp;
	PIMAGE_IMPORT_DESCRIPTOR   pCurrent = pImport;
	DWORD* pFirstThunk; //������ӱ�,IAT�洢������ַ��.

	//���������
	while (pCurrent->Characteristics && pCurrent->FirstThunk != NULL)
	{
		dwTemp = pCurrent->FirstThunk + (DWORD)GetModuleHandle(NULL);// �ҵ��ڴ��еĵ����
		pFirstThunk = (DWORD*)dwTemp;                               // ��ֵ pFirstThunk
		while (*(DWORD*)pFirstThunk != NULL)                         // ��ΪNULl˵��û�н���
		{
			if (*(DWORD*)pFirstThunk == (DWORD)OldMessageBoxA)       // ���˵������������Ҫ�ĵ�ַ
			{
				DWORD oldProtected;
				VirtualProtect(pFirstThunk, 0x1000, PAGE_EXECUTE_READWRITE, &oldProtected);  // ����дȨ��
				dwTemp = (DWORD)MyMessageBoxA;
				memcpy(pFirstThunk, (DWORD*)&dwTemp, 4);                                    // ��MyMessageBox��ַ�����滻
				VirtualProtect(pFirstThunk, 0x1000, oldProtected, &oldProtected);            // �ر�д����
			}
			pFirstThunk++; // ��������ѭ��
		}
		pCurrent++;        // ÿ���Ǽ�1�������ṹ.
	}
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	IATHook();
	return TRUE;
}

int main()
{
	const char* a = "123";

	IATHook();

	while (1)
	{
		std::cout << "1" << std::endl;
		Sleep(1000);
	}

	/*while (true)
	{
		Sleep(2000);
		std::cout << "Test" << std::endl;
	}
	std::std::cout << "Hello World!\n"; */
}