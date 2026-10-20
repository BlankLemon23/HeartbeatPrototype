#pragma once

struct Position
{
	int row;
	int column;
};

bool operator == (const Position& lhs, const Position& rhs);
