// Desktop entry point.
#include "raylib.h"
#include <iostream>
#include<string>
#include"Map.hpp"

int main() {
    const int cellSize = 40;
    std::string title;
	std::vector<std::string> map = { "*****",
									"*...*",
									"*.*.*",
									"*...*" };
	int maxColumns = 0;
	for (int i = 0; i < map.size(); i++) {

		if (map[i].length() > maxColumns) {
			maxColumns = map[i].length();
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
		for (int i = 0; i < map.size(); i++) {
			for (int j = 0; j < map[i].size();j++) {
			const CellType cellType = classifyCell(map, i, j);
			switch (cellType) {
			case CellType::Wall:
				DrawRectangle(j * cellSize, i * cellSize, cellSize, cellSize, BLACK);
				break;
			case CellType::Space:
				DrawRectangle(j * cellSize, i * cellSize, cellSize, cellSize, WHITE);
				break;
			case CellType::OutOfBounds:
			case CellType::Unknown:
				DrawRectangle(j * cellSize, i * cellSize, cellSize, cellSize, RED);
				break;
			}
			}
		
		}
			
		EndDrawing();
    }
	CloseWindow();

    return 0;
}
