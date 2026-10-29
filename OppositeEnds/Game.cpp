#include "Game.h"
#include "ComponentIncludes.h"

#include <Windows.h>
#include <iomanip>
#include <sstream>
#include <string>

void CGame::Initialize() {
    m_pRenderer = new LSpriteRenderer(eSpriteMode::Batched2D);
    m_pRenderer->Initialize(eSprite::Size);
    const XMVECTORF32 background = { 0.05f, 0.06f, 0.10f, 1.0f };
    m_pRenderer->SetBgColor(background);
    LoadImages();
    m_levelLoaded = LoadLevel(1);
    m_bGameStarted = false;
    m_bGameWon = false;
}

bool CGame::LoadLevel(int number) {
    std::ostringstream name;
    name << "Levels/level" << std::setfill('0') << std::setw(2) << number << ".txt";

    // VS debugger working directories vary by solution/project configuration.
    // Try the solution root and up to three parent folders.
    const std::string prefixes[] = { "", "../", "../../", "../../../" };
    std::string error;
    for (const std::string& prefix : prefixes) {
        if (m_obstacles.LoadFromFile(prefix + name.str(), &error)) {
            m_levelNumber = number;
            m_levelLoaded = true;
            m_levelError.clear();
            m_allLevelsComplete = false;
            m_bGameWon = false;
            m_bGameStarted = true;
            m_primarySprite = eSprite::PrimaryCharaDown;
            m_oppositeSprite = eSprite::OppositeCharaDown;
            m_facingHistory.clear();
            return true;
        }
        // A found but invalid file is an error, not a missing next level.
        if (error.find("Cannot open ") != 0) break;
    }
    m_levelError = error;
    return false;
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
    m_pRenderer->Load(eSprite::FlashOverlay, "flash_overlay");
    m_pRenderer->EndResourceUpload();
}

bool CGame::MouseOverPlayButton() {
    POINT mouse;
    if (!GetCursorPos(&mouse)) return false;
    HWND window = GetActiveWindow();
    if (!window) return false;
    ScreenToClient(window, &mouse);
    const float mx = static_cast<float>(mouse.x);
    const float my = SCREEN_HEIGHT - static_cast<float>(mouse.y);
    return mx >= BUTTON_X - BUTTON_WIDTH / 2 && mx <= BUTTON_X + BUTTON_WIDTH / 2 &&
        my >= BUTTON_Y - BUTTON_HEIGHT / 2 && my <= BUTTON_Y + BUTTON_HEIGHT / 2;
}

void CGame::KeyboardHandler() {
    m_pKeyboard->GetState();
    if (!m_levelLoaded) return;

    if (m_bGameWon) {
        if (m_pKeyboard->TriggerDown('R')) {
            m_obstacles.Reset();
            m_bGameWon = false;
            m_bGameStarted = true;
            m_allLevelsComplete = false;
            m_facingHistory.clear();
            m_primarySprite = eSprite::PrimaryCharaDown;
            m_oppositeSprite = eSprite::OppositeCharaDown;
        }
        else if (m_pKeyboard->TriggerDown(VK_RETURN)) {
            if (!LoadLevel(m_levelNumber + 1))
                m_allLevelsComplete = m_levelError.find("Cannot open ") == 0;
        }
        return;
    }

    if (!m_bGameStarted) {
        if ((m_pKeyboard->TriggerDown(VK_LBUTTON) && MouseOverPlayButton()) ||
            m_pKeyboard->TriggerDown(VK_RETURN)) m_bGameStarted = true;
        return;
    }
    // Freeze controls during the red flash.
    if (std::chrono::steady_clock::now() < m_edgeFlashUntil) {
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

    if (m_pKeyboard->TriggerDown('W') || m_pKeyboard->TriggerDown(VK_UP)) {
        TryMove(-1, 0);
        m_primarySprite = eSprite::PrimaryCharaUp;
        m_oppositeSprite = eSprite::OppositeCharaDown;
    }
    else if (m_pKeyboard->TriggerDown('A') || m_pKeyboard->TriggerDown(VK_LEFT)) {
        TryMove(0, -1);
        m_primarySprite = eSprite::PrimaryCharaLeft;
        m_oppositeSprite = eSprite::OppositeCharaRight;
    }
    else if (m_pKeyboard->TriggerDown('S') || m_pKeyboard->TriggerDown(VK_DOWN)) {
        TryMove(1, 0);
        m_primarySprite = eSprite::PrimaryCharaDown;
        m_oppositeSprite = eSprite::OppositeCharaUp;
    }
    else if (m_pKeyboard->TriggerDown('D') || m_pKeyboard->TriggerDown(VK_RIGHT)) {
        TryMove(0, 1);
        m_primarySprite = eSprite::PrimaryCharaRight;
        m_oppositeSprite = eSprite::OppositeCharaLeft;
    }
}

void CGame::TryMove(int dr, int dc) {
    const auto now = std::chrono::steady_clock::now();

    // Don't allow movement while the screen is flashing.
    if (now < m_edgeFlashUntil)
        return;

    const GridPosition red = m_obstacles.PrimaryPosition();
    const GridPosition blue = m_obstacles.OppositePosition();

    // Calculate where each character is trying to go.
    const GridPosition redNext{
        red.row + dr,
        red.col + dc
    };

    const GridPosition blueNext{
        blue.row - dr,
        blue.col - dc
    };

    // Check whether a position is outside the board
    // or is one of the outer boundary walls.
    auto hitsEdge = [this](const GridPosition& pos) {
        const int height = m_obstacles.Height();
        const int width = m_obstacles.Width();

        if (pos.row < 0 || pos.row >= height ||
            pos.col < 0 || pos.col >= width) {
            return true;
        }

        const bool onBorder =
            pos.row == 0 ||
            pos.row == height - 1 ||
            pos.col == 0 ||
            pos.col == width - 1;

        return onBorder && m_obstacles.IsWall(pos);
        };

    // If EITHER character hits the edge,
    // cancel the entire move and trigger the flash.
    if (hitsEdge(redNext) || hitsEdge(blueNext)) {
        m_edgeFlashUntil = now + std::chrono::seconds(1);
        return;
    }

    // Otherwise, use the existing movement system.
    if (m_obstacles.TryMove(dr, dc)) {
        m_facingHistory.emplace_back(
            m_primarySprite,
            m_oppositeSprite
        );
    }
}

Vector2 CGame::CellCenter(int row, int col) const {
    const float left = (SCREEN_WIDTH - m_obstacles.Width() * TILE_SIZE) / 2.0f;
    const float top = (SCREEN_HEIGHT - m_obstacles.Height() * TILE_SIZE) / 2.0f;
    return Vector2(left + (col + 0.5f) * TILE_SIZE,
        SCREEN_HEIGHT - (top + (row + 0.5f) * TILE_SIZE));
}

void CGame::DrawStartScreen() {
    m_pRenderer->DrawScreenText("OPPOSITE ENDS", Vector2(270.0f, 180.0f), Colors::White);
    LSpriteDesc2D button;
    button.m_nSpriteIndex = static_cast<UINT>(eSprite::PlayButton);
    button.m_vPos = Vector2(BUTTON_X, BUTTON_Y);
    button.m_fXScale = BUTTON_WIDTH / m_pRenderer->GetWidth(eSprite::PlayButton);
    button.m_fYScale = BUTTON_HEIGHT / m_pRenderer->GetHeight(eSprite::PlayButton);
    button.m_f4Tint = MouseOverPlayButton()
        ? XMFLOAT4(0.7f, 0.8f, 1.0f, 1.0f) : XMFLOAT4(1, 1, 1, 1);
    m_pRenderer->Draw(&button);
}

void CGame::DrawTile(int row, int col, const XMFLOAT4& color, eSprite sprite) {
    LSpriteDesc2D tile;
    tile.m_nSpriteIndex = static_cast<UINT>(sprite);
    tile.m_vPos = CellCenter(row, col);
    tile.m_fXScale = TILE_SIZE / m_pRenderer->GetWidth(sprite);
    tile.m_fYScale = TILE_SIZE / m_pRenderer->GetHeight(sprite);
    tile.m_f4Tint = color;
    m_pRenderer->Draw(&tile);
}

void CGame::DrawCrate(int row, int col) {
    LSpriteDesc2D crate;
    crate.m_nSpriteIndex = static_cast<UINT>(eSprite::Crate);
    crate.m_vPos = CellCenter(row, col);
    crate.m_fXScale = TILE_SIZE * 0.90f / m_pRenderer->GetWidth(eSprite::Crate);
    crate.m_fYScale = TILE_SIZE * 0.90f / m_pRenderer->GetHeight(eSprite::Crate);
    crate.m_f4Tint = XMFLOAT4(1, 1, 1, 1);
    m_pRenderer->Draw(&crate);
}

void CGame::DrawCharacter(eSprite sprite, const GridPosition& position) {
    LSpriteDesc2D character;
    character.m_nSpriteIndex = static_cast<UINT>(sprite);
    character.m_vPos = CellCenter(position.row, position.col);
    character.m_fXScale = CHARACTER_SIZE / m_pRenderer->GetWidth(sprite);
    character.m_fYScale = CHARACTER_SIZE / m_pRenderer->GetHeight(sprite);
    m_pRenderer->Draw(&character);
}

void CGame::DrawGoals(const GridPosition& primaryPosition, const GridPosition& oppositePosition) {
    LSpriteDesc2D goal;
    goal.m_nSpriteIndex = static_cast<UINT>(eSprite::PrimaryGoal);
    goal.m_vPos = CellCenter(primaryPosition.row, primaryPosition.col);
    goal.m_fXScale = GOAL_SIZE / m_pRenderer->GetWidth(eSprite::PrimaryGoal);
    goal.m_fYScale = GOAL_SIZE / m_pRenderer->GetHeight(eSprite::PrimaryGoal);
    m_pRenderer->Draw(&goal);

    goal.m_nSpriteIndex = static_cast<UINT>(eSprite::OppositeGoal);
    goal.m_vPos = CellCenter(oppositePosition.row, oppositePosition.col);
    goal.m_fXScale = GOAL_SIZE / m_pRenderer->GetWidth(eSprite::OppositeGoal);
    goal.m_fYScale = GOAL_SIZE / m_pRenderer->GetHeight(eSprite::OppositeGoal);
    m_pRenderer->Draw(&goal);
}

void CGame::DrawSharedGoal(const GridPosition& position) {
    LSpriteDesc2D goal;
    goal.m_nSpriteIndex = static_cast<UINT>(eSprite::SharedGoal);
    goal.m_vPos = CellCenter(position.row, position.col);
    goal.m_fXScale = GOAL_SIZE / m_pRenderer->GetWidth(eSprite::SharedGoal);
    goal.m_fYScale = GOAL_SIZE / m_pRenderer->GetHeight(eSprite::SharedGoal);
    m_pRenderer->Draw(&goal);
}

void CGame::DrawGameGrid() {
    const XMFLOAT4 gray(0.35f, 0.35f, 0.35f, 1.0f);
    const XMFLOAT4 lightGray(0.65f, 0.65f, 0.65f, 1.0f);
    const XMFLOAT4 white(1, 1, 1, 1);

    LSpriteDesc2D background;
    background.m_nSpriteIndex = static_cast<UINT>(eSprite::Background);
    background.m_vPos = Vector2(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f);
    m_pRenderer->Draw(&background);

    for (int row = 0; row < m_obstacles.Height(); ++row) {
        for (int col = 0; col < m_obstacles.Width(); ++col) {
            const GridPosition pos{ row, col };
            if (m_obstacles.IsWall(pos)) {
                DrawTile(row, col, white, eSprite::Wall);
            }
            else if (m_obstacles.IsDoor(pos)) {
                DrawTile(row, col, m_obstacles.IsDoorOpen()
                    ? XMFLOAT4(0.16f, 0.75f, 0.34f, 1.0f)
                    : XMFLOAT4(0.86f, 0.15f, 0.18f, 1.0f));
            }
            else if (m_obstacles.IsPlate(pos)) {
                DrawTile(row, col, m_obstacles.IsPlateOccupied(pos)
                    ? XMFLOAT4(0.15f, 0.85f, 0.24f, 1.0f)
                    : XMFLOAT4(0.97f, 0.70f, 0.13f, 1.0f));
            }
            else {
                DrawTile(row, col, (row + col) % 2 == 0 ? gray : lightGray);
            }
        }
    }
    DrawGoals(m_obstacles.PrimaryGoal(), m_obstacles.OppositeGoal());
    for (int row = 0; row < m_obstacles.Height(); ++row)
        for (int col = 0; col < m_obstacles.Width(); ++col)
            if (m_obstacles.IsCrate({ row, col })) DrawCrate(row, col);
}

void CGame::RenderFrame() {
    m_pRenderer->BeginFrame();
    if (!m_levelLoaded) {
        m_pRenderer->DrawScreenText("ERROR: Level file not found or invalid", Vector2(35, 360), Colors::White);
        m_pRenderer->DrawScreenText("Check Levels/level01.txt", Vector2(35, 325), Colors::White);
    }
    else if (!m_bGameStarted && !m_bGameWon) {
        DrawStartScreen();
    }
    else {
        DrawGameGrid();
        DrawCharacter(m_primarySprite, m_obstacles.PrimaryPosition());
        DrawCharacter(m_oppositeSprite, m_obstacles.OppositePosition());
        std::string hud = "Level " + std::to_string(m_levelNumber) +
            "  Moves: " + std::to_string(m_obstacles.MoveCount());
        
        m_pRenderer->DrawScreenText(hud.c_str(), Vector2(38, 666), Colors::White);
        m_pRenderer->DrawScreenText("Z: Undo  R: Reset", Vector2(410, 666), Colors::White);
        if (m_bGameWon) {
            const char* message = m_allLevelsComplete ? "ALL LEVELS COMPLETE!" :
                (!m_levelError.empty() ? "NEXT LEVEL FILE INVALID" :
                    "LEVEL COMPLETE! ENTER: NEXT");
            m_pRenderer->DrawScreenText(message, Vector2(130, 355), Colors::White);
        }
    }
    // Transparent red screen flash
    const auto now = std::chrono::steady_clock::now();

    if (m_bGameStarted && now < m_edgeFlashUntil) {
        LSpriteDesc2D flash;

        flash.m_nSpriteIndex =
            static_cast<UINT>(eSprite::FlashOverlay);

        flash.m_vPos = Vector2(
            SCREEN_WIDTH / 2.0f,
            SCREEN_HEIGHT / 2.0f
        );

        // Cover the entire screen
        flash.m_fXScale =
            static_cast<float>(SCREEN_WIDTH) /
            m_pRenderer->GetWidth(eSprite::FlashOverlay);

        flash.m_fYScale =
            static_cast<float>(SCREEN_HEIGHT) /
            m_pRenderer->GetHeight(eSprite::FlashOverlay);

        // Red tint
        flash.m_f4Tint = XMFLOAT4(
            1.0f,
            0.0f,
            0.0f,
            1.0f
        );

        // 25% opacity
        flash.m_fAlpha = 0.25f;
        m_pRenderer->Draw(&flash);
    }
    m_pRenderer->EndFrame();
}

void CGame::ProcessFrame() {
    KeyboardHandler();
    if (m_bGameStarted && !m_bGameWon && IsGameWon()) GameWon();
    RenderFrame();
}

bool CGame::IsGameWon() const { return m_obstacles.IsWon(); }
void CGame::GameWon() { m_bGameStarted = false; m_bGameWon = true; }

void CGame::Release() {
    delete m_pRenderer;
    m_pRenderer = nullptr;
}
