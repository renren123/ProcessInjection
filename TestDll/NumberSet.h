#pragma once
#include "NumberItem.h"
class NumberSet
{
public:
	NumberItem number_0;
	NumberItem number_1;
	NumberItem number_2;
	NumberItem number_3;
	NumberItem number_4;
	NumberItem number_5;
	NumberItem number_6;
	NumberItem number_7;
	NumberItem number_8;
	NumberItem number_9;
	NumberItem number_10;//Ã°ºÅ
	NumberItem * numbers = NULL;
	NumberSet();
	~NumberSet();
	NumberItem GetNumber(int number);
	NumberItem * Init();

};

