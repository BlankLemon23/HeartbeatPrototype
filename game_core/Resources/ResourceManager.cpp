#include "ResourceManager.hpp"
#include <utility>

void ResourceManager::craftMolotov(Molotov& molotov,Player player)
{
	if (remainingMaterials_ >= config_.molotovMaterialCost
		&&
		molotov.getRemainingCount() < molotov.getCarryLimit()
		&&
		!player.isInBattle()) {
		molotov.addMolotov(1);
		remainingMaterials_ -= config_.molotovMaterialCost;
		
	}
}

void ResourceManager::craftDecoyMine(DecoyMine& decoyMine,Player player)
{
	if (remainingMaterials_ >= config_.decoyMineMaterialCost
		&&
		decoyMine.getRemainingCount() < decoyMine.getCarryLimit()
		&&
		!player.isInBattle()) {
		decoyMine.addDecoyMine(1);
		remainingMaterials_ -= config_.decoyMineMaterialCost;
	}
}

void ResourceManager::collectSupplies(Player& player, Gun& gun)
{
	const Position playerPosition = player.getPosition();
	//packit 资源包迭代器 pack iterator
	// 
	//这里for的第三部分 递增并没有写 由循环体决定怎样更新packIt
	// 
	//packIt -> 等同于(*packIt).
	//在你的代码里，`packIt` 的类型是 std::vector<MaterialPack>::iterator，也就是迭代器。
	// 它用起来像指针，能用 * 取出它指向的包，也能用 ++ 移到下一个包。
	//为什么要解引用？ 因为 addMaterials() 要的是一个 MaterialPack，
	//而 packIt 记录的是当前包在 vector 里的位置。`*packIt` 让你访问这个位置上的包
	// 。
//读这行时先看右边。`erase(packIt)` 删除当前包，原来的迭代器随之失效。`erase()` 会返回一个有效的迭代器，
// 指向被删元素原来的下一个包。左边的赋值用这个新位置更新 packIt。
// 如果删的是最后一个包，返回的就是 end()。
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
	  materialPacks_(std::move(materialPacks))
{
}
//为什么是	  ammoPacks_(std::move(ammoPacks)), 而不是  ammoPacks_(ammoPacks)  

const std::vector<AmmoPack>& ResourceManager::getAmmoPacks() const
{
	return ammoPacks_;
}

const std::vector<MaterialPack>& ResourceManager::getMaterialPacks() const
{
	return materialPacks_;
}

int ResourceManager::getRemainingMaterials() const
{
	return remainingMaterials_;
}

void ResourceManager::addMaterials(MaterialPack materialPack)
{
	remainingMaterials_ += materialPack.amount;
}

