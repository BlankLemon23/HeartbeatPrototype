#pragma once
#include "World/Position.hpp"
#include <vector>
#include "Actors/Player.hpp"
#include "Weapons/Gun.hpp"
#include "Weapons/DecoyMine.hpp"
#include "Weapons/Molotov.hpp"

struct ResourceManagerConfig {
	int molotovMaterialCost = 3;
	int decoyMineMaterialCost = 2;
};


struct MaterialPack
{
	int amount = 2;
	Position position;
};


struct AmmoPack{
	int amount = 6;
	Position position;
};




class ResourceManager {
public:
	void craftMolotov(Molotov& molotov,Player player);
	void craftDecoyMine(DecoyMine& decoyMine,Player player);
	void collectSupplies(Player& player, Gun& gun);
	ResourceManager(std::vector<AmmoPack> ammoPacks, std::vector<MaterialPack> materialPacks, ResourceManagerConfig config);

	const std::vector<AmmoPack>& getAmmoPacks() const;
	const std::vector<MaterialPack>& getMaterialPacks() const;
	int getRemainingMaterials() const;

private:
	void addMaterials(MaterialPack materialPack);
	ResourceManagerConfig config_;
	int remainingMaterials_ = 0;
	std::vector<AmmoPack> ammoPacks_;
	std::vector<MaterialPack> materialPacks_;
};