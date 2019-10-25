// ProcessInjection.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
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
DWORD ProcessNameToId(LPCTSTR lpszProcessName)
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
/*dllName是DLL的名字，szExeName是可执行程序的名字*/
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
	DWORD dwProcessId = ProcessNameToId(szExeName.data());
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
	/*
	OpenProcess：
	1、想拥有的该进程访问权限，PROCESS_ALL_ACCESS  //所有能获得的权限
	2、表示所得到的进程句柄是否可以被继承
	3、被打开进程的PID
	返回值为指定进程的句柄。
	*/
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
	/*
	VirtualAllocEx：
	1、HANDLE hProcess：需要在其中分配空间的进程的句柄.
	2、LPVOID lpAddress：想要获取的地址区域.这个参数为NULL，那么这个函数会自己决定如何分配..
	3、SIZE_T dwSize：要分配的内存大小.(单位byte字节)
	4、DWORD flAllocationType：内存分配的类型
	5、DWORD flProtect：内存页保护.
	VirtualAlloc在调用进程的地址空间内分配内存，而VirtualAllocEx则可以指定进程。
	*/
	void* pRemoteThreadMem = VirtualAllocEx(hTargetProcess,
		0,
		bufferSize,//这个地方到底分配多大内存还是个谜？？？？？？？？？？
		MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!pRemoteThreadMem)
	{
		std::cout << "GetLastError:" << GetLastError()<<"\tVirtualAllocEx" << std::endl;
		return 0;
	}
	// 改写目标进程的一段内存
	if (!WriteProcessMemory(hTargetProcess,
		pRemoteThreadMem,				//lpBaseAddress：要写入的起始地址
		(LPVOID)szDllTemp.data(),	//lpBuffer：写入的缓存区
		(SIZE_T)szDllTemp.length(),	//nSize：要写入缓存区的大小
		0)							//LPDWORD lpNumberOfBytesWritten ：这个是返回实际写入的字节。
		)
	{
		std::cout << "InjectProcess->WriteProcessMemory->GetLastError:" << GetLastError()<< std::endl;
		return 0;
	}

	//LPVOID pFunc = LoadLibraryA;
	//在宿主进程中创建线程
	HANDLE hRemoteThread = CreateRemoteThread(
		hTargetProcess,
		NULL,		//lpThreadAttributes：指向线程的安全描述结构体的指针，一般设置为NULL，表示使用默认的安全级别
		0,			//dwStackSize：线程堆栈大小，一般设置为0，表示使用默认的大小，一般为1M
		(LPTHREAD_START_ROUTINE)LoadLibraryA,	//lpStartAddress：线程函数的地址
		pRemoteThreadMem,	//lpParameter：线程参数
		0,			//dwCreationFlags：线程的创建方式
		&dwWriteBytes);		//lpThreadId：输出参数，记录创建的远程线程的ID

	if (!hRemoteThread)
	{
		std::cout << "InjectProcess->CreateRemoteThread->GetLastError:" << GetLastError()<< std::endl;
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
			int result = InjectProcess("TestDll.dll", "notepad.exe");
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
