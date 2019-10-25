#include "stdafx.h"
//#include "TestDll.h"
#include <Windows.h>
#define DLL_TRAINING_API _declspec(dllexport)


extern "C" int  DLL_TRAINING_API export333()
{
	std::cout << "this is TestDLL" << std::endl;
	while (1)
	{
		MessageBox(NULL, "TestDLL->export333():DLL已进入目标进程。", "信息", MB_ICONINFORMATION);
		Sleep(2000);
	}
	
	return 123;
}
