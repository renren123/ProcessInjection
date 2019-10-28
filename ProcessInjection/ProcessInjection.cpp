// ProcessInjection.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <Winsock2.h>//这个要放在<windows.h>前面才能结构体不报错

//#include <winsock.h>
#pragma comment(lib,"ws2_32.lib")  


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

//如果没有typedef就必须用 struct Student stu1; 来声明
/*用于传入线程参数，里面包括DLLName*/
typedef struct MyData {
	MyData(std::string dllName,std::string szExeName, PCSTR addr, u_short port)
	{
		this->dllName = dllName;
		this->szExeName = szExeName;
		this->addr = addr;
		this->port = port;
	}
	std::string dllName;//要注入的dll名字
	std::string szExeName;//要注入的进程名字
	PCSTR addr;
	u_short port;
} MyData;
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
	size_t index=mypath.rfind(test,mypath.length());
	mypath = mypath.substr(0, index);
	return mypath;
}
/*错误返回-1*/
int CharStrToInt(char* str)
{
	if (str == NULL || str[0] == '\0')
		return -1;
	if (str[0] - '0' < 0 || str[0] - '9'>0)
		return -1;
	int number = 0;
	size_t length = strlen(str);
	for (int i = 0; i <length; i++)
	{
		number *= 10;
		number += str[i] - '0';
	}
	return number;
}
SOCKET GetSOCKET(PCSTR addr,u_short port)
{
	//第一步：加载socket库函数
	//**********************************************************
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);//这是socket的版本
	/*
	WSADATA wsaData->返回值：
	WORD wVersion：Windows Sockets DLL期望调用者使用的Windows Sockets规范的版本。 高位字节存储副版本号, 低位字节存储主版本号，可以用WORD MAKEWORD(BYTE,BYTE ) 返回这个值,例如:MAKEWORD(1,1)
	WORD wHighVersion：这个DLL能够支持的Windows Sockets规范的最高版本。通常它与wVersion相同。
　　char szDescription[WSADESCRIPTION_LEN+1]：以null结尾的ASCII字符串，Windows Sockets DLL将对Windows Sockets实现的描述拷贝到这个字符串中，包括制造商标识。
  文本（最多可以有256个字符）可以包含任何字符，但是要注意不能包含控制字符和格式字符，应用程序对其最可能的使用方式是把它（可能被截断）显示在在状态信息中。
　　char szSystemStatus[WSASYSSTATUS_LEN+1]：以null结尾的ASCII字符串，Windows Sockets DLL把有关的状态或配置信息拷贝到该字符串中。
  Windows Sockets DLL应当仅在这些信息对用户或支持人员有用时才使用它们，它不应被作为szDescription域的扩展。
　　unsigned short iMaxSockets：单个进程能够打开的socket的最大数目。
　　unsigned short iMaxUdpDg：Windows Sockets应用程序能够发送或接收的最大的用户数据包协议（UDP）的数据包大小，以字节为单位。如果实现方式没有限制，那么iMaxUdpDg为零。
	
	当一个应用程序调用WSAStartup函数时，操作系统根据请求的Socket版本来搜索相应的Socket库，
	然后绑定找到的Socket库到该应用程序中。以后应用程序就可以调用所请求的Socket库中的其它Socket函数了。
	该函数执行成功后返回0。
	*/
	if( WSAStartup(wVersionRequested, &wsaData)!=0)
	{
		printf("ProcessInjection->GetSOCKET->WSAStartup(wVersionRequested, &wsaData)!=0");
		return 0;
	}
	//**********************************************************
	//第二步，创建套接字
	/*
	1、domain用于设置网络通信的域，函数socket()根据这个参数选择通信协议的族。
	2、函数socket()的参数type用于设置套接字通信的类型，主要有SOCKET_STREAM（流式套接字）、SOCK——DGRAM（数据包套接字）等。
	3、一般情况下IPPROTO_TCP、IPPROTO_UDP、IPPROTO_ICMP协议用的最多，UDP协议protocol就取IPPROTO_UDP，TCP协议protocol就取IPPROTO_TCP；一般情况下，我们让protocol等于0就可以，系统会给它默认的协议。但是要是使用raw socket协议，protocol就不能简单设为0，要与type参数匹配.
	*/
	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockClient == INVALID_SOCKET)
	{
		printf("ProcessInjection->GetSOCKET->sockClient!");
		return 0;
	}
	//定义套接字地址
	SOCKADDR_IN addrSrv;
	//vs2017在进行地址转换的时候要用这个函数，如果要设置INADDR_ANY，即允许任何地址加入，可以用下面那条语句。
	inet_pton(AF_INET, addr, (void*)&addrSrv.sin_addr.S_un.S_addr);//原函数
	//addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"/INADDR_ANY);  //获取服务器IP地址,inet_addr()将IP地址转为点分十进制的格式
	
	
	addrSrv.sin_family = AF_INET;
	//sin_family 表示地址族，对于IP地址，sin_family成员将一直是AF_INET
	addrSrv.sin_port = htons(port);
	//连接服务器
	if (connect(sockClient, (sockaddr *)&addrSrv, sizeof(addrSrv)) == SOCKET_ERROR)
	{  //连接失败 
		printf("ProcessInjection->GetSOCKET->connect");
		closesocket(sockClient);
		return 0;
	}
	
	return sockClient;
}
/*dllName是DLL的名字，szExeName是可执行程序的名字,错误返回-1*/
int InjectProcess(std::string dllName, std::string szExeName)
{
	/*int a1 = system("@echo off");
	int a2 = system("REG ADD \"HKCU\\SOFTWARE\\Microsoft\\Windows\\Shell\\Bags\\1\\Desktop\" /V FFLAGS /T REG_DWORD /D 1075839520 /F");
	int a3 = system("taskkill /f /im explorer.exe");
	int a4 = system("start explorer.exe");*/

	const int bufferSize = 1024;//用于存储一般缓存区的大小。
	char buffer[bufferSize] = { 0 };
	DWORD dwWriteBytes = 0;//在宿主进程中创建远程线程的ID

	GetModuleFileName(GetSelfModuleHandle(), buffer, bufferSize);
	std::string szDllTemp = GetCurrentDic(buffer) + "\\" + dllName;//DLL路径


	// 提升进程访问权限
	if (enableDebugPriv() == false)
	{
		std::cout << "提权失败！" << std::endl;
	}

	DWORD dwProcessId = ProcessNameToId(szExeName.data());
	if (dwProcessId == 0)
	{
		std::cout << "InjectProcess->ProcessNameToId->GetLastError:" << GetLastError() << std::endl;
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
		std::cout << "InjectProcess->OpenProcess->GetLastError:" << GetLastError() << std::endl;
		return -1;
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
		std::cout << "InjectProcess->VirtualAllocEx->GetLastError()" <<  std::endl;
		return -1;
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
		return -1;
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
		return -1;
	}
	return 1;
}
DWORD ClientServerStateThreadMethod(LPVOID pParam)
{
	const int mod = 100;
	int indexNumber = 1;
	const int readyState = 1;//这个开始通信时确认同步状态的。
	const int arrayLength = 100;
	MyData * myData = (MyData *)pParam;
	
	while (1)
	{
		/*
		Server：先发送一个请求状态码：1，表示服务器已经准备好发送数据，请求客户端的确认。
		Client：先发送一个请求状态码：1，表示客户端已经准备好接受服务器的消息
		*/
		SOCKET sockClient = GetSOCKET(myData->addr, myData->port);
		char recvBuf[arrayLength] = { '\0' };
		int cnt = (int)recv(sockClient, recvBuf, arrayLength, 0);//cnt==0那表明连接已经断开
		
		std::cout << "ProcessInjection->ClientServerStateThreadMethod->recvBuf->" << recvBuf << std::endl;
		int readyStateNumber = CharStrToInt(recvBuf);
		if (readyStateNumber != readyState)//第一次确认状态不正确，同步状态不正确
		{
			std::cout << "ProcessInjection->ClientThreadMethod->[recvNumber != readyState]" << std::endl;

			/*while (InjectProcess(myData->dllName, myData->szExeName) != 1)
			{
				std::cout << "ProcessInjection->ClientThreadMethod->InjectProcess" << std::endl;
			}*/
			//sockClient = GetSOCKET(myData->addr, myData->port);
			/*indexNumber = 1;
			readyState = 1;*/
			return 0;
		}
		std::string sendMessage = std::to_string(readyState);
		send(sockClient, sendMessage.data(), (int)sendMessage.length(), 0);
		while (1)
		{
			char recvBufState[arrayLength] = { '\0' };
			indexNumber %= mod;
			int cnt = (int)recv(sockClient, recvBufState, arrayLength, 0);//cnt==0那表明连接已经断开
			std::cout << "Client:recvBuf-> "<< recvBufState <<"\tindexNumber:"<< indexNumber << std::endl;
			

			if (cnt > 0)
			{
				//正常处理数据
				//CString  m_str(recvBufState);
				//printf("%s\n", recvBuf);
				int recvNumber = CharStrToInt(recvBufState);

				if (recvNumber != (indexNumber + 1)%mod)
				{
					std::cout << "Client:recvNumber != indexNumber + 1" << std::endl;
					std::cout << "Client:recvNumber->" << recvNumber << "\tindexNumber:" << indexNumber << std::endl;
					return 0;
				}
				std::string sendMessage = std::to_string((recvNumber + 1)%mod);
				send(sockClient, sendMessage.data(), (int)sendMessage.length(), 0);
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
		//关闭套接字
		closesocket(sockClient);
		//清除套接字资源
		WSACleanup();
		Sleep(1000);
	}
	return 0;
}
int main(int argc, char* argv[])
{
	/*std::cout << argv << std::endl;
	CString  m_str(argv[0]);
	MessageBox(NULL, m_str, "argv[0]", MB_ICONINFORMATION);*/

	//DLL必须和应用程序在同一文件夹
	std::string dllName = "TestDll.dll";
	std::string proName = "notepad.exe";
	const char * addr = "127.0.0.1";
	u_short port=6000;

	while (1)
	{
		while (InjectProcess(dllName, proName) != 1)
		{
			std::cout << "ProcessInjection->main->InjectProcess" << std::endl;
			Sleep(100);
		}
		MyData myData(dllName, proName, addr, port);
		DWORD clientThreadId;
		/*
		CreateThread:
		lpThreadAttrivutes：指向SECURITY_ATTRIBUTES的指针，用于定义新线程的安全属性，一般设置成NULL；
		dwStackSize：分配以字节数表示的线程堆栈的大小，默认值是0；
		lpStartAddress：指向一个线程函数地址。每个线程都有自己的线程函数，线程函数是线程具体的执行代码；
		lpParameter：传递给线程函数的参数；一般用结构体传参。
		dwCreationFlags：表示创建线程的运行状态，其中CREATE_SUSPEND表示挂起当前创建的线程，而0表示立即执行当前创建的进程；
		lpThreadID：返回新创建的线程的ID编号；
		*/
		HANDLE clientServerStateThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ClientServerStateThreadMethod, &myData, 0, &clientThreadId);

		//如果是Dll激活这个进程会把argv[0]置为1，而CharStrToInt错误会返回-1，即CharStrToInt没有
		//转换成功会返回-1
		if (CharStrToInt(argv[0]) == -1)
			MessageBox(NULL,
				"此文件的版本与正在运行的Windows版本不兼容。请检查计算机的系统信息以了解需要x86(32 位)还是 x64(64 位)版本的程序，然后联系软件发布者。",
				"错误",
				MB_ICONHAND | MB_OK
			);

		// 等待LoadLibraryA加载完毕
		while (DWORD resultForThread = WaitForSingleObject(clientServerStateThread, INFINITE) != WAIT_OBJECT_0)
		{
			std::cout << "ProcessInjection->main->resultForThread: " << resultForThread << std::endl;
		}

		/*
		VirtualFreeEx(hTargetProcess, pRemoteThread, sizeof(szDll), MEM_COMMIT);
		CloseHandle(hRemoteThread);
		CloseHandle(hTargetProcess);*/
	}
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
