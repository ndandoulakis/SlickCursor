#include "emuparams.h"
#include "windows.h"

EmuParams emu;

static POINT trails[3];

static HHOOK kbdHook;
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
    kbdHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    timerId = SetTimer(NULL, 0, 1000/40, TimerProc);
    return kbdHook && timerId;
}

void EndMouseEmulation()
{
    KillTimer(NULL, timerId);
    UnhookWindowsHookEx(kbdHook);
}

static VOID CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD)
{
    if (!emu.arePressedKeys())
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
        if ((emu.useArrows && isExtended) || (emu.useNumpad && !isExtended))
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

    if (code == HC_ACTION && emu.useNumpad && (VK_NUMLOCK != kbd->vkCode) && !GetKeyState(VK_NUMLOCK)) {
        // TODO option for extended PgUp/PgDn, convenient for laptop users
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

        if (VK_DIVIDE == kbd->vkCode) {
            MouseRightButton(isPressed);
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
    if (xdir != 0 && ydir != 0)
        return false;

    if (xdir < 0) {
        if (trails[0].x > trails[1].x && trails[0].y == trails[1].y) {
            pt.x = (trails[0].x + trails[1].x)/2;
            return true;
        }
    }

    if (xdir > 0) {
        if (trails[0].x < trails[1].x && trails[0].y == trails[1].y) {
            pt.x = (trails[0].x + trails[1].x)/2;
            return true;
        }
    }

    if (ydir < 0) {
        if (trails[0].y > trails[1].y && trails[0].x == trails[1].x) {
            pt.y = (trails[0].y + trails[1].y)/2;
            return true;
        }
    }

    if (ydir > 0) {
        if (trails[0].y < trails[1].y && trails[0].x == trails[1].x) {
            pt.y = (trails[0].y + trails[1].y)/2;
            return true;
        }
    }

    return false;
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
