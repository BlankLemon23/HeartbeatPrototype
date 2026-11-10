#pragma once
#include "World/Position.hpp"
#include <vector>
#include "Actors/Player.hpp"
#include "Weapons/Gun.hpp"
#include "Weapons/DecoyMine.hpp"
#include "Weapons/Molotov.hpp"

struct ResourceManagerConfig {
	int molotovMaterialCost;
	int decoyMineMaterialCost;
};

struct MaterialPack {
	int amount;
	Position position;
};

struct AmmoPack {
	int amount;
	Position position;
};

enum class CraftResult {
	Success,
	InBattle,
	NotEnoughMaterials,
	CarryLimitReached
};

class ResourceManager {
public:
	CraftResult craftMolotov(Molotov& molotov, const Player& player);
	CraftResult craftDecoyMine(DecoyMine& decoyMine, const Player& player);
	void collectSupplies(Player& player, Gun& gun);
	ResourceManager(std::vector<AmmoPack> ammoPacks, std::vector<MaterialPack> materialPacks, ResourceManagerConfig config);

	const std::vector<AmmoPack>& getAmmoPacks() const;
	const std::vector<MaterialPack>& getMaterialPacks() const;
	int getRemainingMaterials() const;

private:
	CraftResult checkCraft(int materialCost, int count, int carryLimit, const Player& player) const;
	void addMaterials(MaterialPack materialPack);
	ResourceManagerConfig config_;
	int remainingMaterials_ = 0;
	std::vector<AmmoPack> ammoPacks_;
	std::vector<MaterialPack> materialPacks_;
};