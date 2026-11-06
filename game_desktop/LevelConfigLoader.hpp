#pragma once

#include "GameRenderer.hpp"
#include "Sessions/GameSession.hpp"
#include <filesystem>

struct LevelConfig {
	GameSessionConfig session;
	RendererConfig renderer;
};

LevelConfig loadLevelConfig(const std::filesystem::path& path);
