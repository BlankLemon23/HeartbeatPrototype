// Desktop entry point.
#include "raylib.h"
#include "GameRenderer.hpp"
#include "Sessions/GameSession.hpp"
#include <iostream>
#include <string>

int main() {
	GameSession session;

	// Presentation and window setup. Input uses the same cell size as drawing.
	RendererConfig rendererConfig{};
	GameRenderer renderer(rendererConfig);
	const int cellSize = rendererConfig.cellSizePixels;
	const int width = session.getMap().maxColumnCount() * cellSize;
	const int height = session.getMap().rowCount() * cellSize;

	std::string title;
	std::cout << "Enter window title:" << std::endl;
	std::cin >> title;

	InitWindow(width, height, title.c_str());
	SetTargetFPS(60);

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

		input.toggleStealth = IsKeyPressed(KEY_LEFT_SHIFT);
		input.reload = IsKeyPressed(KEY_R);
		input.toggleMolotov = IsKeyPressed(KEY_V);
		input.toggleDecoyMine = IsKeyPressed(KEY_B);
		input.craftMolotov = IsKeyPressed(KEY_ONE);
		input.craftDecoyMine = IsKeyPressed(KEY_TWO);
		input.scan = IsKeyPressed(KEY_SPACE);

		const Vector2 mousePosition = GetMousePosition();
		//mousePosition.x >= 0.0f	鼠标 x 坐标不为负（不在窗口左边界之外）
		//mousePosition.x < width	鼠标 x 坐标小于窗口宽度（不在右边界之外）
		//mousePosition.y >= 0.0f	鼠标 y 坐标不为负（不在窗口上边界之外）
		//mousePosition.y < height	鼠标 y 坐标小于窗口高度（不在下边界之外）
		const bool isMouseInsideWindow =
			mousePosition.x >= 0.0f && mousePosition.x < width &&
			mousePosition.y >= 0.0f && mousePosition.y < height;
		// Firing and the aim preview use the same mouse sample in cell units.
		const GridPoint aimPosition{
			static_cast<double>(mousePosition.y) / cellSize,
			static_cast<double>(mousePosition.x) / cellSize
		};

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isMouseInsideWindow) {
			input.weaponTarget = aimPosition;
		}

		if (!session.update(elapsedSeconds, input)) {
			break;
		}

		// Draw back-to-front from the updated session state.
		BeginDrawing();
		ClearBackground(RAYWHITE);
		renderer.drawWorld(session.getMap(), session.getMolotov(), session.getResourceManager());

		if (session.isPreparingThrow()) {
			renderer.drawThrowPreview(session.getThrowPreviewCells());
		}
		else if (isMouseInsideWindow) {
			const Position playerPosition = session.getPlayer().getPosition();
			const GridPoint edgePosition = session.getGun().calculateEdgePosition(playerPosition, aimPosition);
			renderer.drawAimLine(playerPosition, edgePosition);
		}

		renderer.drawActors(session.getPlayer(), session.getEnemySnapshotPositions());
		renderer.drawHud(session.getMolotov(), session.getDecoyMine(),
			session.getGun(), session.getResourceManager());
		EndDrawing();
	}

	CloseWindow();
	return 0;
}
