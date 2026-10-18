// Desktop entry point.
#include "raylib.h"
#include <iostream>
#include<string>
#include"Map.hpp"
#include "Player.hpp"
#include "ScanController.hpp"
int main() {
	Player player({ 1, 1 });
    const int cellSize = 40;
    std::string title;
	Map map;
	map.setMap({
	"******************************",
	"*..............*.............*",
	"*..............*.............*",
	"*....*****.....*....*****....*",
	"*....*...*..........*...*....*",
	"*....*...*.....*....*...*....*",
	"*....**.**.....*....**.**....*",
	"*..............*.............*",
	"*..............*.............*",
	"***.******************.*******",
	"*.........*..................*",
	"*.........*..................*",
	"*.........*....*******.......*",
	"*..............*.....*.......*",
	"*.........*....*.....*.......*",
	"*.........*....***.***.......*",
	"*.........*..................*",
	"*.........*..................*",
	"*............................*",
	"******************************"
		});
	int maxColumns = 0;
	for (int row = 0; row < map.getMap().size(); row++) {

		if (map.getMap()[row].length() > maxColumns) {
			maxColumns = map.getMap()[row].length();
		}
	}
	const int width = maxColumns * cellSize;
	const int height = map.getMap().size() * cellSize;

	std::cout << "Enter window title:" << std::endl;
	std::cin >> title;

    InitWindow(width, height, title.c_str());
    SetTargetFPS(60);
	const double manualScanIntervalSeconds = 5.0;
	ScanController scanController(false, 0, 5);
    while (true) {
		if (WindowShouldClose()) {
			break;
		}

		scanController.updateScanTime(GetFrameTime());

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

		if (IsKeyPressed(KEY_SPACE) && scanController.canScanManually()) {
			scanController.scan(player.getPosition(), map, manualScanIntervalSeconds);
		}

		BeginDrawing();


		ClearBackground(RAYWHITE);

		for (int row = 0; row < map.getMap().size(); row++) {
			for (int column = 0; column < maxColumns; column++) {
			const CellType cellType = map.classifyCell(row, column);
			const int screenX = column * cellSize;
			const int screenY = row * cellSize;
			if (map.isScanned(row, column)) {
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
			else {
				DrawRectangle(screenX, screenY, cellSize, cellSize, GRAY);
			}

			}

		}

		DrawRectangle(player.getPosition().column * cellSize, player.getPosition().row * cellSize, cellSize, cellSize, BLUE);

		EndDrawing();
    }
	CloseWindow();

    return 0;
}
