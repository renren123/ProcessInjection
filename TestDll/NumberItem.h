#pragma once
class NumberItem
{

public:
	struct Point
	{
		int x;
		int y;
		Point()
		{

		}
		Point(int x, int y)
		{
			this->x = x;
			this->y = y;
		}
	};
	NumberItem();
	~NumberItem();
	int width = 0;
	int height = 0;
	int iconCount = 0;
	Point * points = NULL;
	Point  SetPointValue(int x, int y, NumberItem::Point point);
};

