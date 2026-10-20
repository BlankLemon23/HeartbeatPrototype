#include "Position.hpp"

bool operator == (const Position& lhs, const Position& rhs) {
	return lhs.row == rhs.row && lhs.column == rhs.column;
}//lhs和rhs是常用的命名方式，表示左侧和右侧操作数（left-hand side / right-hand side），不是左值和右值分类
