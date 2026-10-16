// Desktop entry point.
#include "raylib.h"
#include <iostream>
#include<string>
#include"Map.hpp"
#include "Player.hpp"
int main() {
	Player player({ 1, 1 });
    const int cellSize = 40;
    std::string title;
	std::vector<std::string> map = { "*****",
									"*...*",
									"*.*.*",
									"*...*" };
	int maxColumns = 0;
	for (int row = 0; row < map.size(); row++) {

		if (map[row].length() > maxColumns) {
			maxColumns = map[row].length();
		}
	}
	const int width = maxColumns * cellSize;
	const int height = map.size() * cellSize;
	
	std::cout << "Enter window title:" << std::endl;
	std::cin >> title;

    InitWindow(width, height, title.c_str());
    SetTargetFPS(60);
    while (true) {
		if (WindowShouldClose()) {
			break;
		}
		BeginDrawing();

		ClearBackground(RAYWHITE);

		for (int row = 0; row < map.size(); row++) {
			for (int column = 0; column < map[row].size();column++) {
			const CellType cellType = classifyCell(map, row, column);
			const int screenX = column * cellSize;
			const int screenY = row * cellSize;
			switch (cellType) {
			case CellType::Wall:
				DrawRectangle(screenX, screenY, cellSize, cellSize, BLACK);
				break;
			case CellType::Space:
				DrawRectangle(screenX, screenY, cellSize, cellSize, WHITE);
				break;
			case CellType::OutOfBounds:
			case CellType::Unknown:
				DrawRectangle(screenX, screenY, cellSize, cellSize, RED);
				break;
			}
			}

		}

		if (IsKeyPressed(KEY_RIGHT)) {
			player.tryMove(Direction::RIGHT, map);
		}
		else if (IsKeyPressed(KEY_UP)) {
			player.tryMove(Direction::UP, map);
		}
		else if (IsKeyPressed(KEY_DOWN)) {
			player.tryMove(Direction::DOWN, map);
		}
		else if (IsKeyPressed(KEY_LEFT)) {
			player.tryMove(Direction::LEFT, map);
		}

		DrawRectangle(player.getPosition().column * cellSize, player.getPosition().row * cellSize, cellSize, cellSize, BLUE);

		EndDrawing();
    }
	CloseWindow();

    return 0;
}
