///*
//* HOOK住OpenProcess 防止进程被杀掉，这个项目是测试项目
//*/
//
//// Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
////
//// #pragma comment(linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"")//不显示窗口
//
//
///*********************************实现hook OpenProcess实现ring3保护进程、、C++完整代码、、************************************************/
#include "pch.h"
//#include <windows.h>
//#include <iostream>
//#pragma warning(disable: 4996)
//#pragma comment(lib,"user32.lib")  
//
//
//PIMAGE_IMPORT_BY_NAME  pImportByName = NULL;
//PIMAGE_THUNK_DATA    pOriginalThunk = NULL;
//PIMAGE_THUNK_DATA    pFirstThunk = NULL;
//
////IAT HOOK的核心函数、
//int IatHook(const char* DllName, const char* FunName, DWORD RealAddr);
//
//
////自己的OpenProcess函数、
//HANDLE WINAPI MyOpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);
//
//DWORD MyOpenProcessAddr = (DWORD)MyOpenProcess;
//
//
////真正的 OpenProcess函数指针、、
//typedef HANDLE(WINAPI* RealOpenProcess)(DWORD, BOOL, DWORD);
//RealOpenProcess pRealOpenProcess = (RealOpenProcess)OpenProcess;
//
//
////DLL MAIN 函数、
//BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
//{
//	std::cout << "DllMain enter" << std::endl;
//	if (fdwReason == DLL_PROCESS_ATTACH)
//	{
//		IatHook("Kernel32.dll", "OpenProcess", MyOpenProcessAddr);
//	}
//	return TRUE;
//}
//
///****************************************************** MyOpenProcess 函数的实现部分****************************************************/
//
//
//HANDLE WINAPI MyOpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId)
//{
//	std::cout << "MyOpenProcess enter" << std::endl;
//	return 0;
//
//
//	//获取  要保护进程的标题  的窗口句柄 系统API函数前使用::为了和类扩展函数区别
//	// 为了代码的健壮性 使用TEXT宏、  HWND 窗口句柄、
//	HWND HProtect = ::FindWindow(NULL, TEXT("Windows 当前所有进程"));
//	if (!HProtect)
//	{
//		return (pRealOpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId));
//	}                         //若不存在则调用返回
//								//获取创建此窗口的进程的ID、保存在  &ProtectId  地址中、
//	DWORD ProtectId;                //下边找出某个窗口的创建者(线程或进程)
//	GetWindowThreadProcessId(HProtect, &ProtectId);
//	if (ProtectId == dwProcessId)                      //dwProcessId是任务管理器要结束的进程ID
//	{
//		return 0;                              //如果结束的是我们的进程则 返回错误码0、
//	}
//	return (pRealOpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId));
//}
//
///******************************************************IAT  HOOK  函数的实现部分 * ***************************************************/
//
//int IatHook(const char* DllName, const char* FunName, DWORD RealAddr)
//{
//	/**************************   找相同的DLL   ****************************/
//	HANDLE pBegin = GetModuleHandle(NULL);
//	PBYTE  pBegin2 = (PBYTE)pBegin;
//	PIMAGE_DOS_HEADER DOS = PIMAGE_DOS_HEADER(pBegin2);
//	PIMAGE_NT_HEADERS NT = PIMAGE_NT_HEADERS(pBegin2 + DOS->e_lfanew);
//	PIMAGE_OPTIONAL_HEADER OPTION = &(NT->OptionalHeader);
//	PIMAGE_IMPORT_DESCRIPTOR IMPORT = PIMAGE_IMPORT_DESCRIPTOR(OPTION->DataDirectory[1].VirtualAddress + pBegin2);
//
//	while (IMPORT->Name)
//	{
//		char* OurDllName = (char*)(IMPORT->Name + pBegin2);
//		if (0 == strcmpi(DllName, OurDllName))
//		{
//			break;
//		}
//		IMPORT++;
//	}
//
//	/*************************   找相同的API函数    ****************************/
//	PIMAGE_IMPORT_BY_NAME  pImportByName = NULL;
//	PIMAGE_THUNK_DATA   pOriginalThunk = NULL;
//	PIMAGE_THUNK_DATA   pFirstThunk = NULL;
//	pOriginalThunk = (PIMAGE_THUNK_DATA)(IMPORT->OriginalFirstThunk + pBegin2);
//	pFirstThunk = (PIMAGE_THUNK_DATA)(IMPORT->FirstThunk + pBegin2);
//	while (pOriginalThunk->u1.Function) //记住是Function
//	{
//		DWORD u1 = pOriginalThunk->u1.Ordinal;  //记住是Ordinal
//		if ((u1 & IMAGE_ORDINAL_FLAG) != IMAGE_ORDINAL_FLAG) //说明MSB不是1  不是以序号导入
//		{
//			pImportByName = (PIMAGE_IMPORT_BY_NAME)((DWORD)pOriginalThunk->u1.AddressOfData + pBegin2);
//			char* OurFunName = (char*)(pImportByName->Name); //下边的计算也可以  
//			//char* OurFunName2 = (char*)((DWORD)pOriginalThunk->u1.AddressOfData + pBegin2 + 2);  
//			if (0 == strcmpi(FunName, OurFunName))
//			{
//				//获取以pFirstThunk开始的内存的信息并将其保存到MEMORY_BASIC_INFORMATION结构中
//				MEMORY_BASIC_INFORMATION   mbi_thunk;
//				VirtualQuery(pFirstThunk, &mbi_thunk, sizeof(MEMORY_BASIC_INFORMATION));
//				//VirtualProtect(mbi_thunk.BaseAddress,mbi_thunk.RegionSize, PAGE_READWRITE, &mbi_thunk.Protect);
//				//修改以pFirstThunk开始的内存的的保护属性为PAGE_READWRITE并将原保护属性保存到&dwOLD中
//				DWORD dwOLD;
//				VirtualProtect(pFirstThunk, sizeof(DWORD), PAGE_READWRITE, &dwOLD);
//				//更改真正OpenProcess的地址为自己写的MyOpenProcess函数的地址、、
//
//				//****************************更改的地方****************************
//				//pFirstThunk->u1.Function = (PDWORD)RealAddr;      //关键地方
//				pFirstThunk->u1.Function = (ULONGLONG)RealAddr;      //关键地方
//				//****************************更改的地方****************************
//
//
//				//****************************更改的地方****************************
//				//恢复之前更改的内存的保护属性为人家自己的、、           
//				//VirtualProtect(pFirstThunk, sizeof(DWORD), dwOLD, 0);
//				VirtualProtect(pFirstThunk, sizeof(DWORD), dwOLD, NULL);
//				//VirtualProtect(pFirstThunk, sizeof(DWORD), 0, &dwOLD);
//				//****************************更改的地方****************************
//				break;
//			}
//		}
//		pOriginalThunk++;
//		pFirstThunk++;
//	}
//	return 0;
//}
////int main()
////{
////	const char *a = "123";
////
////	int result = IatHook("Kernel32.dll", "OpenProcess", MyOpenProcessAddr);
////
////	std::cout <<"result: "<< result << std::endl;
////
////	while (1)
////	{
////		std::cout << "1" << std::endl;
////		Sleep(1000);
////	}
////
////	/*while (true)
////	{
////		Sleep(2000);
////		std::cout << "Test" << std::endl;
////	}
////    std::std::cout << "Hello World!\n"; */
////}
//
//// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
//// 调试程序: F5 或调试 >“开始调试”菜单
//
//// 入门提示: 
////   1. 使用解决方案资源管理器窗口添加/管理文件
////   2. 使用团队资源管理器窗口连接到源代码管理
////   3. 使用输出窗口查看生成输出和其他消息
////   4. 使用错误列表窗口查看错误
////   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
////   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
