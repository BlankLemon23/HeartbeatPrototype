// Desktop entry point.
#include "raylib.h"
#include "GameRenderer.hpp"
#include "LevelConfigLoader.hpp"
#include "Sessions/GameSession.hpp"
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
enum RunResult {
	Quit,
	Restart,
	Initial
};
static RunResult runGame(GameSession& session, const RendererConfig& rendererConfig) {
	RunResult result_ = RunResult::Initial;
	GameRenderer renderer(rendererConfig);
	const int cellSize = rendererConfig.cellSizePixels;
	const int width = session.getMap().maxColumnCount() * cellSize;
	const int height = session.getMap().rowCount() * cellSize;

	std::string title = "gameprototype";

	InitWindow(width, height, title.c_str());
	SetTargetFPS(60);
	const ResultButtons resultButtons = renderer.getResultButtons(width, height);

	while (!WindowShouldClose()) {
		const double elapsedSeconds = GetFrameTime();
		SessionInput input;

		if (IsKeyDown(KEY_D)) {
			input.moveDirection = Direction::RIGHT;
		}
		else if (IsKeyDown(KEY_W)) {
			input.moveDirection = Direction::UP;
		}
		else if (IsKeyDown(KEY_S)) {
			input.moveDirection = Direction::DOWN;
		}
		else if (IsKeyDown(KEY_A)) {
			input.moveDirection = Direction::LEFT;
		}

		// Capture actions triggered on this frame.
		input.toggleStealth = IsKeyPressed(KEY_LEFT_SHIFT);
		input.reload = IsKeyPressed(KEY_R);
		input.toggleMolotov = IsKeyPressed(KEY_V);
		input.toggleDecoyMine = IsKeyPressed(KEY_B);
		input.craftMolotov = IsKeyPressed(KEY_ONE);
		input.craftDecoyMine = IsKeyPressed(KEY_TWO);
		input.scan = IsKeyPressed(KEY_SPACE);

		const Vector2 mousePosition = GetMousePosition();

		const bool isMouseInsideWindow =
			mousePosition.x >= 0.0f && mousePosition.x < width &&
			mousePosition.y >= 0.0f && mousePosition.y < height;

		const GridPoint aimPosition{
			static_cast<double>(mousePosition.y) / cellSize,
			static_cast<double>(mousePosition.x) / cellSize};

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isMouseInsideWindow) {
			input.weaponTarget = aimPosition;
		}

		GameState frameState = session.update(elapsedSeconds, input);

		BeginDrawing();
		if (frameState == GameState::Defeat) {
			renderer.drawDefeatInterface(resultButtons);
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
				CheckCollisionPointRec(mousePosition, resultButtons.quit)) {
				result_ = RunResult::Quit;
			}
			else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
				CheckCollisionPointRec(mousePosition, resultButtons.restart)) {
				result_ = RunResult::Restart;
			}
		}
		else if (frameState == GameState::Victory) {
			renderer.drawVictoryInterface(resultButtons);
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
				CheckCollisionPointRec(mousePosition, resultButtons.quit)) {
				result_ = RunResult::Quit;
			}
			else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
				CheckCollisionPointRec(mousePosition, resultButtons.restart)) {
				result_ = RunResult::Restart;
			}
		}
		else {
			ClearBackground(RAYWHITE);
			renderer.drawWorld(session.getMap(), session.getMolotov(), session.getResourceManager());

			if (session.getPlayer().isStealthEnabled()) {
				renderer.drawStealthDetectionRange(session.getPlayer().getPosition(),
					session.getStealthDetectionRadiusCells());
			}

			if (session.isPreparingThrow()) {
				renderer.drawThrowPreview(session.getThrowPreviewCells());
			}
			else if (isMouseInsideWindow) {
				const Position playerPosition = session.getPlayer().getPosition();
				const GridPoint edgePosition = session.getGun().calculateEdgePosition(playerPosition, aimPosition);
				renderer.drawAimLine(playerPosition, edgePosition);
			}

			renderer.drawActors(session.getPlayer(), session.getEnemySnapshotPositions());
			renderer.drawHud(session.getPlayer(), session.getMolotov(), session.getDecoyMine(),
				session.getGun(), session.getResourceManager(), session.getActionFeedback());
		}
		EndDrawing();
		if (result_ == RunResult::Quit || result_ == RunResult::Restart) {
			break;
		}
	}

	CloseWindow();
	return result_;
}

int main() {
	const std::filesystem::path levelPath = std::filesystem::path(GetApplicationDirectory()) / "level.txt";
	try {
		while (true) {
			LevelConfig config = loadLevelConfig(levelPath);
			GameSession session(std::move(config.session));
			if (runGame(session, config.renderer) != RunResult::Restart) {
				break;
			}
		}
		return 0;
	}
	catch (const std::exception& error) {
		std::cerr << "Could not run level " << levelPath << ": " << error.what() << '\n';
		return 1;
	}
}
