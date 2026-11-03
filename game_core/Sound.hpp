#pragma once

#include "Position.hpp"
#include <vector>

class Enemy;

// Broadcast one fixed source; each enemy applies the supplied circular radius.
void makeSound(Position soundSource, int soundRadiusCells,
               std::vector<Enemy>& enemies);
