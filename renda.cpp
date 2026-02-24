#include <windows.h>
#include <stdio.h>
#include <signal.h>

HHOOK hHook;

bool enabled = false;
bool autoClicking = false;
volatile sig_atomic_t running = 1;

DWORD WINAPI ClickThread(LPVOID)
{
    INPUT inputDown = {};
    INPUT inputUp = {};

    inputDown.type = INPUT_MOUSE;
    inputDown.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    inputUp.type = INPUT_MOUSE;
    inputUp.mi.dwFlags = MOUSEEVENTF_LEFTUP;

    while (autoClicking && running)
    {
        SendInput(1, &inputDown, sizeof(INPUT));
        SendInput(1, &inputUp, sizeof(INPUT));
        Sleep(7);
    }

    // ★ 停止時に必ず解放
    SendInput(1, &inputUp, sizeof(INPUT));

    return 0;
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && running)
    {
        MSLLHOOKSTRUCT* p = (MSLLHOOKSTRUCT*)lParam;

        // 自己入力を無視
        if (p->flags & LLMHF_INJECTED)
            return CallNextHookEx(hHook, nCode, wParam, lParam);

        switch (wParam)
        {
        case WM_MBUTTONDOWN:
            enabled = !enabled;
            printf("AutoClick: %s\n", enabled ? "ON" : "OFF");

            if (!enabled)
                autoClicking = false;
            break;

        case WM_LBUTTONDOWN:
            if (enabled && !autoClicking)
            {
                autoClicking = true;
                CreateThread(NULL, 0, ClickThread, NULL, 0, NULL);
            }
            break;

        case WM_LBUTTONUP:
            autoClicking = false;
            break;
        }
    }

    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

void handle_sigint(int)
{
    printf("\nSIGINT received. Exiting...\n");
    running = 0;
    autoClicking = false;
}

int main()
{
    printf("Toggle AutoClicker\n");
    printf("Middle Click : ON/OFF\n");
    printf("Hold Left Click while ON to auto-click\n");
    printf("Press Ctrl+C to exit\n");

    signal(SIGINT, handle_sigint);

    hHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);

    MSG msg;

    while (running)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(10);
    }

    autoClicking = false;

    if (hHook)
        UnhookWindowsHookEx(hHook);

    return 0;
}