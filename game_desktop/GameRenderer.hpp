#pragma once
#include "raylib.h"
#include "World/Position.hpp"
#include <vector>

class Map;
class Player;
class Molotov;
class DecoyMine;
class Gun;
class ResourceManager;
enum class ActionFeedback;

struct RendererConfig {
	int cellSizePixels;
	float fireRadiusInCells;
	Color fireColor;
	Color throwPreviewColor;
	float throwPreviewLineThicknessPixels;
	Color aimLineColor;
	float aimLineThicknessPixels;
};

struct ResultButtons {
	Rectangle restart;
	Rectangle quit;
};

// Reads game state; owns only presentation settings.
class GameRenderer {
public:
	explicit GameRenderer(RendererConfig config);

	// The caller owns BeginDrawing/EndDrawing and the order of these layers.
	void drawWorld(const Map& map, const Molotov& molotov,
		const ResourceManager& resources) const;
	void drawThrowPreview(const std::vector<Position>& throwCells) const;
	void drawStealthDetectionRange(Position center, int radiusCells) const;
	void drawAimLine(Position playerPosition, GridPoint edgePosition) const;
	void drawActors(const Player& player, const std::vector<Position>& snapshots) const;
	void drawHud(const Player& player, const Molotov& molotov, const DecoyMine& decoyMine,
		const Gun& gun, const ResourceManager& resources, ActionFeedback feedback) const;
	ResultButtons getResultButtons(int screenWidth, int screenHeight) const;
	void drawVictoryInterface(const ResultButtons& buttons) const;
	void drawDefeatInterface(const ResultButtons& buttons) const;

private:
	void drawMap(const Map& map) const;
	void drawFire(const Molotov& molotov) const;
	void drawSupplies(const Map& map, const ResourceManager& resources) const;
	void drawRangeOutline(const std::vector<Position>& cells) const;
	void drawActionFeedback(ActionFeedback feedback) const;
	void drawResultInterface(const char* title, Color background, Color accent,
		const ResultButtons& buttons) const;

	RendererConfig config_;
};
