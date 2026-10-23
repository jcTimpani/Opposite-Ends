#include "Game.h"
#include "ComponentIncludes.h"

#include <Windows.h>
#include <string>

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
	m_bGameWon = false;
}

void CGame::LoadImages() {
    m_pRenderer->BeginResourceUpload();

    m_pRenderer->Load(eSprite::PlayButton, "play");
    m_pRenderer->Load(eSprite::Tile, "tile");
    m_pRenderer->Load(eSprite::Wall, "wall");
    m_pRenderer->Load(eSprite::Crate, "crate");
	m_pRenderer->Load(eSprite::Background, "background");
    m_pRenderer->Load(eSprite::PrimaryCharaDown, "astronaut_a_down");
	m_pRenderer->Load(eSprite::PrimaryCharaLeft, "astronaut_a_left");
	m_pRenderer->Load(eSprite::PrimaryCharaRight, "astronaut_a_right"); 
	m_pRenderer->Load(eSprite::PrimaryCharaUp, "astronaut_a_up");
    m_pRenderer->Load(eSprite::OppositeCharaDown, "astronaut_b_down");
	m_pRenderer->Load(eSprite::OppositeCharaLeft, "astronaut_b_left");
	m_pRenderer->Load(eSprite::OppositeCharaRight, "astronaut_b_right");  
	m_pRenderer->Load(eSprite::OppositeCharaUp, "astronaut_b_up");
	m_pRenderer->Load(eSprite::PrimaryGoal, "primary_goal");
	m_pRenderer->Load(eSprite::OppositeGoal, "opposite_goal");
    m_pRenderer->Load(eSprite::SharedGoal, "shared_goal");

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

    if (m_pKeyboard->TriggerDown('R')) {
        m_obstacles.Reset();
        m_facingHistory.clear();
        m_primarySprite = eSprite::PrimaryCharaDown;
        m_oppositeSprite = eSprite::OppositeCharaDown;
        return;
    }
    if (m_pKeyboard->TriggerDown('Z')) {
        if (m_obstacles.Undo() && !m_facingHistory.empty()) {
            const auto previousFacing = m_facingHistory.back();
            m_facingHistory.pop_back();
            m_primarySprite = previousFacing.first;
            m_oppositeSprite = previousFacing.second;
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
    return position.row > 0 && position.row < GRID_SIZE - 1 &&
        position.col > 0 && position.col < GRID_SIZE - 1;
}

void CGame::TryMove(int rowDelta, int colDelta) {
    // Keep facing direction in sync with the pure puzzle state for Undo.
    if (m_obstacles.TryMove(rowDelta, colDelta)) {
        m_facingHistory.emplace_back(m_primarySprite, m_oppositeSprite);
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


void CGame::DrawTile(int row, int col,
    const XMFLOAT4& color, eSprite sprite) {

    LSpriteDesc2D tile;

    // Use the sprite passed into the function
    tile.m_nSpriteIndex = static_cast<UINT>(sprite);

    const float x = col * TILE_SIZE + TILE_SIZE / 2.0f;
    const float y = SCREEN_HEIGHT - (row * TILE_SIZE + TILE_SIZE / 2.0f);

    tile.m_vPos = Vector2(x, y);

    // Get the dimensions of the selected texture
    const float originalWidth = m_pRenderer->GetWidth(sprite);
    const float originalHeight = m_pRenderer->GetHeight(sprite);

    tile.m_fXScale = TILE_SIZE / originalWidth;
    tile.m_fYScale = TILE_SIZE / originalHeight;

    tile.m_f4Tint = color;

    m_pRenderer->Draw(&tile);
}


void CGame::DrawCrate(int row, int col) {
    LSpriteDesc2D crate;

    crate.m_nSpriteIndex = static_cast<UINT>(eSprite::Crate);

    const float x = col * TILE_SIZE + TILE_SIZE / 2.0f;
    const float y = SCREEN_HEIGHT -
        (row * TILE_SIZE + TILE_SIZE / 2.0f);

    crate.m_vPos = Vector2(x, y);

    const float width = m_pRenderer->GetWidth(eSprite::Crate);
    const float height = m_pRenderer->GetHeight(eSprite::Crate);

    crate.m_fXScale = (TILE_SIZE * 0.90f) / width;
    crate.m_fYScale = (TILE_SIZE * 0.90f) / height;

    crate.m_f4Tint = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    m_pRenderer->Draw(&crate);
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

    m_pRenderer->Draw(&character);
}

void CGame::DrawGameGrid() {
    
	// Draw the background
    const XMFLOAT4 gray = XMFLOAT4(0.35f, 0.35f, 0.35f, 1.0f);
    const XMFLOAT4 lightGray = XMFLOAT4(0.65f, 0.65f, 0.65f, 1.0f);
    const XMFLOAT4 wallColor = XMFLOAT4(0.38f, 0.20f, 0.72f, 1.0f);

       LSpriteDesc2D background;
	   background.m_nSpriteIndex = static_cast<UINT>(eSprite::Background);
	   background.m_vPos = Vector2(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);

	   m_pRenderer->Draw(&background);

	   // Draw grid tiles
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            if (row == 0 || row == GRID_SIZE - 1 ||
                col == 0 || col == GRID_SIZE - 1) {
                continue;
            }
            else if (m_obstacles.IsWall(GridPosition{ row, col })) {
                DrawTile(row, col,
                    XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
                    eSprite::Wall);
            }
            else if (m_obstacles.IsDoor(GridPosition{ row, col })) {
                // Red = closed; green = open. Use existing tile art so no
                // additional texture dependency is introduced.
                const bool open = m_obstacles.IsDoorOpen();
                DrawTile(row, col, open
                    ? XMFLOAT4(0.16f, 0.75f, 0.34f, 1.0f)
                    : XMFLOAT4(0.86f, 0.15f, 0.18f, 1.0f));
            }
            else if (m_obstacles.IsPlate(GridPosition{ row, col })) {
                // Each plate lights up only when THIS plate is occupied.
                const bool active = m_obstacles.IsPlateOccupied(GridPosition{ row, col });
                DrawTile(row, col, active
                    ? XMFLOAT4(0.15f, 0.85f, 0.24f, 1.0f)
                    : XMFLOAT4(0.97f, 0.70f, 0.13f, 1.0f));
            }
            else if ((row + col) % 2 == 0) {
				DrawTile(row, col, gray);
            }
            else {
                DrawTile(row, col, lightGray);
            }
            if (m_obstacles.IsCrate(GridPosition{ row, col })) {
                DrawCrate(row, col);
            }
        }
    }

    // Draw goals
    if (m_sharedGoal) {
        DrawSharedGoal(m_primaryGoalPosition);
    }
    else {
        DrawGoals(m_primaryGoalPosition, m_oppositeGoalPosition);
    }
}

// Draw separate goals for each character
void CGame::DrawGoals(const GridPosition& primaryGoalPosition, const GridPosition& oppositeGoalPosition) {
    
    LSpriteDesc2D primaryGoal;
    LSpriteDesc2D oppositeGoal;

	// Draw the primary goal
	primaryGoal.m_nSpriteIndex = static_cast<UINT>(eSprite::PrimaryGoal);

    const float x = primaryGoalPosition.col * TILE_SIZE + TILE_SIZE / 2.0f;
    const float y = SCREEN_HEIGHT - (primaryGoalPosition.row * TILE_SIZE + TILE_SIZE / 2.0f);
    primaryGoal.m_vPos = Vector2(x, y);

    const float width = m_pRenderer->GetWidth(eSprite::PrimaryGoal);
    const float height = m_pRenderer->GetHeight(eSprite::PrimaryGoal);
    primaryGoal.m_fXScale = GOAL_SIZE / width;
    primaryGoal.m_fYScale = GOAL_SIZE / height;

    m_pRenderer->Draw(&primaryGoal);

	// Draw the opposite goal
    oppositeGoal.m_nSpriteIndex = static_cast<UINT>(eSprite::OppositeGoal);

    const float oppositeX = oppositeGoalPosition.col * TILE_SIZE + TILE_SIZE / 2.0f;
    const float oppositeY = SCREEN_HEIGHT - (oppositeGoalPosition.row * TILE_SIZE + TILE_SIZE / 2.0f);
    oppositeGoal.m_vPos = Vector2(oppositeX, oppositeY);

    const float oppositeWidth = m_pRenderer->GetWidth(eSprite::OppositeGoal);
    const float oppositeHeight = m_pRenderer->GetHeight(eSprite::OppositeGoal);
    oppositeGoal.m_fXScale = GOAL_SIZE / oppositeWidth;
    oppositeGoal.m_fYScale = GOAL_SIZE / oppositeHeight;

    m_pRenderer->Draw(&oppositeGoal);
}

// Draw a shared goal for both characters
void CGame::DrawSharedGoal(const GridPosition& goalPosition) {
    LSpriteDesc2D sharedGoal;

    sharedGoal.m_nSpriteIndex = static_cast<UINT>(eSprite::SharedGoal);

    const float x = goalPosition.col * TILE_SIZE + TILE_SIZE / 2.0f;
    const float y = SCREEN_HEIGHT - (goalPosition.row * TILE_SIZE + TILE_SIZE / 2.0f);
    sharedGoal.m_vPos = Vector2(x, y);

    const float width = m_pRenderer->GetWidth(eSprite::SharedGoal);
    const float height = m_pRenderer->GetHeight(eSprite::SharedGoal);
    sharedGoal.m_fXScale = CHARACTER_SIZE / width;
    sharedGoal.m_fYScale = CHARACTER_SIZE / height;

    m_pRenderer->Draw(&sharedGoal);
}

void CGame::RenderFrame() {
    m_pRenderer->BeginFrame();

    if (!m_bGameStarted && !m_bGameWon) {
        DrawStartScreen();
    }
    else {
        DrawGameGrid();
        DrawCharacter(m_primarySprite, m_obstacles.PrimaryPosition());
        DrawCharacter(m_oppositeSprite, m_obstacles.OppositePosition());
        // Split the HUD into two short labels and move both inside the
        // 720x720 view to avoid the previously clipped long text line.
        const std::string hud = "Moves: " + std::to_string(m_obstacles.MoveCount()) +
            (m_obstacles.IsDoorOpen() ? "   Gate: OPEN" : "   Gate: CLOSED");
        m_pRenderer->DrawScreenText(hud.c_str(), Vector2(45.0f, 666.0f), Colors::White);
        m_pRenderer->DrawScreenText("Z: Undo  R: Reset", Vector2(410.0f, 666.0f), Colors::White);
    }

	// Check if the game has been won before ending the frame to ensure the "YOU WIN!" message is displayed immediately and properly
    if (IsGameWon()) { GameWon(); }
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


bool CGame::IsGameWon() const {
    // Shared goal functionality is not implemented yet.
    if (m_sharedGoal) {
        return false;
    }

    // Get character positions from the obstacle system.
    const GridPosition primary = m_obstacles.PrimaryPosition();
    const GridPosition opposite = m_obstacles.OppositePosition();

    // Check whether both characters reached their goals.
    return primary.row == m_primaryGoalPosition.row &&
        primary.col == m_primaryGoalPosition.col &&
        opposite.row == m_oppositeGoalPosition.row &&
        opposite.col == m_oppositeGoalPosition.col;
}


void CGame::GameWon() {

	m_bGameStarted = false;
	m_bGameWon = true;

    m_pRenderer->DrawScreenText(
        "YOU WIN!",
        Vector2(300.0f, 360.0f),
        Colors::White
	);
}