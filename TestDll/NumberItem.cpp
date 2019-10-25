#include "stdafx.h"
#include "NumberItem.h"

NumberItem::NumberItem()
{
}


NumberItem::~NumberItem()
{
}

NumberItem::Point NumberItem::SetPointValue(int x, int y, NumberItem::Point point)
{
	point.x = x;
	point.y = y;
	return point;
}
