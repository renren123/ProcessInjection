#pragma once
#include <afx.h>
#include <string>
#include "IconInfo.h"
#include "NumberSet.h"
#include "NumberItem.h"
class Desktop_Info
{
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
public:
	Desktop_Info();
	~Desktop_Info();

	int iconCount = 0;
	HWND hDeskTop = NULL;
	IconInfo iconInfo[256];
	int windowHeight = 0;
	int windowWidth = 0;
	int score = 0;//得分；
	int *accessX;
	const int accessNumber = 3;

	HWND GetHwndDesktop();
	HWND FindDTWindow();
	void Move(HWND hDeskTop, int index, RECT  rc);
	void Init();
	void Execute();
	RECT ChangeRECT(int direction, int distence, RECT rc);
	RECT ChangeRECT(int distence[], RECT rc);
	RECT SetRECT(int left, int top, RECT rc);
	int* MapCreate(int checked, int* startPoint);
	int ArrangeIcon(int index, int start, int end, int x, int interval, int checked);
	bool IsFail(int *accessArray, int *startPoint, RECT rc);
	bool Intersection(Line l1, Line l2);
	bool IsPointInMatrix(Point & p, Point &p1, Point &p2, Point &p3, Point &p4);
	float GetCross(Point& p1, Point& p2, Point& p);
	bool isInRec(int x, int start, int end, RECT rc);
	void ResetMoveIcon(int checked);
	int round_double(double number);
	void DrowScore(int score);
	int DrowNumber(NumberItem number, int x, int y, int iconWidth, int iconHeight, int iconIndex);//左上方坐标
	void DrowTime();
	LPCWSTR stringToLPCWSTR(std::string orig);
};

