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

    m_pRenderer->Load(eSprite::PlayButton, "play");
    m_pRenderer->Load(eSprite::Tile, "tile");
    m_pRenderer->Load(eSprite::PrimaryCharaDown, "astronaut_a_down");
	m_pRenderer->Load(eSprite::PrimaryCharaLeft, "astronaut_a_left");
	m_pRenderer->Load(eSprite::PrimaryCharaRight, "astronaut_a_right"); 
	m_pRenderer->Load(eSprite::PrimaryCharaUp, "astronaut_a_up");
    m_pRenderer->Load(eSprite::OppositeCharaDown, "astronaut_b_down");
	m_pRenderer->Load(eSprite::OppositeCharaLeft, "astronaut_b_left");
	m_pRenderer->Load(eSprite::OppositeCharaRight, "astronaut_b_right");  
	m_pRenderer->Load(eSprite::OppositeCharaUp, "astronaut_b_up");

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

    const float mouseX = static_cast<float>(mouse.x);
    const float mouseY = SCREEN_HEIGHT - static_cast<float>(mouse.y);

    const float left = BUTTON_X - BUTTON_WIDTH / 2.0f;
    const float right = BUTTON_X + BUTTON_WIDTH / 2.0f;
    const float bottom = BUTTON_Y - BUTTON_HEIGHT / 2.0f;
    const float top = BUTTON_Y + BUTTON_HEIGHT / 2.0f;

    return mouseX >= left && mouseX <= right &&
        mouseY >= bottom && mouseY <= top;
}

void CGame::KeyboardHandler() {
    m_pKeyboard->GetState();

    if (!m_bGameStarted) {
        if (m_pKeyboard->TriggerDown(VK_LBUTTON) && MouseOverPlayButton()) {
            m_bGameStarted = true;
        }

        if (m_pKeyboard->TriggerDown(VK_RETURN)) {
            m_bGameStarted = true;
        }

        return;
    }


    if (m_pKeyboard->TriggerDown('W') ||
        m_pKeyboard->TriggerDown(VK_UP)) {
        TryMove(-1, 0);
		m_primarySprite = eSprite::PrimaryCharaUp; 
		m_oppositeSprite = eSprite::OppositeCharaDown;
    }
    else if (m_pKeyboard->TriggerDown('A') ||
        m_pKeyboard->TriggerDown(VK_LEFT)) {
        TryMove(0, -1);
		m_primarySprite = eSprite::PrimaryCharaLeft;
		m_oppositeSprite = eSprite::OppositeCharaRight;
    }
    else if (m_pKeyboard->TriggerDown('S') ||
        m_pKeyboard->TriggerDown(VK_DOWN)) {
        TryMove(1, 0);
		m_primarySprite = eSprite::PrimaryCharaDown;
		m_oppositeSprite = eSprite::OppositeCharaUp;
    }
    else if (m_pKeyboard->TriggerDown('D') ||
        m_pKeyboard->TriggerDown(VK_RIGHT)) {
        TryMove(0, 1);
		m_primarySprite = eSprite::PrimaryCharaRight;
		m_oppositeSprite = eSprite::OppositeCharaLeft;
    }

}

bool CGame::IsInsidePlayArea(const GridPosition& position) const {
    // Tile 0 and tile GRID_SIZE - 1 form the orange wall.
    return position.row > 0 && position.row < GRID_SIZE - 1 &&
        position.col > 0 && position.col < GRID_SIZE - 1;
}

// Fixed internal walls for the first obstacle test level.
bool CGame::IsObstacle(const GridPosition& position) const {
    static const GridPosition obstacles[] = {
        { 6, 4 }, { 6, 5 }, { 6, 6 }, { 6, 7 }, { 6, 8 },
        { 11, 10 }, { 12, 10 }, { 13, 10 }
    };

    for (const GridPosition& obstacle : obstacles) {
        if (position.row == obstacle.row && position.col == obstacle.col) {
            return true;
        }
    }

    return false;
}

void CGame::TryMove(int rowDelta, int colDelta) {
    const GridPosition nextPrimary{
        m_primaryPosition.row + rowDelta,
        m_primaryPosition.col + colDelta
    };

    // The opposite character always takes the inverse step.
    const GridPosition nextOpposite{
        m_oppositePosition.row - rowDelta,
        m_oppositePosition.col - colDelta
    };

    // Each astronaut is blocked independently by borders and internal walls.
    if (IsInsidePlayArea(nextPrimary) && !IsObstacle(nextPrimary)) {
        m_primaryPosition = nextPrimary;
    }

    if (IsInsidePlayArea(nextOpposite) && !IsObstacle(nextOpposite)) {
        m_oppositePosition = nextOpposite;
    }
}

void CGame::DrawStartScreen() {
    m_pRenderer->DrawScreenText(
        "OPPOSITE ENDS",
        Vector2(270.0f, 180.0f),
        Colors::White
    );

    LSpriteDesc2D button;
    button.m_nSpriteIndex = static_cast<UINT>(eSprite::PlayButton);
    button.m_vPos = Vector2(BUTTON_X, BUTTON_Y);

    const float originalWidth = m_pRenderer->GetWidth(eSprite::PlayButton);
    const float originalHeight = m_pRenderer->GetHeight(eSprite::PlayButton);

    button.m_fXScale = BUTTON_WIDTH / originalWidth;
    button.m_fYScale = BUTTON_HEIGHT / originalHeight;

    if (MouseOverPlayButton()) {
        button.m_f4Tint = XMFLOAT4(0.7f, 0.8f, 1.0f, 1.0f);
    }
    else {
        button.m_f4Tint = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    m_pRenderer->Draw(&button);
}

void CGame::DrawTile(int row, int col, const XMFLOAT4& color) {
    LSpriteDesc2D tile;
    tile.m_nSpriteIndex = static_cast<UINT>(eSprite::Tile);

    const float x = col * TILE_SIZE + TILE_SIZE / 2.0f;
    const float y = SCREEN_HEIGHT - (row * TILE_SIZE + TILE_SIZE / 2.0f);

    tile.m_vPos = Vector2(x, y);

    const float originalWidth = m_pRenderer->GetWidth(eSprite::Tile);
    const float originalHeight = m_pRenderer->GetHeight(eSprite::Tile);

    tile.m_fXScale = TILE_SIZE / originalWidth;
    tile.m_fYScale = TILE_SIZE / originalHeight;
    tile.m_f4Tint = color;

    m_pRenderer->Draw(&tile);
}

void CGame::DrawCharacter(eSprite sprite, const GridPosition& position) {
    LSpriteDesc2D character;
    character.m_nSpriteIndex = static_cast<UINT>(sprite);

    // Keep the character centered in its grid cell.
    const float x = position.col * TILE_SIZE + TILE_SIZE / 2.0f;
    const float y = SCREEN_HEIGHT - (position.row * TILE_SIZE + TILE_SIZE / 2.0f);
    character.m_vPos = Vector2(x, y);

    const float width = m_pRenderer->GetWidth(sprite);
    const float height = m_pRenderer->GetHeight(sprite);
    character.m_fXScale = CHARACTER_SIZE / width;
    character.m_fYScale = CHARACTER_SIZE / height;
    character.m_f4Tint = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    m_pRenderer->Draw(&character);
}

void CGame::DrawGameGrid() {
    const XMFLOAT4 orange = XMFLOAT4(1.0f, 0.45f, 0.05f, 1.0f);
    const XMFLOAT4 gray = XMFLOAT4(0.35f, 0.35f, 0.35f, 1.0f);
    const XMFLOAT4 lightGray = XMFLOAT4(0.65f, 0.65f, 0.65f, 1.0f);
    const XMFLOAT4 wallColor = XMFLOAT4(0.38f, 0.20f, 0.72f, 1.0f);

    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            if (row == 0 || row == GRID_SIZE - 1 ||
                col == 0 || col == GRID_SIZE - 1) {
                DrawTile(row, col, orange);
            }
            else if (IsObstacle(GridPosition{ row, col })) {
                DrawTile(row, col, wallColor);
            }
            else if ((row + col) % 2 == 0) {
                DrawTile(row, col, gray);
            }
            else {
                DrawTile(row, col, lightGray);
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
        DrawCharacter(m_primarySprite, m_primaryPosition);
        DrawCharacter(m_oppositeSprite, m_oppositePosition);
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
