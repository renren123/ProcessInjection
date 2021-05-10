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
	HMODULE ImageBase = GetModuleHandle(NULL);                              // 取自身ImageBase
	//pDosHead = (PIMAGE_DOS_HEADER)(DWORD)ImageBase;                         // 取pDosHead地址

	pDosHead = (PIMAGE_DOS_HEADER)ImageBase;                         // 取pDosHead地址
	dwTemp = (DWORD)pDosHead + (DWORD)pDosHead->e_lfanew;
	//dwTemp = (DWORD)pDosHead + (DWORD)pDosHead->e_lfanew;

	pNtHead = (PIMAGE_NT_HEADERS)dwTemp;                                    // 取出NtHead头地址
	return pNtHead;
}

void IATHook()
{
	PVOID pFuncAddress = NULL;
	pFuncAddress = GetProcAddress(GetModuleHandleA("user32.dll"), "MessageBoxA");  // 取Hook函数地址
	OldMessageBoxA = (pfMessageBoxA)pFuncAddress;                                  // 保存原函数指针
	PIMAGE_NT_HEADERS pNtHead = GetLocalNtHead();                                  // 获取到程序自身NtHead
	PIMAGE_FILE_HEADER pFileHead = (PIMAGE_FILE_HEADER)&pNtHead->FileHeader;
	PIMAGE_OPTIONAL_HEADER pOpHead = (PIMAGE_OPTIONAL_HEADER)&pNtHead->OptionalHeader;

	DWORD dwInputTable = pOpHead->DataDirectory[1].VirtualAddress;    // 找出导入表偏移
	DWORD dwTemp = (DWORD)GetModuleHandle(NULL) + dwInputTable;
	PIMAGE_IMPORT_DESCRIPTOR   pImport = (PIMAGE_IMPORT_DESCRIPTOR)dwTemp;
	PIMAGE_IMPORT_DESCRIPTOR   pCurrent = pImport;
	DWORD* pFirstThunk; //导入表子表,IAT存储函数地址表.

	//遍历导入表
	while (pCurrent->Characteristics && pCurrent->FirstThunk != NULL)
	{
		dwTemp = pCurrent->FirstThunk + (DWORD)GetModuleHandle(NULL);// 找到内存中的导入表
		pFirstThunk = (DWORD*)dwTemp;                               // 赋值 pFirstThunk
		while (*(DWORD*)pFirstThunk != NULL)                         // 不为NULl说明没有结束
		{
			if (*(DWORD*)pFirstThunk == (DWORD)OldMessageBoxA)       // 相等说明正是我们想要的地址
			{
				DWORD oldProtected;
				VirtualProtect(pFirstThunk, 0x1000, PAGE_EXECUTE_READWRITE, &oldProtected);  // 开启写权限
				dwTemp = (DWORD)MyMessageBoxA;
				memcpy(pFirstThunk, (DWORD*)&dwTemp, 4);                                    // 将MyMessageBox地址拷贝替换
				VirtualProtect(pFirstThunk, 0x1000, oldProtected, &oldProtected);            // 关闭写保护
			}
			pFirstThunk++; // 继续递增循环
		}
		pCurrent++;        // 每次是加1个导入表结构.
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