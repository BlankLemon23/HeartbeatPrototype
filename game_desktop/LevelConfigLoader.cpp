#include "LevelConfigLoader.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <locale>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct ConfigLine {
	int number;
	std::string text;
};

struct ConfigSection {
	int headerLine = 0;
	std::vector<ConfigLine> lines;
};

using ConfigSections = std::map<std::string, ConfigSection>;

static constexpr int maxRadiusCells = 128;
static constexpr int maxCount = std::numeric_limits<int>::max() / 2;
static constexpr double minIntervalSeconds = 0.001;
static constexpr double maxIntervalSeconds = 3600.0;

static std::runtime_error configError(int line, const std::string& location, const std::string& reason) {
	const std::string prefix = line > 0 ? "Line " + std::to_string(line) + " " : "";
	return std::runtime_error(prefix + location + ": " + reason);
}

static std::string trim(const std::string& text) {
	const auto first = text.find_first_not_of(" \t\r");
	if (first == std::string::npos) {
		return {};
	}
	return text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
}

static ConfigSections readSections(std::istream& file) {
	ConfigSections sections;
	ConfigSection* current = nullptr;
	std::string rawLine;
	int lineNumber = 0;

	while (std::getline(file, rawLine)) {
		++lineNumber;
		if (lineNumber == 1 && rawLine.compare(0, 3, "\xEF\xBB\xBF") == 0) {
			rawLine.erase(0, 3);
		}
		std::string line = trim(rawLine.substr(0, rawLine.find('#')));
		if (line.empty()) {
			continue;
		}
		if (line.front() == '[') {
			if (line.back() != ']' || line.size() < 3) {
				throw configError(lineNumber, "level", "Invalid section header.");
			}
			const std::string name = trim(line.substr(1, line.size() - 2));
			if (name.empty()) {
				throw configError(lineNumber, "level", "Section name cannot be empty.");
			}
			const auto [entry, inserted] = sections.emplace(name, ConfigSection{lineNumber, {}});
			if (!inserted) {
				throw configError(lineNumber, "[" + name + "]", "Duplicate section.");
			}
			current = &entry->second;
		}
		else {
			if (!current) {
				throw configError(lineNumber, "level", "Expected a section header before data.");
			}
			current->lines.push_back({lineNumber, std::move(line)});
		}
	}

	if (file.bad()) {
		throw std::runtime_error("Could not read the level file.");
	}
	return sections;
}

enum class ConfigMode {
	Required,
	Override
};

static ConfigSection takeSection(ConfigSections& sections, const std::string& name,
	ConfigMode mode = ConfigMode::Required) {
	const auto entry = sections.find(name);
	if (entry == sections.end()) {
		if (mode == ConfigMode::Required) {
			throw configError(0, "[" + name + "]", "Missing required section.");
		}
		return {};
	}
	ConfigSection section = std::move(entry->second);
	sections.erase(entry);
	return section;
}

template <typename T>
static T parseNumber(const ConfigLine& value, const std::string& location, double min, double max) {
	std::istringstream input(value.text);
	input.imbue(std::locale::classic());
	T number{};
	if (!(input >> number)) {
		throw configError(value.number, location, "Invalid number '" + value.text + "'.");
	}
	input >> std::ws;
	if (!input.eof() || !std::isfinite(number) || number < min || number > max) {
		throw configError(value.number, location, "Expected a number from " + std::to_string(min) + " to " + std::to_string(max) + ".");
	}
	return number;
}

static std::vector<std::string> splitValues(const std::string& text) {
	std::istringstream input(text);
	std::vector<std::string> values;
	std::string value;
	while (input >> value) {
		values.push_back(value);
	}
	return values;
}

class ConfigOptions {
public:
	ConfigOptions(ConfigSections& sections, const std::string& name, ConfigMode mode = ConfigMode::Required)
		: location_("[" + name + "]"), mode_(mode) {
		const ConfigSection section = takeSection(sections, name, mode);
		headerLine_ = section.headerLine;
		for (const ConfigLine& line : section.lines) {
			const auto separator = line.text.find('=');
			if (separator == std::string::npos) {
				throw configError(line.number, location_, "Expected key = value.");
			}
			const std::string key = trim(line.text.substr(0, separator));
			const std::string value = trim(line.text.substr(separator + 1));
			if (key.empty() || value.empty()) {
				throw configError(line.number, location_, "Both key and value are required.");
			}
			if (!values_.emplace(key, ConfigLine{line.number, value}).second) {
				throw configError(line.number, location_, "Duplicate key '" + key + "'.");
			}
		}
	}

	template <typename T>
	void number(const char* key, T& target, double min, double max) {
		if (const auto value = take(key)) {
			target = parseNumber<T>(*value, location_ + " " + key, min, max);
		}
	}

	void color(const char* key, Color& target) {
		if (const auto value = take(key)) {
			const auto channels = splitValues(value->text);
			const std::string field = location_ + " " + key;
			if (channels.size() != 4) {
				throw configError(value->number, field, "Expected four RGBA values.");
			}
			const auto readChannel = [&](const std::string& channel) {
				return static_cast<unsigned char>(
					parseNumber<int>({value->number, channel}, field, 0, 255));
			};
			target = {readChannel(channels[0]), readChannel(channels[1]),
				readChannel(channels[2]), readChannel(channels[3])};
		}
	}

	void state(const char* key, GameState& target) {
		if (const auto value = take(key)) {
			if (value->text == "playing") {
				target = GameState::Playing;
			}
			else if (value->text == "victory") {
				target = GameState::Victory;
			}
			else if (value->text == "defeat") {
				target = GameState::Defeat;
			}
			else {
				throw configError(value->number, location_ + " " + key, "Expected playing, victory or defeat.");
			}
		}
	}

	void require(bool condition, const char* reason) const {
		if (!condition) {
			throw configError(headerLine_, location_, reason);
		}
	}

	void finish() const {
		if (!values_.empty()) {
			const auto& [key, value] = *values_.begin();
			throw configError(value.number, location_, "Unknown key '" + key + "'.");
		}
	}

private:
	std::optional<ConfigLine> take(const char* key) {
		const auto entry = values_.find(key);
		if (entry == values_.end()) {
			if (mode_ == ConfigMode::Required) {
				throw configError(headerLine_, location_, "Missing required key '" + std::string(key) + "'.");
			}
			return std::nullopt;
		}
		ConfigLine value = std::move(entry->second);
		values_.erase(entry);
		return value;
	}

	std::string location_;
	ConfigMode mode_;
	int headerLine_ = 0;
	std::map<std::string, ConfigLine> values_;
};

static bool isFloor(const MapConfig& map, Position position) {
	return position.row >= 0 && position.column >= 0 &&
		static_cast<std::size_t>(position.row) < map.rows.size() &&
		static_cast<std::size_t>(position.column) < map.rows[position.row].size() &&
		map.rows[position.row][position.column] == '.';
}

static void readMap(ConfigSections& sections, MapConfig& map) {
	const ConfigSection section = takeSection(sections, "map");
	if (section.lines.empty()) {
		throw configError(section.headerLine, "[map]", "Expected at least one map row.");
	}
	for (const ConfigLine& line : section.lines) {
		if (line.text.find_first_not_of("*.") != std::string::npos) {
			throw configError(line.number, "[map]", "Map rows can only contain '*' and '.'.");
		}
		map.rows.push_back(line.text);
	}
}

static void readPlayer(ConfigSections& sections, GameSessionConfig& session) {
	ConfigOptions options(sections, "player");
	PlayerConfig& player = session.player;
	options.number("startRow", session.playerStart.row, 0, maxCount);
	options.number("startColumn", session.playerStart.column, 0, maxCount);
	options.number("maxHealth", player.maxHealth, 1, maxCount);
	options.number("normalMoveIntervalSeconds", player.normalMoveIntervalSeconds, minIntervalSeconds, maxIntervalSeconds);
	options.number("stealthMoveIntervalSeconds", player.stealthMoveIntervalSeconds, minIntervalSeconds, maxIntervalSeconds);
	options.number("outBattleIntervalSeconds", player.outBattleIntervalSeconds, minIntervalSeconds, maxIntervalSeconds);
	options.number("lowHealthThresholdPercent", player.lowHealthThresholdPercent, 0, 100);
	options.number("mediumHealthThresholdPercent", player.mediumHealthThresholdPercent, 0, 100);
	options.finish();
	options.require(player.lowHealthThresholdPercent <= player.mediumHealthThresholdPercent,
		"lowHealthThresholdPercent must be at most mediumHealthThresholdPercent.");
	options.require(isFloor(session.mapConfig, session.playerStart), "Player spawn must be on a floor cell.");
}

static void readEnemyOptions(ConfigSections& sections, const std::string& name, EnemyConfig& enemy,
	ConfigMode mode = ConfigMode::Required) {
	ConfigOptions options(sections, name, mode);
	options.number("startingHealth", enemy.startingHealth, 1, maxCount);
	options.number("moveIntervalSeconds", enemy.moveIntervalSeconds, minIntervalSeconds, maxIntervalSeconds);
	options.number("wanderIntervalSeconds", enemy.wanderIntervalSeconds, minIntervalSeconds, maxIntervalSeconds);
	options.number("attackIntervalSeconds", enemy.attackIntervalSeconds, minIntervalSeconds, maxIntervalSeconds);
	options.number("wanderingRadiusCells", enemy.wanderingRadiusCells, 0, maxRadiusCells);
	options.number("waitAfterLossSeconds", enemy.waitAfterLossSeconds, 0, maxIntervalSeconds);
	options.number("normalDetectionRadiusCells", enemy.normalDetectionRadiusCells, 0, maxRadiusCells);
	options.number("stealthDetectionRadiusCells", enemy.stealthDetectionRadiusCells, 0, maxRadiusCells);
	options.number("stopChaseRadiusCells", enemy.stopChaseRadiusCells, 0, maxRadiusCells);
	options.number("attackDamage", enemy.attackDamage, 0, maxCount);
	options.finish();
}

static Position readPosition(const std::vector<std::string>& values, const ConfigLine& line,
	const std::string& section) {
	return {parseNumber<int>({line.number, values[0]}, section + " row", 0, maxCount),
		parseNumber<int>({line.number, values[1]}, section + " column", 0, maxCount)};
}

static void readEnemies(ConfigSections& sections, GameSessionConfig& session) {
	EnemyConfig defaults{};
	readEnemyOptions(sections, "enemy_defaults", defaults);
	const ConfigSection section = takeSection(sections, "enemies");
	for (const ConfigLine& line : section.lines) {
		const auto fields = splitValues(line.text);
		if (fields.size() < 2 || fields.size() > 3) {
			throw configError(line.number, "[enemies]", "Expected row column and optional starting health.");
		}
		EnemySpawnConfig spawn{readPosition(fields, line, "[enemies]"), defaults};
		if (!isFloor(session.mapConfig, spawn.position)) {
			throw configError(line.number, "[enemies]", "Enemy spawn must be on a floor cell.");
		}
		if (fields.size() == 3) {
			spawn.config.startingHealth = parseNumber<int>(
				{line.number, fields[2]}, "[enemies] startingHealth", 1, maxCount);
		}
		readEnemyOptions(sections, "enemy." + std::to_string(session.enemySpawns.size()),
			spawn.config, ConfigMode::Override);
		session.enemySpawns.push_back(spawn);
	}
}

static void readSessionOptions(ConfigSections& sections, GameSessionConfig& session) {
	ConfigOptions startup(sections, "session");
	startup.state("initialState", session.initialState);
	startup.finish();

	ConfigOptions scan(sections, "scan");
	scan.number("initialScanCooldownSeconds", session.initialScanCooldownSeconds, 0, maxIntervalSeconds);
	scan.number("scanRadiusCells", session.scanRadiusCells, 0, maxRadiusCells);
	scan.number("enemySnapshotDurationSeconds", session.enemySnapshotDurationSeconds, minIntervalSeconds, maxIntervalSeconds);
	scan.number("manualScanIntervalSeconds", session.manualScanIntervalSeconds, minIntervalSeconds, maxIntervalSeconds);
	scan.finish();

	ConfigOptions battleScan(sections, "battle_scan");
	battleScan.number("highHealthIntervalSeconds", session.battleScan.highHealthIntervalSeconds, minIntervalSeconds, maxIntervalSeconds);
	battleScan.number("mediumHealthIntervalSeconds", session.battleScan.mediumHealthIntervalSeconds, minIntervalSeconds, maxIntervalSeconds);
	battleScan.number("lowHealthIntervalSeconds", session.battleScan.lowHealthIntervalSeconds, minIntervalSeconds, maxIntervalSeconds);
	battleScan.finish();
}

static void readGun(ConfigSections& sections, GunConfig& gun) {
	ConfigOptions options(sections, "gun");
	options.number("damagePerShot", gun.damagePerShot, 0, maxCount);
	options.number("rangeCells", gun.rangeCells, 1, maxRadiusCells);
	options.number("fireIntervalSeconds", gun.fireIntervalSeconds, minIntervalSeconds, maxIntervalSeconds);
	options.number("soundRadiusCells", gun.soundRadiusCells, 0, maxRadiusCells);
	options.number("magazineCapacity", gun.magazineCapacity, 1, maxCount);
	options.number("maxTotalAmmo", gun.maxTotalAmmo, 1, maxCount);
	options.number("initialMagazineAmmo", gun.initialMagazineAmmo, 0, maxCount);
	options.number("initialReserveAmmo", gun.initialReserveAmmo, 0, maxCount);
	options.number("reloadDurationSeconds", gun.reloadDurationSeconds, minIntervalSeconds, maxIntervalSeconds);
	options.finish();
	options.require(gun.magazineCapacity <= gun.maxTotalAmmo, "magazineCapacity cannot exceed maxTotalAmmo.");
	options.require(gun.initialMagazineAmmo <= gun.magazineCapacity, "initialMagazineAmmo cannot exceed magazineCapacity.");
	options.require(gun.initialMagazineAmmo + gun.initialReserveAmmo <= gun.maxTotalAmmo,
		"Initial magazine and reserve ammo together cannot exceed maxTotalAmmo.");
}

static void readThrowables(ConfigSections& sections, GameSessionConfig& session) {
	MolotovConfig& molotov = session.molotov;
	ConfigOptions fire(sections, "molotov");
	fire.number("throwRadiusCells", molotov.throwRadiusCells, 0, maxRadiusCells);
	fire.number("burnRadiusCells", molotov.burnRadiusCells, 0, maxRadiusCells);
	fire.number("burnDurationSeconds", molotov.burnDurationSeconds, minIntervalSeconds, maxIntervalSeconds);
	fire.number("damageIntervalSeconds", molotov.damageIntervalSeconds, minIntervalSeconds, maxIntervalSeconds);
	fire.number("burnDamageEnemy", molotov.burnDamageEnemy, 0, maxCount);
	fire.number("burnDamagePlayer", molotov.burnDamagePlayer, 0, maxCount);
	fire.number("carryLimit", molotov.carryLimit, 0, maxCount);
	fire.number("initialCount", molotov.initialCount, 0, maxCount);
	fire.finish();
	fire.require(molotov.initialCount <= molotov.carryLimit, "initialCount cannot exceed carryLimit.");

	DecoyMineConfig& decoy = session.decoyMine;
	ConfigOptions mine(sections, "decoy_mine");
	mine.number("throwRadiusCells", decoy.throwRadiusCells, 0, maxRadiusCells);
	mine.number("soundRadiusCells", decoy.soundRadiusCells, 0, maxRadiusCells);
	mine.number("carryLimit", decoy.carryLimit, 0, maxCount);
	mine.number("initialCount", decoy.initialCount, 0, maxCount);
	mine.finish();
	mine.require(decoy.initialCount <= decoy.carryLimit, "initialCount cannot exceed carryLimit.");
}

template <typename Pack>
static void readPacks(ConfigSections& sections, const char* name, const MapConfig& map,
	std::vector<Pack>& packs) {
	const ConfigSection section = takeSection(sections, name);
	const std::string location = "[" + std::string(name) + "]";
	long long total = 0;
	for (const ConfigLine& line : section.lines) {
		const auto fields = splitValues(line.text);
		if (fields.size() != 3) {
			throw configError(line.number, location, "Expected row column amount.");
		}
		const Position position = readPosition(fields, line, location);
		if (!isFloor(map, position)) {
			throw configError(line.number, location, "Supply pack must be on a floor cell.");
		}
		const int amount = parseNumber<int>({line.number, fields[2]}, location + " amount", 1, maxCount);
		total += amount;
		if (total > std::numeric_limits<int>::max()) {
			throw configError(line.number, location, "Total supply amount is too large.");
		}
		packs.push_back({amount, position});
	}
}

static void readResources(ConfigSections& sections, GameSessionConfig& session) {
	ConfigOptions crafting(sections, "crafting");
	crafting.number("molotovMaterialCost", session.crafting.molotovMaterialCost, 0, maxCount);
	crafting.number("decoyMineMaterialCost", session.crafting.decoyMineMaterialCost, 0, maxCount);
	crafting.finish();

	readPacks(sections, "ammo_packs", session.mapConfig, session.ammoPacks);
	readPacks(sections, "material_packs", session.mapConfig, session.materialPacks);
}

static void readRenderer(ConfigSections& sections, const MapConfig& map, RendererConfig& renderer) {
	ConfigOptions options(sections, "renderer");
	options.number("cellSizePixels", renderer.cellSizePixels, 16, 256);
	options.number("fireRadiusInCells", renderer.fireRadiusInCells, 0, 8);
	options.color("fireColor", renderer.fireColor);
	options.color("throwPreviewColor", renderer.throwPreviewColor);
	options.number("throwPreviewLineThicknessPixels", renderer.throwPreviewLineThicknessPixels, 0.1, 64);
	options.color("aimLineColor", renderer.aimLineColor);
	options.number("aimLineThicknessPixels", renderer.aimLineThicknessPixels, 0.1, 64);
	options.finish();

	constexpr int maxWindowPixels = 16384;
	const auto maxMapDimension = static_cast<std::size_t>(maxWindowPixels / renderer.cellSizePixels);
	const bool widthFits = std::all_of(map.rows.begin(), map.rows.end(),
		[maxMapDimension](const std::string& row) { return row.size() <= maxMapDimension; });
	options.require(widthFits && map.rows.size() <= maxMapDimension,
		"Map dimensions and cellSizePixels would create an oversized window.");
}

LevelConfig loadLevelConfig(const std::filesystem::path& path) {
	std::ifstream file(path);
	if (!file) {
		throw std::runtime_error("Could not open the level file.");
	}

	ConfigSections sections = readSections(file);
	LevelConfig config{};
	readMap(sections, config.session.mapConfig);
	readPlayer(sections, config.session);
	readEnemies(sections, config.session);
	readSessionOptions(sections, config.session);
	readGun(sections, config.session.gun);
	readThrowables(sections, config.session);
	readResources(sections, config.session);
	readRenderer(sections, config.session.mapConfig, config.renderer);

	if (!sections.empty()) {
		const auto& [name, section] = *sections.begin();
		throw configError(section.headerLine, "[" + name + "]", "Unknown section.");
	}
	return config;
}
