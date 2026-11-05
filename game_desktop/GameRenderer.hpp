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

struct RendererConfig {
	int cellSizePixels = 40;
	float fireRadiusInCells = 0.5f;
	Color fireColor = Fade(ORANGE, 0.55f);
	Color throwPreviewColor = Fade(SKYBLUE, 0.7f);
	float throwPreviewLineThicknessPixels = 2.0f;
	Color aimLineColor = Fade(RED, 0.75f);
	float aimLineThicknessPixels = 2.0f;
};

// Reads game state; owns only presentation settings.
class GameRenderer {
public:
	explicit GameRenderer(RendererConfig config);

	// The caller owns BeginDrawing/EndDrawing and the order of these layers.
	void drawWorld(const Map& map, const Molotov& molotov,
		const ResourceManager& resources) const;
	void drawThrowPreview(const std::vector<Position>& throwCells) const;
	void drawAimLine(Position playerPosition, GridPoint edgePosition) const;
	void drawActors(const Player& player, const std::vector<Position>& snapshots) const;
	void drawHud(const Molotov& molotov, const DecoyMine& decoyMine,
		const Gun& gun, const ResourceManager& resources) const;

private:
	void drawMap(const Map& map) const;
	void drawFire(const Molotov& molotov) const;
	void drawSupplies(const Map& map, const ResourceManager& resources) const;

	RendererConfig config_;
};
