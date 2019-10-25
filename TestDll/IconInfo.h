#pragma once
#include <afx.h>

using namespace std;
class IconInfo
{
public:
	IconInfo();
	~IconInfo();
	wchar_t* iconName;
	RECT  rc;
	int index = 0;
};

