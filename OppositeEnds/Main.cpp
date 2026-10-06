#include "Game.h"
#include "Window.h"

static LWindow g_cWindow;
static CGame g_cGame;

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE,
    _In_ PWSTR,
    _In_ int
) {
    return g_cWindow.WinMain(
        hInstance,
        false,

        []() {
            g_cGame.Initialize();
        },

        []() {
            g_cGame.ProcessFrame();
        },

        []() {
            g_cGame.Release();
        }
    );
}