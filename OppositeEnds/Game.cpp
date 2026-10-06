#include "Game.h"
#include "ComponentIncludes.h"

#include <Windows.h>

void CGame::Initialize() {
    m_pRenderer = new LSpriteRenderer(eSpriteMode::Batched2D);
    m_pRenderer->Initialize(eSprite::Size);

    const XMVECTORF32 background = {
        0.05f,
        0.06f,
        0.10f,
        1.0f
    };

    m_pRenderer->SetBgColor(background);

    LoadImages();

    m_bGameStarted = false;
}

void CGame::LoadImages() {
    m_pRenderer->BeginResourceUpload();

    m_pRenderer->Load(
        eSprite::PlayButton,
        "play"
    );

    m_pRenderer->Load(
        eSprite::Tile,
        "tile"
    );

    m_pRenderer->EndResourceUpload();
}

bool CGame::MouseOverPlayButton() {
    POINT mouse;

    if (!GetCursorPos(&mouse))
        return false;

    HWND window = GetActiveWindow();

    if (window == nullptr)
        return false;

    ScreenToClient(window, &mouse);

    const float mouseX =
        static_cast<float>(mouse.x);

    const float mouseY =
        SCREEN_HEIGHT -
        static_cast<float>(mouse.y);

    const float left =
        BUTTON_X - BUTTON_WIDTH / 2.0f;

    const float right =
        BUTTON_X + BUTTON_WIDTH / 2.0f;

    const float bottom =
        BUTTON_Y - BUTTON_HEIGHT / 2.0f;

    const float top =
        BUTTON_Y + BUTTON_HEIGHT / 2.0f;

    return
        mouseX >= left &&
        mouseX <= right &&
        mouseY >= bottom &&
        mouseY <= top;
}

void CGame::KeyboardHandler() {
    m_pKeyboard->GetState();

    if (!m_bGameStarted) {
        if (
            m_pKeyboard->TriggerDown(VK_LBUTTON) &&
            MouseOverPlayButton()
            ) {
            m_bGameStarted = true;
        }

        if (m_pKeyboard->TriggerDown(VK_RETURN)) {
            m_bGameStarted = true;
        }
    }
}

void CGame::DrawStartScreen() {
    // Title
    m_pRenderer->DrawScreenText(
        "OPPOSITE ENDS",
        Vector2(270.0f, 180.0f),
        Colors::White
    );

    // Play button
    LSpriteDesc2D button;

    button.m_nSpriteIndex =
        static_cast<UINT>(eSprite::PlayButton);

    button.m_vPos =
        Vector2(BUTTON_X, BUTTON_Y);

    const float originalWidth =
        m_pRenderer->GetWidth(eSprite::PlayButton);

    const float originalHeight =
        m_pRenderer->GetHeight(eSprite::PlayButton);

    button.m_fXScale =
        BUTTON_WIDTH / originalWidth;

    button.m_fYScale =
        BUTTON_HEIGHT / originalHeight;

    if (MouseOverPlayButton()) {
        button.m_f4Tint =
            XMFLOAT4(
                0.7f,
                0.8f,
                1.0f,
                1.0f
            );
    }
    else {
        button.m_f4Tint =
            XMFLOAT4(
                1.0f,
                1.0f,
                1.0f,
                1.0f
            );
    }

    m_pRenderer->Draw(&button);
}
void CGame::DrawTile(
    int row,
    int col,
    const XMFLOAT4& color
) {
    LSpriteDesc2D tile;

    tile.m_nSpriteIndex =
        static_cast<UINT>(eSprite::Tile);

    // Find center of this grid square.
    float x =
        col * TILE_SIZE +
        TILE_SIZE / 2.0f;

    float y =
        SCREEN_HEIGHT -
        (
            row * TILE_SIZE +
            TILE_SIZE / 2.0f
            );

    tile.m_vPos =
        Vector2(x, y);

    float originalWidth =
        m_pRenderer->GetWidth(eSprite::Tile);

    float originalHeight =
        m_pRenderer->GetHeight(eSprite::Tile);

    tile.m_fXScale =
        TILE_SIZE / originalWidth;

    tile.m_fYScale =
        TILE_SIZE / originalHeight;

    tile.m_f4Tint = color;

    m_pRenderer->Draw(&tile);
}
void CGame::DrawGameGrid() {
    const XMFLOAT4 orange =
        XMFLOAT4(
            1.0f,
            0.45f,
            0.05f,
            1.0f
        );

    const XMFLOAT4 gray =
        XMFLOAT4(
            0.35f,
            0.35f,
            0.35f,
            1.0f
        );

    const XMFLOAT4 lightGray =
        XMFLOAT4(
            0.65f,
            0.65f,
            0.65f,
            1.0f
        );


    for (int row = 0; row < GRID_SIZE; row++) {

        for (int col = 0; col < GRID_SIZE; col++) {

            // Outer edge
            if (
                row == 0 ||
                row == GRID_SIZE - 1 ||
                col == 0 ||
                col == GRID_SIZE - 1
                ) {
                DrawTile(
                    row,
                    col,
                    orange
                );
            }

            // Inner 16 x 16 checkerboard
            else {

                if ((row + col) % 2 == 0) {
                    DrawTile(
                        row,
                        col,
                        gray
                    );
                }
                else {
                    DrawTile(
                        row,
                        col,
                        lightGray
                    );
                }
            }
        }
    }
}
void CGame::RenderFrame() {
    m_pRenderer->BeginFrame();

    if (!m_bGameStarted) {
        DrawStartScreen();
    }
    else {
        DrawGameGrid();
    }

    m_pRenderer->EndFrame();
}

void CGame::ProcessFrame() {
    KeyboardHandler();
    RenderFrame();
}

void CGame::Release() {
    delete m_pRenderer;
    m_pRenderer = nullptr;
}