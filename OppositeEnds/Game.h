
#pragma once

#include "Component.h"
#include "SpriteRenderer.h"
#include "GameDefines.h"
#include "ObstacleSystem.h"
#include "PuzzleSolver.h"

#include <chrono>
#include <string>
#include <utility>
#include <vector>

class CGame : public LComponent {
private:

    // =========================
    // Renderer
    // =========================
    LSpriteRenderer* m_pRenderer = nullptr;

    // =========================
    // Game State
    // =========================
    bool m_bGameStarted = false;
    bool m_bGameWon = false;
    bool m_bGameLost = false;
    bool m_levelLoaded = false;
    bool m_allLevelsComplete = false;

    int m_levelNumber = 1;

    std::string m_levelError;
    std::string m_hintText;

    std::chrono::steady_clock::time_point
        m_edgeFlashUntil{};

    // =========================
    // Screen and Tile Settings
    // =========================
    static constexpr int SCREEN_WIDTH = 720;
    static constexpr int SCREEN_HEIGHT = 720;

    static constexpr float TILE_SIZE = 40.0f;
    static constexpr float CHARACTER_SIZE = 40.0f;
    static constexpr float GOAL_SIZE = 40.0f;

    // =========================
    // Menu System
    // =========================
    enum class MenuPage {
        Main,
        Settings,
        HowToPlay,
        LevelSelect
    };

    MenuPage m_menuPage = MenuPage::Main;

    bool m_showControls = true;

    void DrawStartScreen();

    bool MouseOverRect(
        float x,
        float y,
        float width,
        float height
    );

    void DrawMenuButton(
        const char* label,
        float x,
        float y,
        float width,
        float height,
        float textX
    );

    // =========================
    // Level and Player Data
    // =========================
    using GridPosition = ObstacleSystem::Position;

    ObstacleSystem m_obstacles;

    std::vector<std::pair<eSprite, eSprite>>
        m_facingHistory;

    eSprite m_primarySprite =
        eSprite::PrimaryCharaDown;

    eSprite m_oppositeSprite =
        eSprite::OppositeCharaDown;

    // =========================
    // Level Management
    // =========================
    bool LoadLevel(int number);

    Vector2 CellCenter(int row, int col) const;

    // =========================
    // Rendering
    // =========================
    void LoadImages();

    void RenderFrame();

    void DrawGameGrid();

    void DrawTile(
        int row,
        int col,
        const XMFLOAT4& color,
        eSprite sprite = eSprite::Tile
    );

    void DrawCharacter(
        eSprite sprite,
        const GridPosition& position
    );

    void DrawCrate(int row, int col);

    void DrawGoals(
        const GridPosition& primaryGoal,
        const GridPosition& oppositeGoal
    );

    void DrawSharedGoal(
        const GridPosition& position
    );

    // =========================
    // Input and Movement
    // =========================
    void KeyboardHandler();

    void TryMove(int rowDelta, int colDelta);

    // =========================
    // Win and Lose Conditions
    // =========================
    bool IsGameWon() const;
    bool isGameLost() const;

    void GameWon();
    void GameLost();

public:

    CGame() = default;
    ~CGame() = default;

    void Initialize();
    void ProcessFrame();
    void Release();
};
