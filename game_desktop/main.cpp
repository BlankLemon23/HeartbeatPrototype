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
#include "Sound.hpp"
#include "Weapons/Molotov.hpp"
#include "Weapons/DecoyMine.hpp"
#include "Weapons/Gun.hpp"

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
	const float fireCircleRadiusPixels = cellSize * 0.5f;
	const Color fireColor = Fade(ORANGE, 0.55f);
	const Color throwPreviewColor = Fade(SKYBLUE, 0.7f);
	const float throwPreviewLineThicknessPixels = 2.0f;
	const Color aimLineColor = Fade(RED, 0.75f);
	const float aimLineThicknessPixels = 2.0f;
	const int maxColumns = map.maxColumnCount();
	const int width = maxColumns * cellSize;
	const int height = map.rowCount() * cellSize;

	PlayerConfig playerConfig{};
	Player player({ 1, 1 }, playerConfig);

	const double manualScanIntervalSeconds = 3.0;

	BattleScanConfig battleScanConfig{};

	const double initialScanCooldownSeconds = 0.0;
	const double scanRadiusCells = 5.0;
	const double enemySnapshotDurationSeconds = 0.5;
	ScanController scanController(initialScanCooldownSeconds, scanRadiusCells, enemySnapshotDurationSeconds);

	EnemyConfig enemyConfig{};
	std::vector<Enemy> enemies;
	enemies.emplace_back(Position{ 1, 11 }, enemyConfig);
	enemies.emplace_back(Position{ 1, 2 }, enemyConfig);

	MolotovConfig molotovConfig{};
	Molotov molotov(molotovConfig);
	bool isPreparingMolotovThrow = false;

	DecoyMineConfig decoyMineConfig{};
	DecoyMine decoyMine(decoyMineConfig);
	bool isPreparingDecoyMineThrow = false;

	GunConfig gunConfig{};
	Gun gun(gunConfig);

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
		gun.update(elapsedSeconds);

		if (IsKeyPressed(KEY_LEFT_SHIFT)) {
			player.toggleStealth();
		}

		if (IsKeyDown(KEY_D)) {
			player.tryMove(Direction::RIGHT, map);
		}
		else if (IsKeyDown(KEY_W)) {
			player.tryMove(Direction::UP, map);
		}
		else if (IsKeyDown(KEY_S)) {
			player.tryMove(Direction::DOWN, map);
		}
		else if (IsKeyDown(KEY_A)) {
			player.tryMove(Direction::LEFT, map);
		}

		if (IsKeyPressed(KEY_R)) {
			gun.tryReload();
		}

		if (IsKeyPressed(KEY_V)) {
			isPreparingMolotovThrow = !isPreparingMolotovThrow;
			isPreparingDecoyMineThrow = false;
		}

		if (IsKeyPressed(KEY_B)) {
			isPreparingMolotovThrow = false;
			isPreparingDecoyMineThrow = !isPreparingDecoyMineThrow;
		}

		const Vector2 mousePosition = GetMousePosition();
		//mousePosition.x >= 0.0f	鼠标 x 坐标不为负（不在窗口左边界之外）
		//mousePosition.x < width	鼠标 x 坐标小于窗口宽度（不在右边界之外）
		//mousePosition.y >= 0.0f	鼠标 y 坐标不为负（不在窗口上边界之外）
		//mousePosition.y < height	鼠标 y 坐标小于窗口高度（不在下边界之外）
		const bool isMouseInsideWindow =
			mousePosition.x >= 0.0f && mousePosition.x < width &&
			mousePosition.y >= 0.0f && mousePosition.y < height;
		// Firing and the aim preview use the same mouse sample in cell units.
		const GridPoint aimPosition{
			static_cast<double>(mousePosition.y) / cellSize,
			static_cast<double>(mousePosition.x) / cellSize
		};

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isMouseInsideWindow) {
			if (isPreparingMolotovThrow || isPreparingDecoyMineThrow) {
				const int column = static_cast<int>(mousePosition.x) / cellSize;
				const int row = static_cast<int>(mousePosition.y) / cellSize;
				const Position clickedPosition{row, column};

				bool throwSucceeded = false;
				if (isPreparingMolotovThrow) {
					throwSucceeded = molotov.tryThrow(player.getPosition(), clickedPosition, map);
				}
				else {
					throwSucceeded =
						decoyMine.tryThrow(player.getPosition(), clickedPosition, map, enemies);
				}
				if (throwSucceeded) {
					isPreparingMolotovThrow = false;
					isPreparingDecoyMineThrow = false;
				}
			}
			else {
				gun.tryFire(player.getPosition(), aimPosition, map, enemies);
			}
		}

		molotov.updateBurningArea(elapsedSeconds, player, enemies, map);

		for (auto& enemy : enemies) {
			enemy.updateEnemy(player, elapsedSeconds, map, player.isStealthEnabled());
		}

		if (player.getHealth() <= 0) {
			break;
		}

		const auto newEnd = std::remove_if(
			enemies.begin(), enemies.end(),
			[](const Enemy& enemy) {
				return enemy.getHealth() <= 0;
			});
		//•std::remove_if	是把保留的元素前移，然后由 erase 删除尾部；不能保证“把死亡元素挪到末尾”。
		//newEnd,指向"保留下来的元素"之后、即"垃圾区"的起点。
		enemies.erase(newEnd, enemies.end());

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

		// Draw each covered cell once so overlapping areas keep the same opacity.

		std::vector<Position> burningCells = molotov.getBurningCells();//按值传递 因为后面不能动molotov里的数据


		// 先排序，再去重，避免同一格被重复绘制，导致半透明火焰颜色加深。
	//
	// std::sort(begin, end, 比较规则)：对 [begin, end) 范围内的元素排序。
	// begin() 指向起始元素，end() 指向末尾之后，不属于有效元素。
	//
	// [](const Position& left, const Position& right) { ... } 是 lambda 表达式：
	// []：不捕获外部变量。
	// const Position&：只读引用，比较时不复制、不修改位置。
	// 返回 true：表示 left 应排在 right 前面。
	// 比较规则：行号小的在前；行号相同时，列号小的在前。
	// 排序后，相同位置就会挨在一起。
	//
	// std::unique(begin, end)：使用 Position 的 operator== 判断相邻位置是否相同。
	// 它将不重复的元素整理到前部，返回“有效部分末尾之后”的迭代器。
	// 注意：unique 不会缩短 vector，尾部内容不应再使用。
	//
	// erase(unique(...), end())：先执行 unique，再删除其返回位置到原末尾的部分。
	// 相当于：
	// const auto newEnd = std::unique(burningCells.begin(), burningCells.end());
	// burningCells.erase(newEnd, burningCells.end());
		std::sort(burningCells.begin(), burningCells.end(),
			[](const Position& left, const Position& right) {
				return left.row < right.row ||
					(left.row == right.row && left.column < right.column);
			});
		burningCells.erase(std::unique(burningCells.begin(), burningCells.end()), burningCells.end());


		for (const Position cell : burningCells) {
			const Vector2 screenCenter{
				(cell.column + 0.5f) * cellSize,
				(cell.row + 0.5f) * cellSize
			};
			DrawCircleV(screenCenter, fireCircleRadiusPixels, fireColor);
		}


		if (isPreparingMolotovThrow || isPreparingDecoyMineThrow) {
			std::vector<Position> throwCells;
			if (isPreparingMolotovThrow) {
				throwCells = molotov.findValidThrowCells(player.getPosition(), map);
			}
			else {
				throwCells = decoyMine.findValidThrowCells(player.getPosition(), map);
			}
			const auto containsCell = [&throwCells](Position cell) {
				return std::find(throwCells.begin(), throwCells.end(), cell) != throwCells.end();
				};
			//参数	这里传的	含义
			//first	throwCells.begin()	搜索起点（含）
			//	last	throwCells.end()	搜索终点（不含）
			//	value	cell	要找的值
			//	返回值：找到就返回指向该元素的迭代器，找不到就返回 last（也就是 throwCells.end()）。
			//	所以 != throwCells.end() 就是“找到了”。整个表达式是“cell 在不在 throwCells 里”。
			// 
			// 捕获不是“从某行到某行”的文本区间，而是“lambda 定义点之前、在包围作用域里可见的变量”。
			// 
			// Shared edges are inside the range; only exposed edges form its outline.
			for (const Position cell : throwCells) {
				const float left = static_cast<float>(cell.column * cellSize);
				const float top = static_cast<float>(cell.row * cellSize);
				//行 列乘以格子大小得到是左上角坐标
				const float right = left + cellSize;
				const float bottom = top + cellSize;
				if (!containsCell({ cell.row - 1, cell.column })) {
					//上邻居不在时 
					DrawLineEx({ left, top }, { right, top }, throwPreviewLineThicknessPixels, throwPreviewColor);
				}
				if (!containsCell({ cell.row + 1, cell.column })) {
					//下邻居
					DrawLineEx({ left, bottom }, { right, bottom }, throwPreviewLineThicknessPixels, throwPreviewColor);
				}
				if (!containsCell({ cell.row, cell.column - 1 })) {
					DrawLineEx({ left, top }, { left, bottom }, throwPreviewLineThicknessPixels, throwPreviewColor);
				}
				if (!containsCell({ cell.row, cell.column + 1 })) {
					DrawLineEx({ right, top }, { right, bottom }, throwPreviewLineThicknessPixels, throwPreviewColor);
				}
			}
		}

		if (!isPreparingMolotovThrow && !isPreparingDecoyMineThrow && isMouseInsideWindow) {
			const Position playerPosition = player.getPosition();
			const GridPoint edgePosition = gun.calculateEdgePosition(playerPosition, aimPosition);
			const Vector2 rayStart{
				(playerPosition.column + 0.5f) * cellSize,
				(playerPosition.row + 0.5f) * cellSize
			};
			const Vector2 rayEnd{
				static_cast<float>(edgePosition.column * cellSize),
				static_cast<float>(edgePosition.row * cellSize)
			};
			// This is the maximum-range guide; terrain blocking belongs to hit detection.
			DrawLineEx(rayStart, rayEnd, aimLineThicknessPixels, aimLineColor);
		}

		DrawRectangle(player.getPosition().column * cellSize, player.getPosition().row * cellSize,
			cellSize, cellSize, player.isStealthEnabled() ? PURPLE : BLUE);

		for (const Position snapshotPosition : scanController.getEnemySnapshotPositions()) {
			DrawRectangle(snapshotPosition.column * cellSize, snapshotPosition.row * cellSize, cellSize, cellSize, YELLOW);
		}

		const int molotovCount = molotov.getRemainingCount();
		const int decoyMineCount = decoyMine.getRemainingCount();
		const auto decoyMineText = TextFormat("Decoy Mine: %d", decoyMineCount);
		const auto molotovText = TextFormat("Molotov: %d", molotovCount);
		DrawText(molotovText, 12, 12, 20, ORANGE);
		DrawText(decoyMineText, 12, 36, 20, SKYBLUE);
		const auto ammoText = TextFormat("Ammo: %d / %d%s",
			gun.getAmmoInMagazine(), gun.getReserveAmmo(),
			gun.isReloading() ? " (Reloading)" : "");
		DrawText(ammoText, 13, 61, 20, BLACK);
		DrawText(ammoText, 12, 60, 20, GOLD);

		EndDrawing();
	}
	CloseWindow();

	return 0;
}
