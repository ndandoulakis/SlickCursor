#include "common.h"
#include "emuparams.h"
#include "resource.h"
#include "windows.h"

static char MAIN_CLASS[] = "SLICKCURSOR_MAIN_CLASS";
static char MAIN_TITLE[] = "SlickCursor 0.2a";
static char TOOL_CLASS[] = "SLICKCURSOR_TOOL_CLASS";
static char TOOL_TITLE[] = "SlickCursor - Inspector";

static HWND CreateMainWindow(HINSTANCE);
static HWND CreateToolWindow(HINSTANCE);
static HMENU CreatePopupMenu(bool, bool, bool);
static LRESULT CALLBACK MainWindowProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK ToolWindowProc(HWND, UINT, WPARAM, LPARAM);
static bool AcquireAppMutex();
static void GoSysTray(HWND);
static void LeaveSysTray(HWND);

static HWND hMain;
static HWND hTool;

extern EmuParams emu;

extern bool BeginMouseEmulation();
extern void EndMouseEmulation();
extern void SetScreenObserver(HWND);
extern void DrawAnalysisData(HDC);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    // One app instance per user
    if (!AcquireAppMutex()) {
        return 0;
    }

    hMain = CreateMainWindow(hInstance);
    if (!hMain) {
        return 0;
    }

    hTool = CreateToolWindow(hInstance);
    SetScreenObserver(hTool);

    if (!BeginMouseEmulation()) {
        return 0;
    }

    emu.LoadParams("SlickCursor");

    GoSysTray(hMain);

    MSG messages;
    while (GetMessage(&messages, NULL, 0, 0)) {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }

    LeaveSysTray(hMain);

    EndMouseEmulation();

    emu.SaveParams("SlickCursor");

    return messages.wParam;
}

static HWND CreateMainWindow(HINSTANCE hInst)
{
    WNDCLASSEX wcx;
    wcx.hInstance = hInst;
    wcx.lpszClassName = MAIN_CLASS;
    wcx.lpfnWndProc = MainWindowProc;
    wcx.style = CS_DBLCLKS;
    wcx.cbSize = sizeof (WNDCLASSEX);
    wcx.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDR_ICO_MAIN));
    wcx.hIconSm = NULL;
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcx.lpszMenuName = NULL;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    if (!RegisterClassEx(&wcx)) {
        return 0;
    }

    HWND hwnd = CreateWindowEx(
           0,
           MAIN_CLASS,
           MAIN_TITLE,
           WS_OVERLAPPEDWINDOW,
           CW_USEDEFAULT, // x
           CW_USEDEFAULT, // y
           210, // width
           0, // height
           NULL, // parent
           NULL, // menu
           hInst,
           NULL
           );

    return hwnd;
}

static HWND CreateToolWindow(HINSTANCE hInst)
{
    WNDCLASSEX wcx;
    wcx.hInstance = hInst;
    wcx.lpszClassName = TOOL_CLASS;
    wcx.lpfnWndProc = ToolWindowProc;
    wcx.style = CS_DBLCLKS;
    wcx.cbSize = sizeof (WNDCLASSEX);
    wcx.hIcon = NULL;
    wcx.hIconSm = NULL;
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcx.lpszMenuName = NULL;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    if (!RegisterClassEx(&wcx)) {
        return 0;
    }

    HWND hwnd = CreateWindowEx(
           WS_EX_PALETTEWINDOW|WS_EX_COMPOSITED,
           TOOL_CLASS,
           TOOL_TITLE,
           WS_OVERLAPPED|WS_SYSMENU,
           CW_USEDEFAULT, // x
           CW_USEDEFAULT, // y
           256, // width
           256, // height
           NULL, // parent
           NULL, // menu
           hInst,
           NULL
           );

    return hwnd;
}

static HMENU CreatePopupMenu(bool arrows, bool numpad, bool tool)
{
    HMENU hMenu = CreatePopupMenu();
    InsertMenu(hMenu, -1, MF_BYPOSITION|MF_STRING|(arrows?MF_CHECKED:MF_UNCHECKED), 2, "Use arrow keys");
    InsertMenu(hMenu, -1, MF_BYPOSITION|MF_STRING|(numpad?MF_CHECKED:MF_UNCHECKED), 3, "Use numpad keys");
    InsertMenu(hMenu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, 0);
    InsertMenu(hMenu, -1, MF_BYPOSITION|MF_STRING|(tool?MF_CHECKED:MF_UNCHECKED), 4, "Show Inspector");
    InsertMenu(hMenu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, 0);
    InsertMenu(hMenu, -1, MF_BYPOSITION|MF_STRING, 1, "Exit");

    return hMenu;
}

static LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_SYSTRAY_EVENT:
            switch (lParam) {
                case WM_LBUTTONUP:
                case WM_RBUTTONUP:
                    bool toolVisible = IsWindowVisible(hTool);

                    POINT pt;
                    GetCursorPos(&pt);

                    SetForegroundWindow(hwnd);

                    HMENU hMenu = CreatePopupMenu(emu.useArrows, emu.useNumpad, toolVisible);
                    BOOL id = TrackPopupMenu(hMenu, TPM_RIGHTBUTTON|TPM_BOTTOMALIGN|TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
                    DestroyMenu(hMenu);

                    PostMessage(hwnd, WM_NULL, 0, 0);

                    if (id == 1)
                        PostMessage(hwnd, WM_CLOSE, 0, 0);
                    else if (id == 2)
                        emu.useArrows = !emu.useArrows;
                    else if (id == 3)
                        emu.useNumpad = !emu.useNumpad;
                    else if (id == 4)
                        ShowWindow(hTool, toolVisible? SW_HIDE : SW_SHOW);

                    break;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}

static LRESULT CALLBACK ToolWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message) {
        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            DrawAnalysisData(hdc);
            EndPaint(hwnd, &ps);
            break;

        case WM_SCREEN_OBSERVER:
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_CLOSE:
            ShowWindow(hTool, SW_HIDE);
            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}

static bool AcquireAppMutex()
{
    HANDLE mutex = CreateMutex(NULL, FALSE, MAIN_TITLE);

    if (!mutex) {
        return false;
    }

    if (ERROR_ALREADY_EXISTS == GetLastError()) {
        CloseHandle(mutex);
        return false;
    }

    return true;
}

static void GoSysTray(HWND hwnd)
{
    NOTIFYICONDATA data = {0};

    data.cbSize = sizeof(NOTIFYICONDATA);
    data.hWnd = hwnd;
    data.uID = 0;
    data.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_ICO_SMALL));
    data.uCallbackMessage = WM_SYSTRAY_EVENT;
    strcpy(data.szTip, MAIN_TITLE);
    strcpy(data.szInfo, "SlickCursor is running");
    data.uTimeout = 0;
    data.dwInfoFlags = NIIF_NONE;
    data.uFlags = NIF_TIP | NIF_MESSAGE | NIF_ICON | NIF_INFO;

    Shell_NotifyIcon(NIM_ADD, &data);
}

static void LeaveSysTray(HWND hwnd)
{
    NOTIFYICONDATA data = {0};

    data.cbSize = sizeof(NOTIFYICONDATA);
    data.hWnd = hwnd;
    data.uID = 0;

    Shell_NotifyIcon(NIM_DELETE, &data);
}
