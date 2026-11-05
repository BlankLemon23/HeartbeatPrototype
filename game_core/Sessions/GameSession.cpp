#include "GameSession.hpp"
#include <algorithm>

namespace {
	constexpr double initialScanCooldownSeconds = 0.0;
	constexpr double scanRadiusCells = 5.0;
	constexpr double enemySnapshotDurationSeconds = 0.5;
	constexpr double manualScanIntervalSeconds = 3.0;
	constexpr BattleScanConfig battleScanConfig{};
}

GameSession::GameSession()
	: player_({1, 1}, PlayerConfig{}),
	  scanController_(initialScanCooldownSeconds, scanRadiusCells, enemySnapshotDurationSeconds),
	  molotov_(MolotovConfig{}),
	  decoyMine_(DecoyMineConfig{}),
	  gun_(GunConfig{}),
	  resourceManager_(
		std::vector<AmmoPack>{{6, {1, 5}}, {6, {10, 24}}},
		std::vector<MaterialPack>{
			{2, {1, 4}}, {2, {4, 3}}, {2, {8, 3}},
			{2, {10, 5}}, {2, {13, 12}}, {2, {18, 20}}
		},
		ResourceManagerConfig{}) {
	map_.setMap({
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

	const EnemyConfig enemyConfig{};
	enemies_.emplace_back(Position{1, 11}, enemyConfig);
	enemies_.emplace_back(Position{1, 2}, enemyConfig);
}

bool GameSession::update(double elapsedSeconds, const SessionInput& input) {
	// Advance timers once per frame.
	scanController_.updateScanTime(elapsedSeconds);
	scanController_.updateSnapshotRemainingSeconds(elapsedSeconds);
	player_.updateMoveCooldown(elapsedSeconds);
	gun_.update(elapsedSeconds);

	// Movement and automatic pickup.
	if (input.toggleStealth) {
		player_.toggleStealth();
	}
	if (input.moveDirection) {
		player_.tryMove(*input.moveDirection, map_);
	}
	resourceManager_.collectSupplies(player_, gun_);

	// Weapon actions.
	if (input.reload) {
		gun_.tryReload();
	}
	if (input.toggleMolotov) {
		isPreparingMolotovThrow_ = !isPreparingMolotovThrow_;
		isPreparingDecoyMineThrow_ = false;
	}
	if (input.toggleDecoyMine) {
		isPreparingMolotovThrow_ = false;
		isPreparingDecoyMineThrow_ = !isPreparingDecoyMineThrow_;
	}

	if (input.weaponTarget) {
		const GridPoint target = *input.weaponTarget;
		const Position playerPosition = player_.getPosition();
		if (isPreparingThrow()) {
			const Position clickedPosition{
				static_cast<int>(target.row), static_cast<int>(target.column)
			};
			const bool throwSucceeded = isPreparingMolotovThrow_
				? molotov_.tryThrow(playerPosition, clickedPosition, map_)
				: decoyMine_.tryThrow(playerPosition, clickedPosition, map_, enemies_);
			if (throwSucceeded) {
				isPreparingMolotovThrow_ = false;
				isPreparingDecoyMineThrow_ = false;
			}
		}
		else {
			gun_.tryFire(playerPosition, target, map_, enemies_);
		}
	}

	// Resolve fire and enemy actions before removing dead enemies.
	molotov_.updateBurningArea(elapsedSeconds, player_, enemies_, map_);

	for (auto& enemy : enemies_) {
		enemy.updateEnemy(player_, elapsedSeconds, map_, player_.isStealthEnabled());
	}

	// Keep the current death-to-exit behavior until result states are wired in.
	if (player_.getHealth() <= 0) {
		return false;
	}

	const auto newEnd = std::remove_if(
		enemies_.begin(), enemies_.end(),
		[](const Enemy& enemy) {
			return enemy.getHealth() <= 0;
		});
	//•std::remove_if	是把保留的元素前移，然后由 erase 删除尾部；不能保证“把死亡元素挪到末尾”。
	//newEnd,指向"保留下来的元素"之后、即"垃圾区"的起点。
	enemies_.erase(newEnd, enemies_.end());

	// Refresh battle state before crafting and scanning.
	const bool allNotChasing = std::all_of(
		enemies_.begin(), enemies_.end(),
		[](const Enemy& e) { return !e.isChasing(); }
	);

	//语法：•	[] —— 捕获列表,空表示不捕获任何外部变量
	//•(const Enemy& e) —— 参数, all_of 会依次把每个敌人传进来
	//•{ return !e.isChasing(); } —— 函数体, 返回"该敌人没有在追"这个布尔值
	//对 enemies 里的每个敌人 e,判断 !e.isChasing();如果所有敌人都满足(即都不在追),allNotChasing 就是 true。
	if (!allNotChasing) {
		player_.setInBattle(true);
		player_.resetOutBattleRemainingSeconds();
	}
	else if (player_.isInBattle()) {
		player_.updateOutBattleRemainingSeconds(elapsedSeconds);
		if (player_.getOutBattleRemainingSeconds() <= 0.0) {
			player_.setInBattle(false);
		}
	}

	// Crafting and scanning.
	if (input.craftMolotov) {
		resourceManager_.craftMolotov(molotov_, player_);
	}
	if (input.craftDecoyMine) {
		resourceManager_.craftDecoyMine(decoyMine_, player_);
	}

	if (input.scan) {
		scanController_.tryScanManually(player_, map_, manualScanIntervalSeconds, enemies_);
	}

	scanController_.updateBattleAutoScan(player_, map_, battleScanConfig, enemies_);

	return true;
}

bool GameSession::isPreparingThrow() const {
	return isPreparingMolotovThrow_ || isPreparingDecoyMineThrow_;
}

std::vector<Position> GameSession::getThrowPreviewCells() const {
	if (isPreparingMolotovThrow_) {
		return molotov_.findValidThrowCells(player_.getPosition(), map_);
	}
	if (isPreparingDecoyMineThrow_) {
		return decoyMine_.findValidThrowCells(player_.getPosition(), map_);
	}
	return {};
}

const Map& GameSession::getMap() const {
	return map_;
}

const Player& GameSession::getPlayer() const {
	return player_;
}

const Molotov& GameSession::getMolotov() const {
	return molotov_;
}

const DecoyMine& GameSession::getDecoyMine() const {
	return decoyMine_;
}

const Gun& GameSession::getGun() const {
	return gun_;
}

const ResourceManager& GameSession::getResourceManager() const {
	return resourceManager_;
}

const std::vector<Position>& GameSession::getEnemySnapshotPositions() const {
	return scanController_.getEnemySnapshotPositions();
}
