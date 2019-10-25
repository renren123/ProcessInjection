// dllmain.cpp : 定义 DLL 应用程序的入口点。

#include "stdafx.h"
#include "Desktop_Info.h"
#include <Windows.h>
#include <Winsock2.h>
#include <stdio.h>
#include <WS2tcpip.h>
#include <string>

DWORD WINAPI MyThreadProc1(LPVOID pParam);
DWORD WINAPI ServerThreadMethod(LPVOID pParam);
DWORD WINAPI ServerClientState(LPVOID pParam);
typedef struct MyData
{
	SOCKET sockConn;
}MyData;
HMODULE GetSelfModuleHandle()
{
	MEMORY_BASIC_INFORMATION mbi;
	return ((::VirtualQuery(GetSelfModuleHandle, &mbi, sizeof(mbi)) != 0) ? (HMODULE)mbi.AllocationBase : NULL);
}
std::string GetCurrentDic(std::string mypath)
{
	char test[] = { "\\" };
	int index = mypath.rfind(test, mypath.length());
	mypath = mypath.substr(0, index);
	return mypath;
}
std::string GetExeFilePath(std::string exeName)
{
	const int bufferSize = 1024;
	char buffer[bufferSize] = { 0 };
	GetModuleFileName(GetSelfModuleHandle(), buffer, bufferSize);
	std::string szDllTemp = GetCurrentDic(buffer) + "\\" + exeName;
	return szDllTemp;
}
BOOL WakeupProc(const char* strFilepath)
{
	STARTUPINFO StartInfo;
	PROCESS_INFORMATION procStruct;
	memset(&StartInfo, 0, sizeof(STARTUPINFO));
	StartInfo.cb = sizeof(STARTUPINFO);
	BOOL bSuc = ::CreateProcess(strFilepath, NULL, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &StartInfo, &procStruct);
	if (!bSuc) {
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	}
	return bSuc;
}
LPCWSTR stringToLPCWSTR(std::string orig)
{
	size_t origsize = orig.length() + 1;
	const size_t newsize = 100;
	size_t convertedChars = 0;
	wchar_t *wcstring = (wchar_t *)malloc(sizeof(wchar_t)*(orig.length() - 1));
	mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);

	return wcstring;
}
int CharStrToInt(char* str)
{
	if (str == NULL || str[0]=='\0')
		return 0;
	int number = 0;
	int length = strlen(str);
	for (int i = 0; i < length; i++)
	{
		number *= 10;
		number += str[i] - '0';
	}
	return number;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		std::cout << "DLL已进入目标进程。" << std::endl;
		DWORD dwThreadId;
		DWORD serverThreadId;
		HANDLE myThread1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MyThreadProc1, NULL, 0, &dwThreadId);
		HANDLE myThread2 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServerThreadMethod, NULL, 0, &serverThreadId);
		break;
	}
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
	{
		std::cout <<"DLL已从目标进程卸载。"<< std::endl;
		//MessageBox(NULL, L"DLL已从目标进程卸载。", L"信息", MB_ICONINFORMATION);
		break;
	}
	}
	return TRUE;
}
DWORD WINAPI MyThreadProc1(LPVOID pParam)
{
	Desktop_Info desktop_Info;
	desktop_Info.Init();
	desktop_Info.Execute();
	while (1)
	{
		desktop_Info.DrowTime();
		int a = 11111;
		MessageBox(NULL, std::to_string(a).data(), "信息", MB_ICONINFORMATION);
		Sleep(5000);

	}
	std::cout << "DLL已进入线程1。" << std::endl;
	//MessageBox(NULL, L"DLL已进入线程1。", L"信息", MB_ICONINFORMATION);
	return 0;
}

DWORD WINAPI ServerThreadMethod(LPVOID pParam)
{
	std::cout << "DLL已进入线程2。" << std::endl;
	//第一步：加载socket库函数
	//**********************************************************
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(1, 1);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		return 0;
	}
	if (LOBYTE(wsaData.wVersion) != 1 ||
		HIBYTE(wsaData.wVersion) != 1) {
		WSACleanup();
		return 0;
	}
	//**********************************************************

	//第二步创建套接字
	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);
	//第三步：绑定套接字
	//获取地址结构
	SOCKADDR_IN addrSrv;
	//addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET, "127.0.0.1", (void*)&addrSrv.sin_addr.S_un.S_addr);
	//将IP地址指定为INADDR_ANY，允许套接字向任何分配给本地机器的IP地址发送或接收数据
	//htonl()将主机的无符号长整形数转换成网络字节顺序。

	addrSrv.sin_family = AF_INET;

	//sin_family 表示地址族，对于IP地址，sin_family成员将一直是AF_INET





	addrSrv.sin_port = htons(6000);

	//htons()将主机的无符号短整形数转换成网络字节顺序

	::bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));

	//监听客户端

	listen(sockSrv, 5);

	//定义从客户端接受的地址信息

	SOCKADDR_IN addrClient;

	int len = sizeof(SOCKADDR);

	while (1)
	{
		MyData socketThread;
		socketThread.sockConn = accept(sockSrv, (SOCKADDR*)&addrClient, &len);
		DWORD serverClientStateID;
		HANDLE serverClientStateThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServerClientState, &socketThread, 0, &serverClientStateID);
	}

	WSACleanup();
	return 0;
}

DWORD __stdcall ServerClientState(LPVOID lpParam)
{
	int mod = 100000;
	int indexNumber = 0;
	int readyState = 1;
	MyData * socketThread = (MyData *)lpParam;
	while (1)
	{
		char sendBuf[100] = { "\0" };
		sendBuf[0] = '1';
		//char sendBufTemp[100] = { '\0' };
		//sprintf(sendBuf, "welcome %s to wuhan", inet_ntop(AF_INET, (void*)&addrClient.sin_addr, sendBufTemp, 100));
		//printf("发送数据\n");
		send(socketThread->sockConn, sendBuf, strlen(sendBuf) + 1, 0);
		//等待客户端发送确认状态码：readyState
		while (1)
		{
			indexNumber %= mod;
			char recvBuf[100]= { '\0' };
			//printf("等待接受数据\n");
			int cnt = (int)recv(socketThread->sockConn, recvBuf, 100, 0);//cnt==0那表明连接已经断开
			//char buf[] = { 0 };
			//sprintf(buf, "cnt->%d", cnt);
			//MessageBox(NULL, stringToLPCWSTR(recvBuf),L"Server->recvBuf", MB_ICONINFORMATION);
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
					MessageBox(NULL, ("recvNumber:"+std::to_string(recvNumber)+"\tindexNumber:"+std::to_string(indexNumber)).data(), "信息", MB_ICONINFORMATION);
					MessageBox(NULL, "Server:Client通信错误！", "信息", MB_ICONINFORMATION);

					return 0;
				}
				//MessageBox(NULL, m_str, L"信息", MB_ICONINFORMATION);
				string sendMessage = std::to_string((recvNumber + 1)%mod);
				send(socketThread->sockConn, sendMessage.data(), sendMessage.length() + 1, 0);
			}
			else
			{
				if ((cnt < 0) && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) //这几种错误码，认为连接是正常的，继续接收
				{
					continue;//继续接收数据
				}
				std::string filePath = GetExeFilePath("ProcessInjection.exe");
				MessageBox(NULL, filePath.data(), "filePath:", MB_ICONINFORMATION);
				
				WakeupProc(filePath.data());
				return 0;
				break;//跳出接收循环

			}
			Sleep(100);
			indexNumber+=2;
		}

		
		
		//MessageBox(NULL, L"Server:Client通信结束！", L"信息", MB_ICONINFORMATION);
		closesocket(socketThread->sockConn);
		Sleep(1000);
	}
	
	return 0;
}

