#include "ResourceManager.hpp"
#include <utility>

CraftResult ResourceManager::checkCraft(int materialCost, int count, int carryLimit,
	const Player& player) const {
	if (player.isInBattle()) {
		return CraftResult::InBattle;
	}
	if (remainingMaterials_ < materialCost) {
		return CraftResult::NotEnoughMaterials;
	}
	if (count >= carryLimit) {
		return CraftResult::CarryLimitReached;
	}
	return CraftResult::Success;
}

CraftResult ResourceManager::craftMolotov(Molotov& molotov, const Player& player) {
	const CraftResult result = checkCraft(config_.molotovMaterialCost,
		molotov.getRemainingCount(), molotov.getCarryLimit(), player);
	if (result == CraftResult::Success) {
		molotov.addMolotov(1);
		remainingMaterials_ -= config_.molotovMaterialCost;
	}
	return result;
}

CraftResult ResourceManager::craftDecoyMine(DecoyMine& decoyMine, const Player& player) {
	const CraftResult result = checkCraft(config_.decoyMineMaterialCost,
		decoyMine.getRemainingCount(), decoyMine.getCarryLimit(), player);
	if (result == CraftResult::Success) {
		decoyMine.addDecoyMine(1);
		remainingMaterials_ -= config_.decoyMineMaterialCost;
	}
	return result;
}

void ResourceManager::collectSupplies(Player& player, Gun& gun) {
	const Position playerPosition = player.getPosition();
	for (auto packIt = ammoPacks_.begin(); packIt != ammoPacks_.end();) {
		if (packIt->position == playerPosition && gun.addAmmo(packIt->amount) > 0) {
			packIt = ammoPacks_.erase(packIt);
		}
		else {
			++packIt;
		}
	}

	for (auto packIt = materialPacks_.begin(); packIt != materialPacks_.end();) {
		if (packIt->position == playerPosition) {
			addMaterials(*packIt);
			packIt = materialPacks_.erase(packIt);
		}
		else {
			++packIt;
		}
	}
}

ResourceManager::ResourceManager(std::vector<AmmoPack> ammoPacks, std::vector<MaterialPack> materialPacks, ResourceManagerConfig config)
	: config_(config),
	  ammoPacks_(std::move(ammoPacks)),
	  materialPacks_(std::move(materialPacks)) {
}

const std::vector<AmmoPack>& ResourceManager::getAmmoPacks() const {
	return ammoPacks_;
}

const std::vector<MaterialPack>& ResourceManager::getMaterialPacks() const {
	return materialPacks_;
}

int ResourceManager::getRemainingMaterials() const {
	return remainingMaterials_;
}

void ResourceManager::addMaterials(MaterialPack materialPack) {
	remainingMaterials_ += materialPack.amount;
}
