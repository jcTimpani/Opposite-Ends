#pragma once

#include "Component.h"
#include "SpriteRenderer.h"
#include "GameDefines.h"
#include "ObstacleSystem.h"

#include <utility>
#include <vector>

class CGame : public LComponent {
private:
    LSpriteRenderer* m_pRenderer = nullptr;

    bool m_bGameStarted = false;
	bool m_bGameWon = false;

    static constexpr int SCREEN_WIDTH = 720;
    static constexpr int SCREEN_HEIGHT = 720;

    static constexpr float BUTTON_X = 360.0f;
    static constexpr float BUTTON_Y = 320.0f;

    static constexpr float BUTTON_WIDTH = 300.0f;
    static constexpr float BUTTON_HEIGHT = 100.0f;

    static constexpr int GRID_SIZE = ObstacleSystem::kGridSize;
    static constexpr float TILE_SIZE = 40.0f;
    static constexpr float CHARACTER_SIZE = 40.0f;
    static constexpr float GOAL_SIZE = 40.0f;

    using GridPosition = ObstacleSystem::Position;
    ObstacleSystem m_obstacles;
    std::vector<std::pair<eSprite, eSprite>> m_facingHistory;

	eSprite m_backgroundSprite = eSprite::Background;

	// Starting sprites for the characters
	eSprite m_primarySprite = eSprite::PrimaryCharaDown;
	eSprite m_oppositeSprite = eSprite::OppositeCharaDown;

    // Goal sprites
	eSprite m_primaryGoalSprite = eSprite::PrimaryGoal;
	eSprite m_oppositeGoalSprite = eSprite::OppositeGoal;
	eSprite m_sharedGoalSprite = eSprite::SharedGoal;

	// Temporary goal positions
    GridPosition m_primaryGoalPosition{ 13, 13 };
	GridPosition m_oppositeGoalPosition{ 4, 4 };

	// used tocheck if level has a shared goal or not, if true, both characters will have the same goal position
	bool m_sharedGoal = false;

    void DrawGameGrid();
    void DrawTile(int row, int col, const XMFLOAT4& color);
    void DrawCharacter(eSprite sprite, const GridPosition& position);
    void DrawCrate(int row, int col);

	// Draw two goals one for each character
    void DrawGoals(const GridPosition& primaryGoalPosition, const GridPosition& oppositeGoalPosition);
	// Draw a shared goal for both characters
	void DrawSharedGoal(const GridPosition& goalPosition);

    void LoadImages();
    void KeyboardHandler();
    void TryMove(int rowDelta, int colDelta);
    bool IsInsidePlayArea(const GridPosition& position) const;
	bool IsGameWon() const;
	void GameWon();

    void RenderFrame();
    void DrawStartScreen();
    bool MouseOverPlayButton();

public:
    CGame() = default;
    ~CGame() = default;

    void Initialize();
    void ProcessFrame();
    void Release();
};
