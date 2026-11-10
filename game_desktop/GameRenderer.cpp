#include "GameRenderer.hpp"
#include "World/Map.hpp"
#include "Actors/Player.hpp"
#include "Resources/ResourceManager.hpp"
#include "Sessions/GameSession.hpp"
#include "Weapons/Molotov.hpp"
#include "Weapons/DecoyMine.hpp"
#include "Weapons/Gun.hpp"
#include <algorithm>

GameRenderer::GameRenderer(RendererConfig config) : config_(config) {
}

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

	std::vector<Position> burningCells = molotov.getBurningCells();

	std::sort(burningCells.begin(), burningCells.end(),
		[](const Position& left, const Position& right) {
			return left.row < right.row ||
				(left.row == right.row && left.column < right.column);
		});
	burningCells.erase(std::unique(burningCells.begin(), burningCells.end()), burningCells.end());

	for (const Position cell : burningCells) {
		const Vector2 screenCenter{
			(cell.column + 0.5f) * cellSize,
			(cell.row + 0.5f) * cellSize};
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
	drawRangeOutline(throwCells);
}

void GameRenderer::drawStealthDetectionRange(Position center, int radiusCells) const {
	const int screenRows = GetScreenHeight() / config_.cellSizePixels;
	const int screenColumns = GetScreenWidth() / config_.cellSizePixels;
	const int rowRadius = std::min(radiusCells, screenRows);
	const int columnRadius = std::min(radiusCells, screenColumns);
	const double radiusSquared = static_cast<double>(radiusCells) * radiusCells;
	std::vector<Position> cells;

	// Include one off-screen cell so the window edge does not form a false outline.
	for (int row = std::max(-1, center.row - rowRadius);
		row <= std::min(screenRows, center.row + rowRadius); ++row) {
		for (int column = std::max(-1, center.column - columnRadius);
			column <= std::min(screenColumns, center.column + columnRadius); ++column) {
			const double rowDistance = static_cast<double>(row) - center.row;
			const double columnDistance = static_cast<double>(column) - center.column;
			if (rowDistance * rowDistance + columnDistance * columnDistance <= radiusSquared) {
				cells.push_back({row, column});
			}
		}
	}
	drawRangeOutline(cells);
}

void GameRenderer::drawRangeOutline(const std::vector<Position>& cells) const {
	const int cellSize = config_.cellSizePixels;
	const auto containsCell = [&cells](Position cell) {
		return std::find(cells.begin(), cells.end(), cell) != cells.end();
	};
	// Shared edges are inside the range; only exposed edges form its outline.
	for (const Position cell : cells) {
		const float left = static_cast<float>(cell.column * cellSize);
		const float top = static_cast<float>(cell.row * cellSize);
		const float right = left + cellSize;
		const float bottom = top + cellSize;
		if (!containsCell({cell.row - 1, cell.column})) {
			DrawLineEx({left, top}, {right, top}, config_.throwPreviewLineThicknessPixels, config_.throwPreviewColor);
		}
		if (!containsCell({cell.row + 1, cell.column})) {
			DrawLineEx({left, bottom}, {right, bottom}, config_.throwPreviewLineThicknessPixels, config_.throwPreviewColor);
		}
		if (!containsCell({cell.row, cell.column - 1})) {
			DrawLineEx({left, top}, {left, bottom}, config_.throwPreviewLineThicknessPixels, config_.throwPreviewColor);
		}
		if (!containsCell({cell.row, cell.column + 1})) {
			DrawLineEx({right, top}, {right, bottom}, config_.throwPreviewLineThicknessPixels, config_.throwPreviewColor);
		}
	}
}

void GameRenderer::drawAimLine(Position playerPosition, GridPoint edgePosition) const {
	const int cellSize = config_.cellSizePixels;
	const Vector2 rayStart{
		(playerPosition.column + 0.5f) * cellSize,
		(playerPosition.row + 0.5f) * cellSize};
	const Vector2 rayEnd{
		static_cast<float>(edgePosition.column * cellSize),
		static_cast<float>(edgePosition.row * cellSize)};
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

void GameRenderer::drawHud(const Player& player, const Molotov& molotov, const DecoyMine& decoyMine,
	const Gun& gun, const ResourceManager& resources, ActionFeedback feedback) const {
	const char* hpText = TextFormat("HP: %d / %d", player.getHealth(), player.getMaxHealth());
	DrawText(hpText, 13, 13, 20, BLACK);
	DrawText(hpText, 12, 12, 20, WHITE);

	const bool inBattle = player.isInBattle();
	const char* battleText = inBattle ? "IN COMBAT" : "EXPLORING";
	DrawText(battleText, 13, 37, 20, BLACK);
	DrawText(battleText, 12, 36, 20, inBattle ? RED : WHITE);

	DrawText(TextFormat("Molotov [1]: %d", molotov.getRemainingCount()), 12, 60, 20, ORANGE);
	DrawText(TextFormat("Decoy Mine [2]: %d", decoyMine.getRemainingCount()), 12, 84, 20, SKYBLUE);

	const char* ammoText = TextFormat("Ammo: %d / %d%s",
		gun.getAmmoInMagazine(), gun.getReserveAmmo(),
		gun.isReloading() ? " (Reloading)" : "");
	DrawText(ammoText, 13, 109, 20, BLACK);
	DrawText(ammoText, 12, 108, 20, GOLD);

	const char* materialsText = TextFormat("Materials: %d", resources.getRemainingMaterials());
	DrawText(materialsText, 13, 133, 20, BLACK);
	DrawText(materialsText, 12, 132, 20, LIME);
	drawActionFeedback(feedback);
}

void GameRenderer::drawActionFeedback(ActionFeedback feedback) const {
	const char* message = nullptr;
	switch (feedback) {
	case ActionFeedback::None:
		return;
	case ActionFeedback::ScanCoolingDown:
		message = "Scan is cooling down";
		break;
	case ActionFeedback::ScanAutomaticInBattle:
		message = "Scan is automatic in combat";
		break;
	case ActionFeedback::CraftInBattle:
		message = "Crafting is blocked in combat";
		break;
	case ActionFeedback::CraftNotEnoughMaterials:
		message = "Not enough crafting materials";
		break;
	case ActionFeedback::CraftCarryLimitReached:
		message = "Crafting inventory is full";
		break;
	}
	if (!message) {
		return;
	}

	constexpr float preferredSize = 20.0f;
	const float width = static_cast<float>(GetScreenWidth());
	const float height = static_cast<float>(GetScreenHeight());
	const float padding = std::min(8.0f, std::min(width, height) / 8.0f);
	const Font font = GetFontDefault();
	const float preferredWidth = MeasureTextEx(font, message, preferredSize, preferredSize / 12.0f).x;
	const float fontSize = std::min({preferredSize, preferredSize * (width - 4.0f * padding) / preferredWidth,
		height - 4.0f * padding});
	const float spacing = fontSize / 12.0f;
	const Vector2 textSize = MeasureTextEx(font, message, fontSize, spacing);
	const Rectangle background{
		(width - textSize.x) / 2.0f - padding,
		height - textSize.y - 3.0f * padding,
		textSize.x + 2.0f * padding,
		textSize.y + 2.0f * padding};
	DrawRectangleRec(background, Fade(BLACK, 0.85f));
	DrawTextEx(font, message, {background.x + padding, background.y + padding}, fontSize, spacing, WHITE);
}

ResultButtons GameRenderer::getResultButtons(int screenWidth, int screenHeight) const {
	const float width = static_cast<float>(screenWidth);
	const float height = static_cast<float>(screenHeight);
	const float buttonWidth = std::min(160.0f, width * 0.38f);
	const float buttonHeight = std::min(48.0f, height * 0.2f);
	const float gap = std::min(24.0f, width * 0.06f);
	const float left = (width - 2.0f * buttonWidth - gap) / 2.0f;
	const float top = height * 0.62f;

	return {{left, top, buttonWidth, buttonHeight},
		{left + buttonWidth + gap, top, buttonWidth, buttonHeight}};
}

void GameRenderer::drawVictoryInterface(const ResultButtons& buttons) const {
	drawResultInterface("Victory", WHITE, GOLD, buttons);
}

void GameRenderer::drawDefeatInterface(const ResultButtons& buttons) const {
	drawResultInterface("YOU DIED", BLACK, RED, buttons);
}

void GameRenderer::drawResultInterface(const char* title, Color background, Color accent,
	const ResultButtons& buttons) const {
	ClearBackground(background);

	const Font font = GetFontDefault();
	const auto drawCenteredText = [&font](const char* text, Rectangle area, float fontSize, Color color) {
		const float spacing = fontSize / 12.0f;
		const Vector2 textSize = MeasureTextEx(font, text, fontSize, spacing);
		DrawTextEx(font, text,
			{area.x + (area.width - textSize.x) / 2.0f,
				area.y + (area.height - textSize.y) / 2.0f},
			fontSize, spacing, color);
	};

	const float width = static_cast<float>(GetScreenWidth());
	const float height = static_cast<float>(GetScreenHeight());
	const float titleSize = std::min({64.0f, width * 0.15f, height * 0.18f});
	drawCenteredText(title, {0.0f, 0.0f, width, height}, titleSize, accent);

	const auto drawButton = [&drawCenteredText, accent](Rectangle bounds, const char* label) {
		DrawRectangleRec(bounds, RAYWHITE);
		DrawRectangleLinesEx(bounds, 2.0f, accent);
		const float labelSize = std::min({22.0f, bounds.width * 0.2f, bounds.height * 0.5f});
		drawCenteredText(label, bounds, labelSize, DARKGRAY);
	};

	drawButton(buttons.restart, "Restart");
	drawButton(buttons.quit, "Quit");
}
