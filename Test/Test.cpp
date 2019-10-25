// Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#pragma comment(linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"")//不显示窗口
#include "pch.h"
#include <Winsock2.h>//这个要放在<windows.h>前面才能结构体不报错
#include <windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <direct.h>
#include <string>
#include <atltime.h>
#include <WS2tcpip.h>
//extern CAPIHook g_OpenProcess;
// 自定义OpenProcess函数
#pragma data_seg("YCIShared")
HHOOK g_hHook = NULL;
DWORD dwCurrentProcessId = 0;
#pragma data_seg()
//HANDLE WINAPI Hook_OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId)
//{
//	typedef HANDLE(WINAPI *PFNTERMINATEPROCESS)(DWORD, BOOL, DWORD);
//
//	if (dwProcessId != dwCurrentProcessId)
//	{
//		return ((PFNTERMINATEPROCESS)(PROC)g_OpenProcess)(dwDesiredAccess, bInheritHandle, dwProcessId);
//	}
//	return 0;
//}
// 挂钩OpenProcess函数
//CAPIHook g_OpenProcess("kernel32.dll","OpenProcess",(PROC)Hook_OpenProcess);

///////////////////////////////////////////////////////////////////////////

static HMODULE ModuleFromAddress(PVOID pv)
{
	MEMORY_BASIC_INFORMATION mbi;
	if (::VirtualQuery(pv, &mbi, sizeof(mbi)) != 0)
	{
		return (HMODULE)mbi.AllocationBase;
	}
	else
	{
		return NULL;
	}
}
static LRESULT WINAPI GetMsgProc(int code, WPARAM wParam, LPARAM lParam)
{
	MessageBox(NULL,
		"GetMsgProc !",
		"Notice",
		MB_ICONINFORMATION | MB_OK
	);
	return ::CallNextHookEx(g_hHook, code, wParam, lParam);
}
BOOL WINAPI SetSysHook(BOOL bInstall, DWORD dwThreadId)
{
	BOOL bOk;
	dwCurrentProcessId = dwThreadId;
	if (bInstall)
	{
		/*g_hHook = ::SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc,
			NULL, dwThreadId);*/
		g_hHook = ::SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc,
			ModuleFromAddress(GetMsgProc), 0);
		bOk = (g_hHook != NULL);
	}
	else
	{
		bOk = ::UnhookWindowsHookEx(g_hHook);
		g_hHook = NULL;
	}
	return bOk;
}

void Method1()
{
	/*HWND hwnd = GetForegroundWindow();
	std::cout << "隐藏" << std::endl;
	system("pause");
	ShowWindow(hwnd, SW_HIDE);
	system("pause");

	system("@echo off");
	system("REG ADD \"HKCU\\SOFTWARE\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop\" /V FFLAGS /T REG_DWORD /D 1075839520 /F");
	system("taskkill /f /im explorer.exe");
	system("start explorer.exe");*/

}
DWORD processNameToId(LPCTSTR lpszProcessName)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe))
	{
		MessageBox(NULL, "The frist entry of the process list has not been copyied to the buffer", "Notice", MB_ICONINFORMATION | MB_OK);
		return 0;
	}
	while (Process32Next(hSnapshot, &pe))
	{
		if (!strcmp(lpszProcessName, pe.szExeFile))
		{
			return pe.th32ProcessID;
		}
	}
	return 0;
}
int Method2()
{
	char szExeName[] = { "notepad.exe" };
	DWORD dwProcessId = processNameToId(szExeName);
	if (dwProcessId == 0)
	{
		std::cout << "GetLastError:" << GetLastError() << std::endl;
		MessageBox(NULL,
			"The target process have not been found !",
			"Notice",
			MB_ICONINFORMATION | MB_OK
		);
		return -1;
	}
	SetSysHook(true, dwProcessId);
}

//BOOL WakeupProc(const WCHAR* strFilepath)
//{
//	STARTUPINFO StartInfo;
//	PROCESS_INFORMATION procStruct;
//	memset(&StartInfo, 0, sizeof(STARTUPINFO));
//	StartInfo.cb = sizeof(STARTUPINFO);
//	BOOL bSuc = ::CreateProcess(strFilepath, NULL, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &StartInfo, &procStruct);
//	if (!bSuc) {
//		LPVOID lpMsgBuf;
//		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
//	}
//	return bSuc;
//}


//原函数类型定义
typedef int (WINAPI* MsgBoxW)(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
MsgBoxW OldMsgBoxW = NULL;//指向原函数的指针
FARPROC pfOldMsgBoxW;  //指向函数的远指针
BYTE OldCode[5]; //原系统API入口代码
BYTE NewCode[5]; //原系统API新的入口代码 (jmp xxxxxxxx)

HANDLE hProcess = NULL;//本程序进程句柄
HINSTANCE hInst = NULL;//API所在的dll文件句柄

void HookOn();
void HookOff();
int WINAPI MyMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
	//TRACE(lpText);
	HookOff();//调用原函数之前，记得先恢复HOOK呀，不然是调用不到的
			  //如果不恢复HOOK，就调用原函数，会造成死循环
			  //毕竟调用的还是我们的函数，从而造成堆栈溢出，程序崩溃。

	int nRet = ::MessageBoxW(hWnd, L"哈哈，MessageBoxW被HOOK了", lpCaption, uType);

	HookOn();//调用完原函数后，记得继续开启HOOK，不然下次会HOOK不到。 

	return nRet;
}




//开启钩子的函数
void HookOn()
{
	std::cout<<"(hProcess != NULL): "<<(hProcess != NULL)<<std::endl;

	DWORD dwTemp = 0;
	DWORD dwOldProtect;

	//修改API函数入口前5个字节为jmp xxxxxx
	VirtualProtectEx(hProcess, pfOldMsgBoxW, 5, PAGE_READWRITE, &dwOldProtect);
	WriteProcessMemory(hProcess, pfOldMsgBoxW, NewCode, 5, 0);
	VirtualProtectEx(hProcess, pfOldMsgBoxW, 5, dwOldProtect, &dwTemp);

}

//关闭钩子的函数
void HookOff()
{
	std::cout << "(hProcess != NULL): " << (hProcess != NULL) << std::endl;

	DWORD dwTemp = 0;
	DWORD dwOldProtect;

	//恢复API函数入口前5个字节
	VirtualProtectEx(hProcess, pfOldMsgBoxW, 5, PAGE_READWRITE, &dwOldProtect);
	WriteProcessMemory(hProcess, pfOldMsgBoxW, OldCode, 5, 0);
	VirtualProtectEx(hProcess, pfOldMsgBoxW, 5, dwOldProtect, &dwTemp);
}

//获取API函数入口前5个字节
//旧入口前5个字节保存在前面定义的字节数组BYTE OldCode[5]
//新入口前5个字节保存在前面定义的字节数组BYTE NewCode[5]
void GetApiEntrance()
{

	//获取原API入口地址
	HMODULE hmod = ::LoadLibrary(_T("User32.dll"));
	OldMsgBoxW = (MsgBoxW)::GetProcAddress(hmod, "MessageBoxW");
	pfOldMsgBoxW = (FARPROC)OldMsgBoxW;

	if (pfOldMsgBoxW == NULL)
	{
		MessageBox(NULL, _T("获取原API入口地址出错"), _T("error!"), 0);
		return;
	}

	// 将原API的入口前5个字节代码保存到OldCode[]
	_asm
	{
		lea edi, OldCode		//获取OldCode数组的地址,放到edi
		mov esi, pfOldMsgBoxW //获取原API入口地址，放到esi
		cld	  //方向标志位，为以下两条指令做准备
		movsd //复制原API入口前4个字节到OldCode数组
		movsb //复制原API入口第5个字节到OldCode数组
	}


	NewCode[0] = 0xe9;//实际上0xe9就相当于jmp指令

	//获取MyMessageBoxW的相对地址,为Jmp做准备
	//int nAddr= UserFunAddr – SysFunAddr - （我们定制的这条指令的大小）;
	//Jmp nAddr;
	//（我们定制的这条指令的大小）, 这里是5，5个字节嘛
	BYTE NewCode[5];
	_asm
	{
		lea eax, MyMessageBoxW //获取我们的MyMessageBoxW函数地址
		mov ebx, pfOldMsgBoxW  //原系统API函数地址
		sub eax, ebx			 //int nAddr= UserFunAddr – SysFunAddr
		sub eax, 5			 //nAddr=nAddr-5
		mov dword ptr[NewCode + 1], eax //将算出的地址nAddr保存到NewCode后面4个字节
									  //注：一个函数地址占4个字节
	}


	//填充完毕，现在NewCode[]里的指令相当于Jmp MyMessageBoxW
	//既然已经获取到了Jmp MyMessageBoxW
	//现在该是将Jmp MyMessageBoxW写入原API入口前5个字节的时候了
	//知道为什么是5个字节吗？
	//Jmp指令相当于0xe9,占一个字节的内存空间
	//MyMessageBoxW是一个地址，其实是一个整数，占4个字节的内存空间
	//int n=0x123;   n占4个字节和MyMessageBoxW占4个字节是一样的
	//1+4=5，知道为什么是5个字节了吧
	HookOn();
}



//开始Hook MessageBoxW
void OnBnClickedBtnStartHook()
{
	DWORD dwPid = ::GetCurrentProcessId();
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, dwPid);

	GetApiEntrance();
	std::cout << "Hook已启动" << std::endl;
}

//调用MessageBoxW
void OnBnClickedBtnCallMsgBox()
{
	::MessageBoxW(NULL, L"这是正常的MessageBoxW", L"Hello", 0);
}

//停止Hook MessageBoxW
void OnBnClickedBtnStopHook()
{
	HookOff();
	//SetDlgItemText(IDC_STATIC_INFO, _T("Hook未启动"));
}
int main()
{
	/*typedef int(*_print)();
	typedef double(* test1)(double, double);
	HINSTANCE hDll = LoadLibrary(L"TestDll.dll");
	_print pAdd = (_print)GetProcAddress(hDll, "export333");
	int a = pAdd(); 
	std::cout << a << std::endl;
	FreeLibrary(hDll);*/

	//const char * szStr = "C:\\Users\\13643\\Desktop\\ProcessInjection\\x64\\Debug\\ProcessInjection.exe";
	//WCHAR wszClassName[256];
	//memset(wszClassName, 0, sizeof(wszClassName));
	//MultiByteToWideChar(CP_ACP, 0, szStr, strlen(szStr) + 1, wszClassName,
	//	sizeof(wszClassName) / sizeof(wszClassName[0]));
	////WakeupProc(wszClassName);
	//OnBnClickedBtnStartHook();

	const char *a = "123";

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

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
