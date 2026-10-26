// Desktop entry point.
#include "raylib.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include "Enemy.hpp"
#include "Map.hpp"
#include "Player.hpp"
#include "ScanController.hpp"

int main() {
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

	const int cellSize = 40;
	const int maxColumns = map.maxColumnCount();
	const int width = maxColumns * cellSize;
	const int height = map.rowCount() * cellSize;

	PlayerConfig playerConfig{};
	playerConfig.maxHealth = 100;
	playerConfig.normalMoveIntervalSeconds = 0.5;
	playerConfig.stealthMoveIntervalSeconds = 1.0;
	playerConfig.outBattleIntervalSeconds = 2.0;
	Player player({ 1, 1 }, playerConfig);

	const double manualScanIntervalSeconds = 3.0;

	BattleScanConfig battleScanConfig{};
	battleScanConfig.highHealthIntervalSeconds = 3.0;
	battleScanConfig.mediumHealthIntervalSeconds = 2.0;
	battleScanConfig.lowHealthIntervalSeconds = 1.2;

	const double initialScanCooldownSeconds = 0.0;
	const double scanRadiusCells = 5.0;
	const double enemySnapshotDurationSeconds = 0.5;
	ScanController scanController(initialScanCooldownSeconds, scanRadiusCells, enemySnapshotDurationSeconds);

	EnemyConfig enemyConfig{};
	enemyConfig.startingHealth = 100;
	enemyConfig.moveIntervalSeconds = 2;
	enemyConfig.wanderIntervalSeconds = 0.5;
	enemyConfig.wanderingRadiusCells = 3;
	enemyConfig.waitAfterLossSeconds = 2.0;
	enemyConfig.normalDetectionRadiusCells = 8;
	enemyConfig.stealthDetectionRadiusCells = 3;
	enemyConfig.stopChaseRadiusCells = 15;

	std::vector<Enemy> enemies;
	enemies.emplace_back(Position{ 1, 11 }, enemyConfig);
	enemies.emplace_back(Position{ 1, 2 }, enemyConfig);


	std::string title;
	std::cout << "Enter window title:" << std::endl;
	std::cin >> title;

	InitWindow(width, height, title.c_str());
	SetTargetFPS(60);

	while (true) {
		if (WindowShouldClose()) {
			break;
		}

		const double elapsedSeconds = GetFrameTime();
		scanController.updateScanTime(elapsedSeconds);
		scanController.updateSnapshotRemainingSeconds(elapsedSeconds);
		player.updateMoveCooldown(elapsedSeconds);

		if (IsKeyPressed(KEY_LEFT_SHIFT)) {
			player.toggleStealth();
		}

		if (IsKeyDown(KEY_RIGHT)) {
			player.tryMove(Direction::RIGHT, map);
		}
		else if (IsKeyDown(KEY_UP)) {
			player.tryMove(Direction::UP, map);
		}
		else if (IsKeyDown(KEY_DOWN)) {
			player.tryMove(Direction::DOWN, map);
		}
		else if (IsKeyDown(KEY_LEFT)) {
			player.tryMove(Direction::LEFT, map);
		}


		for (auto& enemy : enemies) {
			enemy.updateEnemy(elapsedSeconds, map, player.getPosition(), player.isStealthEnabled());
		}

		const bool allNotChasing = std::all_of(
			enemies.begin(), enemies.end(),
			[](const Enemy& e) { return !e.isChasing(); }
		);

		//语法：•	[] —— 捕获列表,空表示不捕获任何外部变量
		//•(const Enemy& e) —— 参数, all_of 会依次把每个敌人传进来
		//•{ return !e.isChasing(); } —— 函数体, 返回"该敌人没有在追"这个布尔值
		//对 enemies 里的每个敌人 e,判断 !e.isChasing();如果所有敌人都满足(即都不在追),allNotChasing 就是 true。
		if (!allNotChasing) {
			player.setInBattle(true);
			player.resetOutBattleRemainingSeconds();
		}
		else if (player.isInBattle()) {
			player.updateOutBattleRemainingSeconds(elapsedSeconds);
			if (player.getOutBattleRemainingSeconds() <= 0.0) {
				player.setInBattle(false);
			}
		}



		if (IsKeyPressed(KEY_SPACE)) {
			scanController.tryScanManually(player, map, manualScanIntervalSeconds, enemies);
		}

		scanController.updateBattleAutoScan(player, map, battleScanConfig, enemies);

		BeginDrawing();
		ClearBackground(RAYWHITE);

		for (int row = 0; row < map.rowCount(); row++) {
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

		DrawRectangle(player.getPosition().column * cellSize, player.getPosition().row * cellSize,
			cellSize, cellSize, player.isStealthEnabled() ? PURPLE : BLUE);
		for (const Position snapshotPosition : scanController.getEnemySnapshotPositions()) {
			DrawRectangle(snapshotPosition.column * cellSize, snapshotPosition.row * cellSize, cellSize, cellSize, YELLOW);
		}

		EndDrawing();
	}
	CloseWindow();

	return 0;
}
