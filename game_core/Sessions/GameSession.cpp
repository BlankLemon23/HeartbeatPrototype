#include "GameSession.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

GameSession::GameSession(GameSessionConfig config)
	: state_(config.initialState),
	  config_(std::move(config)),
	  map_(config_.mapConfig),
	  player_(config_.playerStart, config_.player),
	  scanController_(config_.initialScanCooldownSeconds, config_.scanRadiusCells,
		  config_.enemySnapshotDurationSeconds),
	  molotov_(config_.molotov),
	  decoyMine_(config_.decoyMine),
	  gun_(config_.gun),
	  resourceManager_(config_.ammoPacks, config_.materialPacks, config_.crafting) {
	const Position playerStart = player_.getPosition();

	if (map_.classifyCell(playerStart.row, playerStart.column) != CellType::Space) {
		throw std::invalid_argument("Player spawn at row " + std::to_string(playerStart.row) +
			", column " + std::to_string(playerStart.column) + " must be on a floor cell.");
	}

	enemies_.reserve(config_.enemySpawns.size());
	for (const EnemySpawnConfig& spawn : config_.enemySpawns) {
		if (map_.classifyCell(spawn.position.row, spawn.position.column) != CellType::Space) {
			throw std::invalid_argument("Enemy spawn at row " + std::to_string(spawn.position.row) +
				", column " + std::to_string(spawn.position.column) + " must be on a floor cell.");
		}

		if (spawn.config.startingHealth <= 0) {
			throw std::invalid_argument("Enemy starting health must be positive.");
		}

		enemies_.emplace_back(spawn.position, spawn.config);
	}
}

static ActionFeedback feedbackForCraft(CraftResult result) {
	switch (result) {
	case CraftResult::InBattle:
		return ActionFeedback::CraftInBattle;
	case CraftResult::NotEnoughMaterials:
		return ActionFeedback::CraftNotEnoughMaterials;
	case CraftResult::CarryLimitReached:
		return ActionFeedback::CraftCarryLimitReached;
	case CraftResult::Success:
		return ActionFeedback::None;
	}
	return ActionFeedback::None;
}

// Advances a playing session by one frame and returns its current state.
// Once the session ends, subsequent calls return the stored result.
GameState GameSession::update(double elapsedSeconds, const SessionInput& input) {
	if (state_ == GameState::Defeat) {
		return state_;
	}
	else if (state_ == GameState::Victory) {
		return state_;
	}
	// Advance timers once per frame.
	actionFeedbackRemainingSeconds_ = std::max(0.0, actionFeedbackRemainingSeconds_ - elapsedSeconds);
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
				static_cast<int>(target.row), static_cast<int>(target.column)};
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

	if (player_.getHealth() <= 0) {
		state_ = GameState::Defeat;
		return state_;
	}

	const auto newEnd = std::remove_if(
		enemies_.begin(), enemies_.end(),
		[](const Enemy& enemy) {
			return enemy.getHealth() <= 0;
		});
	enemies_.erase(newEnd, enemies_.end());

	if (enemies_.empty()) {
		state_ = GameState::Victory;
		return state_;
	}

	// Refresh battle state before crafting and scanning.
	const bool allNotChasing = std::all_of(
		enemies_.begin(), enemies_.end(),
		[](const Enemy& e) { return !e.isChasing(); });

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

	// Keep the last failure when several actions are attempted in one frame.
	ActionFeedback frameFeedback = ActionFeedback::None;
	if (input.craftMolotov) {
		frameFeedback = feedbackForCraft(resourceManager_.craftMolotov(molotov_, player_));
	}
	if (input.craftDecoyMine) {
		const auto feedback = feedbackForCraft(resourceManager_.craftDecoyMine(decoyMine_, player_));
		if (feedback != ActionFeedback::None) {
			frameFeedback = feedback;
		}
	}

	if (input.scan &&
		!scanController_.tryScanManually(player_, map_, config_.manualScanIntervalSeconds, enemies_)) {
		frameFeedback = player_.isInBattle() ? ActionFeedback::ScanAutomaticInBattle
											 : ActionFeedback::ScanCoolingDown;
	}

	if (input.craftMolotov || input.craftDecoyMine || input.scan) {
		constexpr double feedbackDurationSeconds = 2.5;
		actionFeedback_ = frameFeedback;
		actionFeedbackRemainingSeconds_ = feedbackDurationSeconds;
	}

	scanController_.updateBattleAutoScan(player_, map_, config_.battleScan, enemies_);

	return state_;
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

int GameSession::getStealthDetectionRadiusCells() const {
	int radiusCells = 0;
	for (const EnemySpawnConfig& spawn : config_.enemySpawns) {
		radiusCells = std::max(radiusCells, spawn.config.stealthDetectionRadiusCells);
	}
	return radiusCells;
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

ActionFeedback GameSession::getActionFeedback() const {
	return actionFeedbackRemainingSeconds_ > 0.0 ? actionFeedback_ : ActionFeedback::None;
}

const std::vector<Position>& GameSession::getEnemySnapshotPositions() const {
	return scanController_.getEnemySnapshotPositions();
}
