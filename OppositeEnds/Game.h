#pragma once

#include "Component.h"
#include "SpriteRenderer.h"
#include "GameDefines.h"
#include "ObstacleSystem.h"
#include <chrono>
#include <string>
#include <utility>
#include <vector>

class CGame : public LComponent {
private:
    LSpriteRenderer* m_pRenderer = nullptr;
    bool m_bGameStarted = false;
    bool m_bGameWon = false;
    bool m_levelLoaded = false;
    bool m_allLevelsComplete = false;
    int m_levelNumber = 1;
    std::string m_levelError;
    std::chrono::steady_clock::time_point m_edgeFlashUntil{};

    static constexpr int SCREEN_WIDTH = 720;
    static constexpr int SCREEN_HEIGHT = 720;
    static constexpr float BUTTON_X = 360.0f;
    static constexpr float BUTTON_Y = 320.0f;
    static constexpr float BUTTON_WIDTH = 300.0f;
    static constexpr float BUTTON_HEIGHT = 100.0f;
    static constexpr float TILE_SIZE = 40.0f;
    static constexpr float CHARACTER_SIZE = 40.0f;
    static constexpr float GOAL_SIZE = 40.0f;

    using GridPosition = ObstacleSystem::Position;
    ObstacleSystem m_obstacles;
    std::vector<std::pair<eSprite, eSprite>> m_facingHistory;
    eSprite m_primarySprite = eSprite::PrimaryCharaDown;
    eSprite m_oppositeSprite = eSprite::OppositeCharaDown;

    bool LoadLevel(int number);
    Vector2 CellCenter(int row, int col) const;
    void DrawGameGrid();
    void DrawTile(int row, int col, const XMFLOAT4& color, eSprite sprite = eSprite::Tile);
    void DrawCharacter(eSprite sprite, const GridPosition& position);
    void DrawCrate(int row, int col);
    void DrawGoals(const GridPosition& primaryGoal, const GridPosition& oppositeGoal);
    void DrawSharedGoal(const GridPosition& position);
    void LoadImages();
    void KeyboardHandler();
    void TryMove(int rowDelta, int colDelta);
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
