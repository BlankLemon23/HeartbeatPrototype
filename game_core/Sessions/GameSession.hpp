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

enum class ActionFeedback {
	None,
	ScanCoolingDown,
	ScanAutomaticInBattle,
	CraftInBattle,
	CraftNotEnoughMaterials,
	CraftCarryLimitReached
};

// One frame of game actions, expressed without desktop key or pixel coordinates.
struct SessionInput {
	std::optional<Direction> moveDirection;
	//std::optional<T> 表示这里可能有一个 T 类型的值，也可能没有值
	std::optional<GridPoint> weaponTarget; // Set on a weapon click inside the play area.
	bool toggleStealth = false;
	bool reload = false;
	bool toggleMolotov = false;
	bool toggleDecoyMine = false;
	bool craftMolotov = false;
	bool craftDecoyMine = false;
	bool scan = false;
};

struct EnemySpawnConfig {
	Position position{};
	EnemyConfig config{};
};

struct GameSessionConfig {
	GameState initialState = GameState::Playing;
	MapConfig mapConfig;
	std::vector<EnemySpawnConfig> enemySpawns;
	double initialScanCooldownSeconds = 0.0;
	double scanRadiusCells = 5.0;
	double enemySnapshotDurationSeconds = 0.5;
	double manualScanIntervalSeconds = 3.0;
	BattleScanConfig battleScan{};
	Position playerStart{1, 1};
	PlayerConfig player{};
	GunConfig gun{};
	MolotovConfig molotov{};
	DecoyMineConfig decoyMine{};
	ResourceManagerConfig crafting{};
	std::vector<AmmoPack> ammoPacks{{6, {1, 5}}, {6, {10, 24}}};
	std::vector<MaterialPack> materialPacks{
		{2, {1, 4}}, {2, {4, 3}}, {2, {8, 3}},
		{2, {10, 5}}, {2, {13, 12}}, {2, {18, 20}}
	};
};

class GameSession {
public:
	explicit GameSession(GameSessionConfig config);
	// Returns false if the player died and the current loop should exit.
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
