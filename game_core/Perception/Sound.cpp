#include "Sound.hpp"
#include "Actors/Enemy.hpp"

void makeSound(Position soundSource, int soundRadiusCells,
               std::vector<Enemy>& enemies) {
    for (Enemy& enemy : enemies) {
        enemy.hearSound(soundSource, soundRadiusCells);
    }
}
