#include "stdafx.h"
#include "Desktop_Info.h"
#include <time.h>
#include <atltime.h>
#define random(a,b) ((rand() % (b-a))+ a)//生成随机数在a-b之间

struct Line {
	int x1;
	int y1;
	int x2;
	int y2;
};
struct Point
{
	int x;
	int y;
	Point(int x, int y)
	{
		this->x = x;
		this->y = y;
	}
};
typedef struct tagLVITEM64
{
	UINT mask;
	int iItem;
	int iSubItem;
	UINT state;
	UINT stateMask;
	_int64 pszText;
	int cchTextMax;
	int iImage;
	_int64 lParam;
#if (_WIN32_IE >= 0x0300)
	int iIndent;
#endif
#if (_WIN32_WINNT >= 0x0501)
	int iGroupId;
	UINT cColumns; // tile view columns
	_int64 puColumns;
#endif
#if _WIN32_WINNT >= 0x0600
	_int64 piColFmt;
	int iGroup; // readonly. only valid for owner data.
#endif
} LVITEM64;

Desktop_Info::Desktop_Info()
{
	accessX = new int[accessNumber];

}


Desktop_Info::~Desktop_Info()
{
}
HWND Desktop_Info::FindDTWindow()
{
	HWND hParent = NULL;
	HWND hDefView = NULL;
	HWND hDeskIcon = NULL;

	hDeskIcon = GetHwndDesktop();
	if (hDeskIcon)
	{
		return hDeskIcon;
	}

	//获得第一个WorkerW类的窗口
	hParent = FindWindowEx(0, 0, "WorkerW", "");

	//接着遍历
	while ((!hDefView) && hParent)
	{
		hDefView = FindWindowEx(hParent, 0, "SHELLDLL_DefView", 0);
		hParent = FindWindowEx(0, hParent, "WorkerW", "");
	}

	hDeskIcon = FindWindowEx(hDefView, 0, "SysListView32", "FolderView");
	return hDeskIcon;
}
LPCWSTR Desktop_Info::stringToLPCWSTR(std::string orig)
{
	size_t origsize = orig.length() + 1;
	//const size_t newsize = 100;
	size_t convertedChars = 0;
	wchar_t *wcstring = (wchar_t *)malloc(sizeof(wchar_t)*(orig.length() - 1));
	mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);

	return wcstring;
}
HWND Desktop_Info::GetHwndDesktop()
{
	HWND hWndDesktop = NULL;
	HWND hProgMan = ::FindWindow("Progman", NULL);
	//HWND hProgMan = FindDTWindow();
	if (hProgMan)
	{
		HWND hShellDefView = ::FindWindowEx(hProgMan, NULL, "SHELLDLL_DefView", NULL);
		if (hShellDefView)
			hWndDesktop = ::FindWindowEx(hShellDefView, NULL, "SysListView32", NULL);
		else
			return NULL;
	}
	return hWndDesktop;
}

void Desktop_Info::Move(HWND hDeskTop, int index, RECT rc)
{
	if (hDeskTop == NULL)
	{
		std::cout << "Desktop_Info->hDestTop==NULL" << std::endl;
		return;
	}
	if (index < 0)
	{
		std::cout << "Desktop_Info->index < 0" << std::endl;
		return;
	}
	::SendMessage(hDeskTop, LVM_SETITEMPOSITION, index, MAKELPARAM(rc.left, rc.top));
}

void Desktop_Info::Init()
{
	//RECT rect;
	//GetWindowRect(hDeskTop, &rect);
	//两个函数获取的是 显示屏幕的大小，但不包括任务栏等区域。
	windowWidth = GetSystemMetrics(SM_CXFULLSCREEN);
	windowHeight = GetSystemMetrics(SM_CYFULLSCREEN);
	accessX = new int[accessNumber];
	accessX[0] = windowWidth / 3;
	accessX[1] = windowWidth * 2 / 3;
	accessX[2] = windowWidth - 100;

	hDeskTop = FindDTWindow();
	if (hDeskTop == NULL)
	{
		std::cout << "Desktop_Info->Init()" << std::endl;
		return;
	}
	iconCount = (int)::SendMessage(hDeskTop, LVM_GETITEMCOUNT, 0, 0);

	LVITEM64 lvi, *_lvi;
	wchar_t item[512], subitem[512];
	wchar_t *_item, *_subitem;
	unsigned long pid;
	HANDLE process;

	GetWindowThreadProcessId(hDeskTop, &pid);
	process = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pid);

	_lvi = (LVITEM64*)VirtualAllocEx(process, NULL, sizeof(LVITEM64), MEM_COMMIT, PAGE_READWRITE);
	_item = (wchar_t*)VirtualAllocEx(process, NULL, 512 * sizeof(wchar_t), MEM_COMMIT, PAGE_READWRITE);
	_subitem = (wchar_t*)VirtualAllocEx(process, NULL, 512 * sizeof(wchar_t), MEM_COMMIT, PAGE_READWRITE);


	RECT* _rc = (RECT*)VirtualAllocEx(process, NULL, sizeof(RECT), MEM_COMMIT, PAGE_READWRITE);

	lvi.cchTextMax = 512;

	for (int i = 0; i < iconCount; i++) {

		RECT  rc;
		rc.left = LVIR_ICON;  //这个一定要设定 可以去看MSDN关于LVM_GETITEMRECT的说明
		lvi.iSubItem = 0;
		lvi.pszText = (_int64)_item;
		WriteProcessMemory(process, _lvi, &lvi, sizeof(LVITEM64), NULL);
		::SendMessage(hDeskTop, LVM_GETITEMTEXT, (WPARAM)i, (LPARAM)_lvi);

		lvi.iSubItem = 1;
		lvi.pszText = (_int64)_subitem;
		WriteProcessMemory(process, _lvi, &lvi, sizeof(LVITEM64), NULL);
		::SendMessage(hDeskTop, LVM_GETITEMTEXT, (WPARAM)i, (LPARAM)_lvi);

		::WriteProcessMemory(process, _rc, &rc, sizeof(rc), NULL);
		::SendMessage(hDeskTop, LVM_GETITEMRECT, (WPARAM)i, (LPARAM)_rc);

		ReadProcessMemory(process, _item, item, 512 * sizeof(wchar_t), NULL);
		ReadProcessMemory(process, _subitem, subitem, 512 * sizeof(wchar_t), NULL);


		ReadProcessMemory(process, _rc, &rc, sizeof(rc), NULL);
		iconInfo[i].iconName = item;
		iconInfo[i].index = i;
		iconInfo[i].rc = rc;
		//_cwprintf(L"%s - %s LF:%d TP:%d RT:%d BT:%d\n", item, subitem, rc.left, rc.top, rc.right, rc.bottom);
	}



	VirtualFreeEx(process, _lvi, 0, MEM_RELEASE);
	VirtualFreeEx(process, _item, 0, MEM_RELEASE);
	VirtualFreeEx(process, _subitem, 0, MEM_RELEASE);
	VirtualFreeEx(process, _rc, 0, MEM_RELEASE);

	CloseHandle(process);
}

/*1上，2下，3左，4右 ,左上角和右下角坐标（left,top）、(right,bottom)*/
RECT Desktop_Info::ChangeRECT(int direction, int distence, RECT rc)
{
	int heightRect = rc.bottom - rc.top;
	int widthRect = rc.right - rc.left;
	switch (direction)
	{
	case 1://向上移动
	{
		if (rc.top - distence < 5)
		{
			rc.top = 5;
			rc.bottom = rc.top + heightRect;
		}
		else
		{
			rc.top = rc.top - distence;
			rc.bottom = rc.bottom - distence;
		}
		break;
	}
	case 2://向下移动,（left,top）、(right,bottom)
	{
		if (rc.top + distence + heightRect > windowHeight)
		{
			rc.top = windowHeight - distence;
			rc.bottom = windowHeight;
		}
		else
		{
			rc.top = rc.top + distence;
			rc.bottom = rc.bottom + distence;
		}
		break;
	}
	case 3://向左移动,（left,top）、(right,bottom)
	{
		if (rc.left - distence < 0)
		{
			rc.left = 0;
			rc.right = windowWidth;
		}
		else
		{
			rc.left = rc.left - distence;
			rc.right = rc.right - distence;
		}
		break;
	}
	case 4://向右移动,（left,top）、(right,bottom)
	{
		if (rc.right + distence > windowWidth)
		{
			rc.left = windowWidth - widthRect;
			rc.right = windowWidth;
		}
		else
		{
			rc.left = rc.left - widthRect;
			rc.right = rc.right - widthRect;
		}
		break;
	}
	default:
		break;
	}
	return rc;
}
/*1上，2下，3左，4右 ,左上角和右下角坐标（left,top）、(right,bottom)*/
RECT Desktop_Info::ChangeRECT(int distence[], RECT rc)
{
	int heightRect = rc.bottom - rc.top;
	int widthRect = rc.right - rc.left;
	rc.left += distence[1];
	rc.top += distence[0];
	rc.right = rc.left + widthRect;
	rc.bottom = rc.top + heightRect;
	if (rc.left > windowWidth)
	{
		rc.left = 0;
		rc.right = rc.left + widthRect;
	}
	if (rc.top > windowHeight)
	{
		rc.top = 0;
		rc.bottom = rc.top + heightRect;
	}
	//std::cout << "left:" << rc.left << "\t" << "top" << rc.top << std::endl;
	return rc;
}
RECT Desktop_Info::SetRECT(int left, int top, RECT rc)
{
	int heightRect = rc.bottom - rc.top;
	int widthRect = rc.right - rc.left;
	rc.left = left;
	rc.top = top;
	rc.right = left + widthRect;
	rc.bottom = top + heightRect;
	return rc;
}


int* Desktop_Info::MapCreate(int checked, int* startPoint)
{
	int iconWidth = iconInfo[0].rc.right - iconInfo[0].rc.left;
	int iconHeight = iconInfo[0].rc.bottom - iconInfo[0].rc.top;
	//int *accessArray=new int[obstacleNumber];
	int interval = 15;
	srand((int)time(0));
	int accessDistence = 300;
	//int access1_Y = 0;
	//int access2_Y = 0;
	//int access3_Y = 0;
	int start = 0;
	int end = 0;
	for (int i = 0; i < accessNumber; i++)
	{
		start = random(0, windowHeight - accessDistence);//这是通道的开始y坐标
		end = start + accessDistence;
		if (start < iconHeight)//如果上面不够一个图标的位置，上面就不放图标了
			start = 0;
		startPoint[i * 2] = start;
		startPoint[i * 2 + 1] = end;
	}

	int i = 0;

	//std::cout << "Method accessArray:" << accessArray[0] << "\t" << accessArray[1] << std::endl;
	for (int j = 0; j < accessNumber; j++)
		i = ArrangeIcon(i, startPoint[j * 2], startPoint[j * 2 + 1], accessX[j], interval, checked);
	for (; i < iconCount; i++)
	{
		if (i == checked)
			continue;
		iconInfo[i].rc = SetRECT(0, 0, iconInfo[i].rc);
		Move(hDeskTop, iconInfo[i].index, iconInfo[i].rc);
	}
	return accessX;
}

int Desktop_Info::ArrangeIcon(int index, int start, int end, int x, int interval, int checked)
{
	int accessY = start;
	int i = index;
	//上
	for (; i < iconCount; i++)
	{
		if (i == checked)
			continue;
		int heightIcon = iconInfo[i].rc.bottom - iconInfo[i].rc.top;
		accessY = accessY - heightIcon;
		iconInfo[i].rc = SetRECT(x, accessY, iconInfo[i].rc);
		Move(hDeskTop, iconInfo[i].index, iconInfo[i].rc);
		if (accessY < 0)
			break;
		accessY = accessY - interval;

	}
	//下
	accessY = end;
	for (; i < iconCount; i++)
	{
		if (i == checked)
			continue;
		int heightIcon = iconInfo[i].rc.bottom - iconInfo[i].rc.top;
		iconInfo[i].rc = SetRECT(x, accessY, iconInfo[i].rc);
		Move(hDeskTop, iconInfo[i].index, iconInfo[i].rc);
		if (accessY > windowHeight)
			break;
		accessY = accessY + heightIcon + interval;

	}

	return i;
}
/*（left,top）、(right,bottom)思路，如果上面的*/
bool Desktop_Info::IsFail(int * accessArray, int *startPoint, RECT rc)
{
	for (int i = 0; i < 3; i++)
	{
		if (isInRec(accessArray[i], startPoint[i * 2], startPoint[i * 2 + 1], rc))
		{
			score += i;
			return false;
		}
	}

	return true;
}
//判断线段是否相交
bool Desktop_Info::Intersection(Line l1, Line  l2)
{
	//快速排斥实验
	if ((l1.x1 > l1.x2 ? l1.x1 : l1.x2) < (l2.x1 < l2.x2 ? l2.x1 : l2.x2) ||
		(l1.y1 > l1.y2 ? l1.y1 : l1.y2) < (l2.y1 < l2.y2 ? l2.y1 : l2.y2) ||
		(l2.x1 > l2.x2 ? l2.x1 : l2.x2) < (l1.x1 < l1.x2 ? l1.x1 : l1.x2) ||
		(l2.y1 > l2.y2 ? l2.y1 : l2.y2) < (l1.y1 < l1.y2 ? l1.y1 : l1.y2))
	{
		return false;
	}
	//跨立实验
	if ((((l1.x1 - l2.x1)*(l2.y2 - l2.y1) - (l1.y1 - l2.y1)*(l2.x2 - l2.x1))*
		((l1.x2 - l2.x1)*(l2.y2 - l2.y1) - (l1.y2 - l2.y1)*(l2.x2 - l2.x1))) > 0 ||
		(((l2.x1 - l1.x1)*(l1.y2 - l1.y1) - (l2.y1 - l1.y1)*(l1.x2 - l1.x1))*
		((l2.x2 - l1.x1)*(l1.y2 - l1.y1) - (l2.y2 - l1.y1)*(l1.x2 - l1.x1))) > 0)
	{
		return false;
	}
	return true;
}

// 计算 |p1 p2| X |p1 p|
float Desktop_Info::GetCross(Point& p1, Point& p2, Point& p)
{
	return (p2.x - p1.x) * (p.y - p1.y) - (p.x - p1.x) * (p2.y - p1.y);
}
bool Desktop_Info::isInRec(int x, int start, int end, RECT rc)
{
	int width = rc.right - rc.left;
	int height = rc.bottom - rc.top;
	int rectification = 20;
	if (start > 0)
	{

		Point p_icon_right_up(rc.right - rectification, rc.top);
		Point point1_left_up(x, 0);
		Point point1_left_down(x, start);
		Point point1_right_down(x + width, start);
		Point point1_right_up(x + width, 0);
		if (IsPointInMatrix(p_icon_right_up, point1_left_up, point1_left_down, point1_right_down, point1_right_up))
		{
			std::cout << "Point:" << rc.right << " " << rc.top << "\t" << "start: " << start << std::endl;
			return true;
		}
	}
	if (end != windowHeight)
	{
		Point p_icon_right_down(rc.right - rectification, rc.bottom);

		Point point2_left_up(x, end);
		Point point2_left_down(x, windowHeight);
		Point point2_right_down(x + width, windowHeight);
		Point point2_right_up(x + width, end);
		if (IsPointInMatrix(p_icon_right_down, point2_left_up, point2_left_down, point2_right_down, point2_right_up))
			return true;
	}
	return false;
}
void Desktop_Info::ResetMoveIcon(int checked)
{
	iconInfo[checked].rc = SetRECT(0, 0, iconInfo[checked].rc);
	Move(hDeskTop, iconInfo[checked].index, iconInfo[checked].rc);
}
int Desktop_Info::round_double(double number)
{
	return (number > 0.0) ? (number + 0.5) : (number - 0.5);
}
void Desktop_Info::DrowScore(int score)
{
	int iconWidth = iconInfo[0].rc.right - iconInfo[0].rc.left;
	int iconHeight = iconInfo[0].rc.bottom - iconInfo[0].rc.top;
	int interval = 80;//数字之间的间隔
	int iconIndex = 0;//调用的icon的序号
	NumberSet numberSet;
	numberSet.Init();
	std::string scoreStr = std::to_string(score);
	int sumWidth = 0;
	int sumHeitht = iconHeight * numberSet.numbers[0].height;
	for (int i = 0; i < scoreStr.length(); i++)
	{
		sumWidth += numberSet.numbers[i].width;
	}
	sumWidth = (scoreStr.length() - 1)*interval + sumWidth * iconWidth;
	int xx = (windowWidth - sumWidth) / 2 - iconWidth / 2;
	int yy = (windowHeight - sumHeitht) / 2;
	for (int i = 0; i < scoreStr.length(); i++)
	{
		int numberIndex = scoreStr[i] - '0';
		iconIndex = DrowNumber(numberSet.GetNumber(numberIndex), xx, yy, iconWidth, iconHeight, iconIndex);
		xx += (numberSet.GetNumber(numberIndex).width*iconWidth + interval);
	}
	for (int i = iconIndex; i < iconCount; i++)
	{
		ResetMoveIcon(i);
	}
}
//画一个具体数字
int Desktop_Info::DrowNumber(NumberItem number, int x, int y, int iconWidth, int iconHeight, int iconIndex)
{
	for (int i = 0; i < number.iconCount; i++)
	{
		int width = iconInfo[iconIndex].rc.right - iconInfo[iconIndex].rc.left;
		int height = iconInfo[iconIndex].rc.bottom - iconInfo[iconIndex].rc.top;
		int xx = number.points[i].x*iconWidth + x;
		int yy = number.points[i].y*iconHeight + y;
		std::cout << number.points[i].x << "、" << number.points[i].y << "\t(" << xx << "、" << yy << ")" << std::endl;
		iconInfo[iconIndex].rc = SetRECT(xx, yy, iconInfo[iconIndex].rc);
		Move(hDeskTop, iconInfo[iconIndex].index, iconInfo[iconIndex].rc);
		iconIndex++;
	}
	return iconIndex;
}
void Desktop_Info::DrowTime()
{
	CTime time = CTime::GetCurrentTime();
	int min = time.GetMinute();
	int second = time.GetSecond();
	int iconWidth = iconInfo[0].rc.right - iconInfo[0].rc.left;
	int iconHeight = iconInfo[0].rc.bottom - iconInfo[0].rc.top;
	int interval = 80;//数字之间的间隔
	int iconIndex = 0;//调用的icon的序号
	NumberSet numberSet;
	numberSet.Init();
	std::string minStr = std::to_string(min);
	std::string secondStr = std::to_string(second);
	std::string iconStr = min + "." + second;

	int sumWidth = 0;
	int sumHeitht = iconHeight * numberSet.numbers[0].height;
	for (int i = 0; i < iconStr.length(); i++)
	{
		sumWidth += numberSet.numbers[i].width;
	}
	sumWidth = (iconStr.length() - 1)*interval + sumWidth * iconWidth;
	int xx = (windowWidth - sumWidth) / 2 - iconWidth / 2;
	int yy = (windowHeight - sumHeitht) / 2;
	for (int i = 0; i < iconStr.length(); i++)
	{
		int numberIndex;
		if (iconStr[i] == '.')
			numberIndex = 10;
		else
			numberIndex = iconStr[i] - '0';
		std::cout << "numberIndex:" << numberIndex << std::endl;
		iconIndex = DrowNumber(numberSet.GetNumber(numberIndex), xx, yy, iconWidth, iconHeight, iconIndex);
		xx += (numberSet.GetNumber(numberIndex).width*iconWidth + interval);
		MessageBox(NULL, std::to_string(iconIndex).data(), "信息", MB_ICONINFORMATION);
		Sleep(3000);
	}
	
	for (int i = iconIndex; i < iconCount; i++)
	{
		ResetMoveIcon(i);
	}
}
//左上，左下、右下、右上
bool Desktop_Info::IsPointInMatrix(Point & p, Point &p1, Point &p2, Point &p3, Point &p4)
{
	return GetCross(p1, p2, p) * GetCross(p3, p4, p) >= 0 && GetCross(p2, p3, p) * GetCross(p4, p1, p) >= 0;
}


void Desktop_Info::Execute()
{
	int checked = 0;//被选定的移动的图标
	int *startPoint = new int[2 * accessNumber];
	int *accessArray = NULL;

	bool isPressed = false;//代表是否按压，按true
	bool isPressedRecord = false;//代表是否按压过，按过代表true
	bool isOut = true;//代表是否可以输出

	double t = 0;
	//iconInfo[0].rc = SetRECT(1920/3,540,iconInfo[0].rc);
	//Move(hDeskTop, 0, iconInfo[0].rc);

	while (1)
	{
		ResetMoveIcon(checked);
		std::cout << "Are you ready ? (enter begin; q exit !)" << std::endl;
		while (1)
		{
			Sleep(10);
			if (GetAsyncKeyState(32) & 0x8000)//回车键
			{
				break;
			}
		}
		accessArray = MapCreate(checked, startPoint);

		isPressed = true;
		t = 0;
		score = 0;
		while (1)
		{
			//退出游戏
			if (GetAsyncKeyState(113) & 0x8000 || GetAsyncKeyState(81) & 0x8000)//Q键
			{
				std::cout << "Score: " << score << "\t" "Exit Game!" << std::endl;
				break;
			}
			if (!IsFail(accessArray, startPoint, iconInfo[checked].rc))
			{
				std::cout << "Score: " << score << "\t" << "Game Over!" << std::endl;

				DrowScore(score);
				while (1)
				{
					Sleep(10);
					if (GetAsyncKeyState(32) & 0x8000)//回车键
					{
						break;
					}
				}
				break;
			}
			Sleep(5);
			int h = (int)(round_double(1 * t));
			int s = (int)(2);
			int distenceRect[2];
			distenceRect[0] = h;
			distenceRect[1] = s;
			if (iconInfo[checked].rc.left + s > windowWidth)
			{
				score += accessNumber;
				int *accessArray = MapCreate(checked, startPoint);
				std::cout << "accessArray:" << accessArray[0] << "\t" << accessArray[1] << std::endl;
			}
			iconInfo[checked].rc = ChangeRECT(distenceRect, iconInfo[checked].rc);
			Move(hDeskTop, iconInfo[checked].index, iconInfo[checked].rc);
			if (GetAsyncKeyState(32) & 0x8000)//空格键
			{
				if (isOut)
				{
					t = -3;
					//iconInfo[checked].rc = ChangeRECT(1, 50, iconInfo[0].rc);
					std::cout << iconInfo[checked].iconName << "\t" << "left: " << iconInfo[checked].rc.left << "\t" << iconInfo[checked].rc.top << std::endl;
					//Move(hDeskTop, 0, iconInfo[checked].rc);

					isOut = false;
				}
				isPressed = true;
			}
			else
			{
				isPressed = false;
			}

			if (isPressed)
			{
				isPressedRecord = true;
			}
			if (isPressedRecord == true && isPressed == false)
			{
				isPressedRecord = false;
				isOut = true;
			}
			t += 0.05;
		}

	}

}

