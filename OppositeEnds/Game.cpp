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

bool CGame::MouseOverRect(float x, float y,
    float width, float height) {
    POINT mouse{};
    if (!GetCursorPos(&mouse)) return false;

    HWND window = GetActiveWindow();
    if (!window) return false;
    if (!ScreenToClient(window, &mouse)) return false;

    RECT client{};
    if (!GetClientRect(window, &client)) return false;
    const float clientWidth = float(client.right - client.left);
    const float clientHeight = float(client.bottom - client.top);
    if (clientWidth <= 0 || clientHeight <= 0) return false;

    // Convert window pixels to the logical 720x720 menu coordinates.
    const float mx = float(mouse.x) * SCREEN_WIDTH / clientWidth;
    const float my = SCREEN_HEIGHT - float(mouse.y) * SCREEN_HEIGHT / clientHeight;
    return mx >= x - width * 0.5f && mx <= x + width * 0.5f &&
        my >= y - height * 0.5f && my <= y + height * 0.5f;
}

void CGame::DrawMenuButton(const char* label,
    float x, float y, float width, float height, float textX) {
    const bool hover = MouseOverRect(x, y, width, height);
    const eSprite sprite = hover ? eSprite::MenuButtonHover : eSprite::MenuButton;

    LSpriteDesc2D button{};
    button.m_nSpriteIndex = static_cast<UINT>(sprite);
    button.m_vPos = Vector2(x, y);
    button.m_fXScale = width / m_pRenderer->GetWidth(sprite);
    button.m_fYScale = height / m_pRenderer->GetHeight(sprite);
    button.m_f4Tint = XMFLOAT4(1, 1, 1, 1);
    m_pRenderer->Draw(&button);

    // The sprite position uses bottom-up Y, whereas DrawScreenText is top-down.
    // Offset the text by approximately half of the 24px spritefont height.
    const float textTop = SCREEN_HEIGHT - y - 13.0f;
    m_pRenderer->DrawScreenText(label, Vector2(textX, textTop), Colors::White);
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
            m_bGameLost = false;
            m_bGameStarted = true;
            m_primarySprite = eSprite::PrimaryCharaDown;
            m_oppositeSprite = eSprite::OppositeCharaDown;
            m_facingHistory.clear();
            m_hintText.clear();
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
    m_pRenderer->Load(eSprite::Ice, "ice");
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
    m_pRenderer->Load(eSprite::MenuTitle, "menu_title");
    m_pRenderer->Load(eSprite::MenuButton, "menu_button");
    m_pRenderer->Load(eSprite::MenuButtonHover, "menu_button_hover");
    m_pRenderer->Load(eSprite::MenuSettings, "menu_settings");
    m_pRenderer->Load(eSprite::MenuInfo, "menu_info");
    m_pRenderer->EndResourceUpload();
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
            m_hintText.clear();
            m_primarySprite = eSprite::PrimaryCharaDown;
            m_oppositeSprite = eSprite::OppositeCharaDown;
        }
        else if (m_pKeyboard->TriggerDown(VK_RETURN)) {
            if (!LoadLevel(m_levelNumber + 1))
                m_allLevelsComplete = m_levelError.find("Cannot open ") == 0;
        }
        return;
    }

    if (m_bGameLost) {
        if (m_pKeyboard->TriggerDown('R')) {
            m_obstacles.Reset();
            m_bGameLost = false;
            m_bGameStarted = true;
            m_facingHistory.clear();
            m_primarySprite = eSprite::PrimaryCharaDown;
            m_oppositeSprite = eSprite::OppositeCharaDown;
        }
        return;
    }


    if (!m_bGameStarted) {
        const bool click = m_pKeyboard->TriggerDown(VK_LBUTTON);
        if (m_menuPage == MenuPage::Main) {
            if ((click && MouseOverRect(360, 405, 342, 76)) ||
                m_pKeyboard->TriggerDown(VK_RETURN)) {
                LoadLevel(1);
            }
            else if (click && MouseOverRect(360, 305, 342, 76)) {
                m_menuPage = MenuPage::LevelSelect;
            }
            else if (click && MouseOverRect(360, 205, 342, 76)) {
                PostMessage(GetActiveWindow(), WM_CLOSE, 0, 0);
            }
            else if (click && MouseOverRect(68, 650, 66, 66)) {
                m_menuPage = MenuPage::Settings;
            }
            else if (click && MouseOverRect(652, 650, 66, 66)) {
                m_menuPage = MenuPage::HowToPlay;
            }
        }
        else if (m_menuPage == MenuPage::HowToPlay) {
            if (click && MouseOverRect(360, 115, 260, 72))
                m_menuPage = MenuPage::Main;
        }
        else if (m_menuPage == MenuPage::Settings) {
            if (click && MouseOverRect(360, 325, 342, 76))
                m_showControls = !m_showControls;
            else if (click && MouseOverRect(360, 150, 260, 72))
                m_menuPage = MenuPage::Main;
        }
        else if (m_menuPage == MenuPage::LevelSelect && click) {
            for (int level = 1; level <= 5; ++level) {
                const float x = 120.0f + (level - 1) * 115.0f;
                if (MouseOverRect(x, 360, 90, 70)) {
                    LoadLevel(level);
                    break;
                }
            }
            if (!m_bGameStarted && MouseOverRect(360, 180, 260, 72))
                m_menuPage = MenuPage::Main;
        }
        return;
    }

    if (m_pKeyboard->TriggerDown('R')) {
        m_obstacles.Reset();
        m_facingHistory.clear();
        m_hintText.clear();
        m_primarySprite = eSprite::PrimaryCharaDown;
        m_oppositeSprite = eSprite::OppositeCharaDown;
        return;
    }
    if (m_pKeyboard->TriggerDown('Z')) {
        if (m_obstacles.Undo() && !m_facingHistory.empty()) {
            m_hintText.clear();
            const auto previousFacing = m_facingHistory.back();
            m_facingHistory.pop_back();
            m_primarySprite = previousFacing.first;
            m_oppositeSprite = previousFacing.second;
        }
        return;
    }

    // H asks the exact puzzle solver for one next move. It never moves
    // either player or modifies the undo history or move count.
    if (m_pKeyboard->TriggerDown('H')) {
        const auto hint = PuzzleSolver::Solve(m_obstacles);
        if (hint.status == PuzzleSolver::Status::Solved) {
            m_hintText = hint.moves.empty() ? "Already solved" :
                "Hint: " + std::string(1, hint.moves[0]) +
                "  (" + std::to_string(hint.moves.size()) + " moves remaining)";
        }
        else if (hint.status == PuzzleSolver::Status::StateLimit) {
            m_hintText = "Hint: search limit reached";
        }
        else {
            m_hintText = "Hint: no solution from here";
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

    // use the existing movement system.
    if (m_obstacles.TryMove(dr, dc)) {
        m_facingHistory.emplace_back(
            m_primarySprite,
            m_oppositeSprite
        );
        m_hintText.clear();
    }
}

Vector2 CGame::CellCenter(int row, int col) const {
    const float left = (SCREEN_WIDTH - m_obstacles.Width() * TILE_SIZE) / 2.0f;
    const float top = (SCREEN_HEIGHT - m_obstacles.Height() * TILE_SIZE) / 2.0f;
    return Vector2(left + (col + 0.5f) * TILE_SIZE,
        SCREEN_HEIGHT - (top + (row + 0.5f) * TILE_SIZE));
}


void CGame::DrawStartScreen() {
    auto drawSprite = [this](eSprite sprite, float x, float y,
        float width, float height) {
            LSpriteDesc2D desc{};
            desc.m_nSpriteIndex = static_cast<UINT>(sprite);
            desc.m_vPos = Vector2(x, y);
            desc.m_fXScale = width / m_pRenderer->GetWidth(sprite);
            desc.m_fYScale = height / m_pRenderer->GetHeight(sprite);
            desc.m_f4Tint = XMFLOAT4(1, 1, 1, 1);
            m_pRenderer->Draw(&desc);
        };

    if (m_menuPage == MenuPage::Main) {
        // A single title sprite avoids the tiny default font and allows
        // the two title colors to remain correctly spaced.
        drawSprite(eSprite::MenuTitle, 360, 580, 570, 109);
        m_pRenderer->DrawScreenText(
            "TWO ASTRONAUTS. OPPOSITE MOVES.",
            Vector2(86, 208), Colors::White);

        // The buttons are positioned using the sprite renderer's bottom-up Y.
        DrawMenuButton("PLAY", 360, 405, 342, 76, 327);
        DrawMenuButton("LEVEL SELECT", 360, 305, 342, 76, 273);
        DrawMenuButton("QUIT", 360, 205, 342, 76, 327);

        drawSprite(eSprite::MenuSettings, 68, 650, 66, 66);
        drawSprite(eSprite::MenuInfo, 652, 650, 66, 66);
        m_pRenderer->DrawScreenText(
            "SELECT A LEVEL OR PRESS ENTER TO PLAY",
            Vector2(55, 620), Colors::White);
    }
    else if (m_menuPage == MenuPage::HowToPlay) {
        m_pRenderer->DrawScreenText("HOW TO PLAY", Vector2(260, 110), Colors::Cyan);
        m_pRenderer->DrawScreenText("Move red: WASD or arrow keys", Vector2(80, 195), Colors::White);
        m_pRenderer->DrawScreenText("Blue moves in the opposite direction", Vector2(80, 245), Colors::White);
        m_pRenderer->DrawScreenText("Guide both astronauts to their goals", Vector2(80, 295), Colors::White);
        m_pRenderer->DrawScreenText("Push crates onto pressure plates", Vector2(80, 345), Colors::White);
        m_pRenderer->DrawScreenText("Z: Undo    R: Reset    H: Hint", Vector2(80, 395), Colors::White);
        DrawMenuButton("BACK", 360, 115, 260, 72, 328);
    }
    else if (m_menuPage == MenuPage::Settings) {
        m_pRenderer->DrawScreenText("SETTINGS", Vector2(290, 110), Colors::Cyan);
        m_pRenderer->DrawScreenText("GAMEPLAY OPTIONS", Vector2(242, 230), Colors::White);
        DrawMenuButton(m_showControls ? "CONTROLS: ON" : "CONTROLS: OFF",
            360, 325, 342, 76, 270);
        DrawMenuButton("BACK", 360, 150, 260, 72, 328);
    }
    else if (m_menuPage == MenuPage::LevelSelect) {
        m_pRenderer->DrawScreenText("SELECT LEVEL", Vector2(255, 110), Colors::Cyan);
        for (int level = 1; level <= 5; ++level) {
            const float x = 120.0f + (level - 1) * 115.0f;
            const std::string name = std::to_string(level);
            DrawMenuButton(name.c_str(), x, 360, 90, 70, x - 8);
        }
        DrawMenuButton("BACK", 360, 180, 260, 72, 328);
    }
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
            else if (m_obstacles.IsIceTile(pos)) {
                DrawTile(row, col, XMFLOAT4(0.5f, 0.8f, 1.0f, 1.0f), eSprite::Ice);
            }
            // make sure to draw the tiles under goals and start positions
            else if (m_obstacles.IsTile(pos) || m_obstacles.IsPrimaryGoal(pos) || m_obstacles.IsOppositeGoal(pos) || m_obstacles.IsPrimaryStart(pos) || m_obstacles.IsOppositeStart(pos) || m_obstacles.IsCrateStart(pos)) {
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
        // Keep the labels on separate short lines so they remain in the 720px window.
        if (m_showControls) {
            m_pRenderer->DrawScreenText("Z: Undo  R: Reset  H: Hint", Vector2(38, 628), Colors::White);
        }
        if (!m_hintText.empty())
            m_pRenderer->DrawScreenText(m_hintText.c_str(), Vector2(38, 591), Colors::White);
        if (m_bGameWon) {
            const char* message = m_allLevelsComplete ? "ALL LEVELS COMPLETE!" :
                (!m_levelError.empty() ? "NEXT LEVEL FILE INVALID" :
                    "LEVEL COMPLETE! ENTER: NEXT");
            m_pRenderer->DrawScreenText(message, Vector2(130, 355), Colors::White);
        }
        if (isGameLost()) {
            m_pRenderer->DrawScreenText("GAME LOST! R: RESET", Vector2(130, 355), Colors::White);
        }
    }
    m_pRenderer->EndFrame();
}

void CGame::ProcessFrame() {
    KeyboardHandler();
    if (m_bGameStarted && !m_bGameWon && IsGameWon()) {
        GameWon();
    }
    else if (m_bGameStarted && !m_bGameLost && isGameLost()) {
        GameLost();
    }
    RenderFrame();
}

bool CGame::IsGameWon() const { return m_obstacles.IsWon(); }

void CGame::GameWon() {
    m_bGameWon = true;
    m_bGameLost = false;
}

bool CGame::isGameLost() const { return m_obstacles.IsLost(); }

void CGame::GameLost() {
    m_bGameLost = true;
    m_bGameWon = false;
}

void CGame::Release() {
    delete m_pRenderer;
    m_pRenderer = nullptr;
}
