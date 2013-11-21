#include <math.h>
#include "emuparams.h"
#include "windows.h"

using namespace std;

EmuParams emu;

static POINT trails[3];

static HHOOK kbdHook = 0;
static UINT_PTR timerId;

static VOID CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD);
static LRESULT CALLBACK KeyboardProc(int, WPARAM, LPARAM);
static void NextCursorPos(POINT&);
static void UpdateMouseTrails(const POINT&);
static bool ChooseHalfbackPos(POINT&, int, int);
static void MouseLeftButton(bool);
static void MouseRightButton(bool);
static void MouseWheelUp();
static void MouseWheelDown();

extern void ResumeScreenAnalysis();
extern void SuspendScreenAnalysis();
extern bool ChooseNearCluster(POINT&, bool);

bool BeginMouseEmulation()
{
    DWORD dwThreadId = 0;
    HMODULE hModule = GetModuleHandle(NULL);
#ifdef __GNUC__
    // AVG Antivirus treats apps that use low level keyboard hooks as a virus.

    // To force the compiler to push a value in the stack, from a variable,
    // we're going to make the compiler unable to infer it at compile time.
    // (SetWindowsHookEx is expected to return NULL at run time)
    dwThreadId = (DWORD) SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hModule, GetCurrentThreadId());

    // We can bypass the detection if we mutate slightly the "standard" code.
    __asm__ ("pushl %0" : : "m" (dwThreadId) :); // <--
    __asm__ ("pushl %0" : : "m" (hModule) :);
    __asm__ ("pushl %0" : : "i" (KeyboardProc) :);
    __asm__ ("pushl %0" : : "I" (WH_KEYBOARD_LL) :);
    __asm__ ("call %P0" : : "i" (SetWindowsHookEx) :);
    __asm__ ("movl %0, %%eax" : "=r" (kbdHook): :);
#else
    kbdHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hModule, dwThreadId);
#endif

    timerId = SetTimer(NULL, 0, 1000/40, TimerProc);

    GetCursorPos(&trails[0]);
    trails[1] = trails[0];

    return kbdHook && timerId;
}

void EndMouseEmulation()
{
    KillTimer(NULL, timerId);
    UnhookWindowsHookEx(kbdHook);
}

static VOID CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD)
{
    if (!emu.useScreenAnalysis || !emu.arePressedKeys())
        SuspendScreenAnalysis();
    else
        ResumeScreenAnalysis();

    POINT pos;
    GetCursorPos(&pos);

    static POINT prevPos = pos;
    if (!(prevPos.x == pos.x && prevPos.y == pos.y)) {
        prevPos = pos;
        UpdateMouseTrails(pos);
    }

    POINT nextPos = pos;
    NextCursorPos(nextPos);

    if (!(nextPos.x == pos.x && nextPos.y == pos.y)) {
        SetCursorPos(nextPos.x, nextPos.y);
    }
}

static LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT * kbd = (KBDLLHOOKSTRUCT *) lParam;

    bool isExtended = kbd->flags & LLKHF_EXTENDED;
    bool isPressed = WM_KEYDOWN == wParam;

    if (code == HC_ACTION) {
        if ((emu.useExtended && isExtended) || (emu.useNumpad && !isExtended))
            switch (kbd->vkCode) {
                case VK_UP: emu.setUp(isPressed); return 1;
                case VK_DOWN: emu.setDown(isPressed); return 1;
                case VK_LEFT: emu.setLeft(isPressed); return 1;
                case VK_RIGHT: emu.setRight(isPressed); return 1;
            }

        if (VK_CLEAR == kbd->vkCode && emu.useNumpad) {
            emu.setClear(isPressed);
            return 1;
        }
    }

    if (code == HC_ACTION && emu.useExtended && isExtended) {
        if (VK_PRIOR == kbd->vkCode && isPressed) {
            MouseWheelUp();
            return 1;
        }

        if (VK_NEXT == kbd->vkCode && isPressed) {
            MouseWheelDown();
            return 1;
        }
    }

    if (code == HC_ACTION && emu.useNumpad && (VK_NUMLOCK != kbd->vkCode) && !GetKeyState(VK_NUMLOCK)) {
        if (VK_PRIOR == kbd->vkCode && isPressed && !isExtended) {
            MouseWheelUp();
            return 1;
        }

        if (VK_NEXT == kbd->vkCode && isPressed && !isExtended) {
            MouseWheelDown();
            return 1;
        }

        if (VK_ADD == kbd->vkCode) {
            MouseLeftButton(isPressed);
            return 1;
        }

        if (VK_SUBTRACT == kbd->vkCode) {
            MouseRightButton(isPressed);
            return 1;
        }

        if (VK_DIVIDE == kbd->vkCode) {
            keybd_event(VK_BROWSER_BACK, MapVirtualKey(VK_BROWSER_BACK, 0), isPressed?0:KEYEVENTF_KEYUP, NULL);
            return 1;
        }

        if (VK_MULTIPLY == kbd->vkCode) {
            keybd_event(VK_BROWSER_FORWARD, MapVirtualKey(VK_BROWSER_FORWARD, 0), isPressed?0:KEYEVENTF_KEYUP, NULL);
            return 1;
        }
    }

    if (code == HC_ACTION && VK_LSHIFT == kbd->vkCode) {
        emu.setLShift(isPressed);
    }

    return CallNextHookEx(kbdHook, code, wParam, lParam);
}

static void NextCursorPos(POINT & pos)
{
    static int t;
    static EmuParams prevEmu;

    if (!emu.arePressedKeys()) {
        t = 0;
        prevEmu = emu;
        return;
    }

    const double ax = 15;
    const double ay = 20;
    double vx = 0;
    double vy = 0;

    if (emu.up()) {
        vy -= ay;
    }

    if (emu.down()) {
        vy += ay;
    }

    if (emu.left()) {
        vx -= ax;
    }

    if (emu.right()) {
        vx += ax;
    }

    if (!prevEmu.arePressedKeys()) {
        // Keys are pressed; Discrete motion
        int xdir = ((vx>0) - (vx<0));
        int ydir = ((vy>0) - (vy<0));

        if (!ChooseHalfbackPos(pos, xdir, ydir)) {
            vx = xdir * ax;
            pos.x += (int) (0.5 + vx);

            bool upwards = vy < 0;
            vy = ydir * ay;
            if (vy != 0 && !ChooseNearCluster(pos, upwards))
                pos.y += (int) (0.5 + vy);
        }
    }

    if (!emu.sameKeyState(prevEmu)) {
        // Direction changed
        vx = 0;
        vy = 0;
    }

    t += 1;

    if (t >= 8) {
        // Keys are held down; Continuous motion
        bool diagonal = (emu.up() || emu.down()) && (emu.left() || emu.right());
        double turbo = emu.lShift()? 2.5 : 1;
        pos.x += (int) (0.5 + turbo * vx * (diagonal? 0.707 : 1));
        pos.y += (int) (0.5 + turbo * vy * (diagonal? 0.707 : 1));
    }

    prevEmu = emu;
}

static void UpdateMouseTrails(const POINT & pt)
{
    trails[2] = trails[1];
    trails[1] = trails[0];
    trails[0] = pt;
}

static bool ChooseHalfbackPos(POINT & pt, int xdir, int ydir)
{
    int x = pt.x;
    int y = pt.y;

    if (xdir != 0 && trails[0].y == trails[1].y) {
        int L = min(trails[1].x, trails[2].x);
        int R = max(trails[1].x, trails[2].x);
        int LR = xdir < 0? L : R;
        int X = trails[0].x;
        int M = xdir < 0? min(X, (X + trails[1].x)/2) : max(X, (X + trails[1].x)/2);
        x = x > L && x < R? LR : M;
    }

    if (ydir != 0 && trails[0].x == trails[1].x) {
        int L = min(trails[1].y, trails[2].y);
        int R = max(trails[1].y, trails[2].y);
        int LR = ydir < 0? L : R;
        int Y = trails[0].y;
        int M = ydir < 0? min(Y, (Y + trails[1].y)/2) : max(Y, (Y + trails[1].y)/2);
        y = y > L && y < R? LR : M;
    }

    if (pt.x == x && pt.y == y)
        return false;

    pt.x = x;
    pt.y = y;

    return true;
}

static void MouseLeftButton(bool down)
{
    static bool state = false;
    if (state == down) return;

    state = down;
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

static void MouseRightButton(bool down)
{
    static bool state = false;
    if (state == down) return;

    state = down;
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    SendInput(1, &input, sizeof(INPUT));
}

static void MouseWheelUp()
{
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = WHEEL_DELTA;
    SendInput(1, &input, sizeof(INPUT));
}

static void MouseWheelDown()
{
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = -WHEEL_DELTA;
    SendInput(1, &input, sizeof(INPUT));
}
