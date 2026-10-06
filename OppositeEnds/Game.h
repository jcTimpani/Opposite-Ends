#pragma once

#include "Component.h"
#include "SpriteRenderer.h"
#include "GameDefines.h"

class CGame : public LComponent {
private:
    LSpriteRenderer* m_pRenderer = nullptr;

    bool m_bGameStarted = false;

    static constexpr int SCREEN_WIDTH = 720;
    static constexpr int SCREEN_HEIGHT = 720;

    static constexpr float BUTTON_X = 360.0f;
    static constexpr float BUTTON_Y = 320.0f;

    static constexpr float BUTTON_WIDTH = 300.0f;
    static constexpr float BUTTON_HEIGHT = 100.0f;
    static constexpr int GRID_SIZE = 18;
    static constexpr float TILE_SIZE = 40.0f;

    void DrawGameGrid();

    void DrawTile(
        int row,
        int col,
        const XMFLOAT4& color
    );
    void LoadImages();
    void KeyboardHandler();
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