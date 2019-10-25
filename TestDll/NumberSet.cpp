#include "stdafx.h"
#include "NumberSet.h"

NumberSet::NumberSet()
{

}


NumberSet::~NumberSet()
{
}

NumberItem NumberSet::GetNumber(int number)
{
	return numbers[number];
}

NumberItem * NumberSet::Init()
{
	const int number0Count = 10;
	NumberItem::Point* number_0_points = new NumberItem::Point[number0Count];
	//number_0.points = new NumberItem::Point[number0Count];
	number_0.width = 3;
	number_0.height = 6;
	number_0.iconCount = number0Count;
	int arrays0[number0Count][2] = {
	{0,1},
	{0,2},
	{0,3},
	{0,4},
	{1,0},
	{1,5},
	{2,1},
	{2,2},
	{2,3},
	{2,4}
	};
	for (int i = 0; i < number0Count; i++)
	{
		number_0_points[i] = number_0.SetPointValue(arrays0[i][0], arrays0[i][1], number_0_points[i]);
	}
	number_0.points = number_0_points;

	const int number1Count = 7;
	NumberItem::Point* number_1points = new NumberItem::Point[number1Count];
	number_1.width = 2;
	number_1.height = 7;
	number_1.iconCount = number1Count;
	int arrays1[number1Count][2] = {
	{0,1},
	{1,0},
	{1,1},
	{1,2},
	{1,3},
	{1,4},
	{1,5}
	};
	for (int i = 0; i < number1Count; i++)
	{
		number_1points[i] = number_1.SetPointValue(arrays1[i][0], arrays1[i][1], number_1points[i]);
	}
	number_1.points = number_1points;

	const int number2Count = 11;
	number_2.points = new NumberItem::Point[number2Count];
	number_2.width = 4;
	number_2.height = 6;
	number_2.iconCount = number2Count;
	int arrays2[number2Count][2] = {
	{0,1},
	{0,5},
	{1,0},
	{1,4},
	{1,5},
	{2,0},
	{2,3},
	{2,5},
	{3,1},
	{3,2},
	{3,5}
	};
	for (int i = 0; i < number2Count; i++)
	{
		number_2.points[i] = number_2.SetPointValue(arrays2[i][0], arrays2[i][1], number_2.points[i]);
	}

	const int number3Count = 10;
	number_3.points = new NumberItem::Point[number3Count];
	number_3.width = 4;
	number_3.height = 6;
	number_3.iconCount = number3Count;
	int arrays3[number3Count][2] = {
	{0,1},
	{0,4},
	{1,0},
	{1,5},
	{2,0},
	{2,2},
	{2,3},
	{2,5},
	{3,1},
	{3,4}
	};
	for (int i = 0; i < number3Count; i++)
	{
		number_3.points[i] = number_3.SetPointValue(arrays3[i][0], arrays3[i][1], number_3.points[i]);
	}

	const int number4Count = 11;
	number_4.points = new NumberItem::Point[number4Count];
	number_4.width = 4;
	number_4.height = 6;
	number_4.iconCount = number4Count;
	int arrays4[number4Count][2] = {
	{0,2},
	{0,3},
	{1,1},
	{1,3},
	{2,0},
	{2,1},
	{2,2},
	{2,3},
	{2,4},
	{2,5},
	{3,3}
	};
	for (int i = 0; i < number4Count; i++)
	{
		number_4.points[i] = number_4.SetPointValue(arrays4[i][0], arrays4[i][1], number_4.points[i]);
	}

	const int number5Count = 10;
	number_5.points = new NumberItem::Point[number5Count];
	number_5.width = 3;
	number_5.height = 6;
	number_5.iconCount = number5Count;
	int arrays5[number5Count][2] = {
	{0,0},
	{0,1},
	{0,2},
	{0,5},
	{1,0},
	{1,2},
	{1,5},
	{2,0},
	{2,3},
	{2,4}
	};
	for (int i = 0; i < number5Count; i++)
	{
		number_5.points[i] = number_5.SetPointValue(arrays5[i][0], arrays5[i][1], number_5.points[i]);
	}

	const int number6Count = 10;
	number_6.points = new NumberItem::Point[number6Count];
	number_6.width = 3;
	number_6.height = 6;
	number_6.iconCount = number6Count;
	int arrays6[number6Count][2] = {
	{0,1},
	{0,2},
	{0,3},
	{0,4},
	{1,0},
	{1,2},
	{1,5},
	{2,0},
	{2,3},
	{2,4}
	};
	for (int i = 0; i < number6Count; i++)
	{
		number_6.points[i] = number_6.SetPointValue(arrays6[i][0], arrays6[i][1], number_6.points[i]);
	}

	const int number7Count = 8;
	number_7.points = new NumberItem::Point[number7Count];
	number_7.width = 3;
	number_7.height = 6;
	number_7.iconCount = number7Count;
	int arrays7[number7Count][2] = {
	{0,0},
	{1,0},
	{2,0},
	{2,1},
	{2,2},
	{2,3},
	{2,4},
	{2,5}
	};
	for (int i = 0; i < number7Count; i++)
	{
		number_7.points[i] = number_7.SetPointValue(arrays7[i][0], arrays7[i][1], number_7.points[i]);
	}

	const int number8Count = 15;
	number_8.points = new NumberItem::Point[number8Count];
	number_8.width = 3;
	number_8.height = 6;
	number_8.iconCount = number8Count;
	int arrays8[number8Count][2] = {
	{0,0},
	{0,1},
	{0,2},
	{0,3},
	{0,4},
	{0,5},
	{1,0},
	{1,3},
	{1,5},
	{2,0},
	{2,1},
	{2,2},
	{2,3},
	{2,4},
	{2,5}
	};
	for (int i = 0; i < number8Count; i++)
	{
		number_8.points[i] = number_8.SetPointValue(arrays8[i][0], arrays8[i][1], number_8.points[i]);
	}

	const int number9Count = 9;
	number_9.points = new NumberItem::Point[number9Count];
	number_9.width = 3;
	number_9.height = 6;
	number_9.iconCount = number9Count;
	int arrays9[number9Count][2] = {
	{0,1},
	{0,2},
	{0,5},
	{1,0},
	{1,3},
	{1,4},
	{2,1},
	{2,2},
	{2,3}
	};
	for (int i = 0; i < number9Count; i++)
	{
		number_9.points[i] = number_9.SetPointValue(arrays9[i][0], arrays9[i][1], number_9.points[i]);
	}

	const int number10Count = 2;
	number_10.points = new NumberItem::Point[number10Count];
	number_10.width = 1;
	number_10.height = 6;
	number_10.iconCount = number10Count;
	int arrays10[number10Count][2] = {
	{0,1},
	{0,2}
	};
	for (int i = 0; i < number10Count; i++)
	{
		number_10.points[i] = number_10.SetPointValue(arrays10[i][0], arrays10[i][1], number_10.points[i]);
	}
	NumberItem * numbersTemp = new  NumberItem[11];

	numbersTemp[0] = number_0;
	numbersTemp[1] = number_1;
	numbersTemp[2] = number_2;
	numbersTemp[3] = number_3;
	numbersTemp[4] = number_4;
	numbersTemp[5] = number_5;
	numbersTemp[6] = number_6;
	numbersTemp[7] = number_7;
	numbersTemp[8] = number_8;
	numbersTemp[9] = number_9;
	numbersTemp[10] = number_10;
	this->numbers = numbersTemp;
	return NULL;
}

