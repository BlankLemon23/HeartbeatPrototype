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

enum class GameState {
	Playing,
	Victory,
	Defeat
};

enum class ActionFeedback {
	None,
	ScanCoolingDown,
	ScanAutomaticInBattle,
	CraftInBattle,
	CraftNotEnoughMaterials,
	CraftCarryLimitReached
};

struct SessionInput {
	std::optional<Direction> moveDirection;
	std::optional<GridPoint> weaponTarget;
	bool toggleStealth = false;
	bool reload = false;
	bool toggleMolotov = false;
	bool toggleDecoyMine = false;
	bool craftMolotov = false;
	bool craftDecoyMine = false;
	bool scan = false;
};

struct EnemySpawnConfig {
	Position position;
	EnemyConfig config;
};

struct GameSessionConfig {
	GameState initialState;
	MapConfig mapConfig;
	std::vector<EnemySpawnConfig> enemySpawns;
	double initialScanCooldownSeconds;
	double scanRadiusCells;
	double enemySnapshotDurationSeconds;
	double manualScanIntervalSeconds;
	BattleScanConfig battleScan;
	Position playerStart;
	PlayerConfig player;
	GunConfig gun;
	MolotovConfig molotov;
	DecoyMineConfig decoyMine;
	ResourceManagerConfig crafting;
	std::vector<AmmoPack> ammoPacks;
	std::vector<MaterialPack> materialPacks;
};

class GameSession {
public:
	explicit GameSession(GameSessionConfig config);
	GameState update(double elapsedSeconds, const SessionInput& input);

	bool isPreparingThrow() const;
	std::vector<Position> getThrowPreviewCells() const;
	int getStealthDetectionRadiusCells() const;
	const Map& getMap() const;
	const Player& getPlayer() const;
	const Molotov& getMolotov() const;
	const DecoyMine& getDecoyMine() const;
	const Gun& getGun() const;
	const ResourceManager& getResourceManager() const;
	ActionFeedback getActionFeedback() const;
	const std::vector<Position>& getEnemySnapshotPositions() const;

private:
	GameState state_;
	GameSessionConfig config_;
	Map map_;
	Player player_;
	std::vector<Enemy> enemies_;
	ScanController scanController_;
	Molotov molotov_;
	DecoyMine decoyMine_;
	Gun gun_;
	ResourceManager resourceManager_;
	ActionFeedback actionFeedback_ = ActionFeedback::None;
	double actionFeedbackRemainingSeconds_ = 0.0;
	bool isPreparingMolotovThrow_ = false;
	bool isPreparingDecoyMineThrow_ = false;
};
