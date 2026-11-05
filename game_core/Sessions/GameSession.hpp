#pragma once

#include "Actors/Enemy.hpp"
#include "Actors/Player.hpp"
#include "Perception/ScanController.hpp"
#include "Resources/ResourceManager.hpp"
#include "Weapons/DecoyMine.hpp"
#include "Weapons/Gun.hpp"
#include "Weapons/Molotov.hpp"
#include "World/Map.hpp"
#include "World/Position.hpp"
#include <optional>
#include <vector>

enum GameState {
	Playing,
	Victory,
	Defeat
};

// One frame of game actions, expressed without desktop key or pixel coordinates.
struct SessionInput {
	std::optional<Direction> moveDirection;
	std::optional<GridPoint> weaponTarget; // Set on a weapon click inside the play area.
	bool toggleStealth = false;
	bool reload = false;
	bool toggleMolotov = false;
	bool toggleDecoyMine = false;
	bool craftMolotov = false;
	bool craftDecoyMine = false;
	bool scan = false;
};

class GameSession {
public:
	GameSession();
	// Returns false if the player died and the current loop should exit.
	bool update(double elapsedSeconds, const SessionInput& input);

	bool isPreparingThrow() const;
	std::vector<Position> getThrowPreviewCells() const;
	const Map& getMap() const;
	const Player& getPlayer() const;
	const Molotov& getMolotov() const;
	const DecoyMine& getDecoyMine() const;
	const Gun& getGun() const;
	const ResourceManager& getResourceManager() const;
	const std::vector<Position>& getEnemySnapshotPositions() const;

private:
	Map map_;
	Player player_;
	std::vector<Enemy> enemies_;
	ScanController scanController_;
	Molotov molotov_;
	DecoyMine decoyMine_;
	Gun gun_;
	ResourceManager resourceManager_;
	bool isPreparingMolotovThrow_ = false;
	bool isPreparingDecoyMineThrow_ = false;
};
