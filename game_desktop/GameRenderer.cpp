#include "GameRenderer.hpp"
#include "World/Map.hpp"
#include "Actors/Player.hpp"
#include "Resources/ResourceManager.hpp"
#include "Weapons/Molotov.hpp"
#include "Weapons/DecoyMine.hpp"
#include "Weapons/Gun.hpp"
#include <algorithm>

GameRenderer::GameRenderer(RendererConfig config) : config_(config) {}

void GameRenderer::drawWorld(const Map& map, const Molotov& molotov,
	const ResourceManager& resources) const {
	drawMap(map);
	drawFire(molotov);
	drawSupplies(map, resources);
}

void GameRenderer::drawMap(const Map& map) const {
	const int cellSize = config_.cellSizePixels;
	const int maxColumns = map.maxColumnCount();
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
}

void GameRenderer::drawFire(const Molotov& molotov) const {
	const int cellSize = config_.cellSizePixels;
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
		DrawCircleV(screenCenter, cellSize * config_.fireRadiusInCells, config_.fireColor);
	}
}

void GameRenderer::drawSupplies(const Map& map, const ResourceManager& resources) const {
	const int cellSize = config_.cellSizePixels;
	const auto drawSupplyMarker = [&map, cellSize](Position position, const char* label, Color color) {
		if (!map.isScanned(position.row, position.column)) {
			return;
		}
		const int markerSize = cellSize / 2;
		const int markerLeft = position.column * cellSize + (cellSize - markerSize) / 2;
		const int markerTop = position.row * cellSize + (cellSize - markerSize) / 2;
		const int fontSize = markerSize - 4;
		DrawRectangle(markerLeft, markerTop, markerSize, markerSize, color);
		DrawText(label, markerLeft + (markerSize - MeasureText(label, fontSize)) / 2,
			markerTop + (markerSize - fontSize) / 2, fontSize, WHITE);
	};

	for (const MaterialPack& pack : resources.getMaterialPacks()) {
		drawSupplyMarker(pack.position, "M", DARKGREEN);
	}
	for (const AmmoPack& pack : resources.getAmmoPacks()) {
		drawSupplyMarker(pack.position, "A", DARKBLUE);
	}
}

void GameRenderer::drawThrowPreview(const std::vector<Position>& throwCells) const {
	const int cellSize = config_.cellSizePixels;
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
			DrawLineEx({ left, top }, { right, top }, config_.throwPreviewLineThicknessPixels, config_.throwPreviewColor);
		}
		if (!containsCell({ cell.row + 1, cell.column })) {
			//下邻居
			DrawLineEx({ left, bottom }, { right, bottom }, config_.throwPreviewLineThicknessPixels, config_.throwPreviewColor);
		}
		if (!containsCell({ cell.row, cell.column - 1 })) {
			DrawLineEx({ left, top }, { left, bottom }, config_.throwPreviewLineThicknessPixels, config_.throwPreviewColor);
		}
		if (!containsCell({ cell.row, cell.column + 1 })) {
			DrawLineEx({ right, top }, { right, bottom }, config_.throwPreviewLineThicknessPixels, config_.throwPreviewColor);
		}
	}
}

void GameRenderer::drawAimLine(Position playerPosition, GridPoint edgePosition) const {
	const int cellSize = config_.cellSizePixels;
	const Vector2 rayStart{
		(playerPosition.column + 0.5f) * cellSize,
		(playerPosition.row + 0.5f) * cellSize
	};
	const Vector2 rayEnd{
		static_cast<float>(edgePosition.column * cellSize),
		static_cast<float>(edgePosition.row * cellSize)
	};
	// This is the maximum-range guide; terrain blocking belongs to hit detection.
	DrawLineEx(rayStart, rayEnd, config_.aimLineThicknessPixels, config_.aimLineColor);
}

void GameRenderer::drawActors(const Player& player, const std::vector<Position>& snapshots) const {
	const int cellSize = config_.cellSizePixels;
	DrawRectangle(player.getPosition().column * cellSize, player.getPosition().row * cellSize,
		cellSize, cellSize, player.isStealthEnabled() ? PURPLE : BLUE);

	for (const Position snapshotPosition : snapshots) {
		DrawRectangle(snapshotPosition.column * cellSize, snapshotPosition.row * cellSize, cellSize, cellSize, YELLOW);
	}
}

void GameRenderer::drawHud(const Molotov& molotov, const DecoyMine& decoyMine,
	const Gun& gun, const ResourceManager& resources) const {
	const int molotovCount = molotov.getRemainingCount();
	const int decoyMineCount = decoyMine.getRemainingCount();
	const auto decoyMineText = TextFormat("Decoy Mine [2]: %d", decoyMineCount);
	const auto molotovText = TextFormat("Molotov [1]: %d", molotovCount);
	DrawText(molotovText, 12, 12, 20, ORANGE);
	DrawText(decoyMineText, 12, 36, 20, SKYBLUE);
	const auto ammoText = TextFormat("Ammo: %d / %d%s",
		gun.getAmmoInMagazine(), gun.getReserveAmmo(),
		gun.isReloading() ? " (Reloading)" : "");
	DrawText(ammoText, 13, 61, 20, BLACK);
	DrawText(ammoText, 12, 60, 20, GOLD);
	const char* materialsText = TextFormat("Materials: %d", resources.getRemainingMaterials());
	DrawText(materialsText, 13, 85, 20, BLACK);
	DrawText(materialsText, 12, 84, 20, LIME);
}

