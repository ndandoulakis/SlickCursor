#include <math.h>
#include "bmp24.h"
#include "common.h"
#include "screengraph.h"

using namespace std;

struct ScreenData {
    bool recent;

    POINT pt;
    Bmp24 bmp;
    vector<ScreenGraph::Cluster> clusters;
};

// Data flow from Screen thread to Main thread through p2.
// To reduce mem allocs, ScreenData instances are reserved.
static ScreenData * p1 = new ScreenData; // Main
static ScreenData * p2 = new ScreenData; // (shared)
static ScreenData * p3 = new ScreenData; // Screen

static HWND hObserver;

static DWORD WINAPI ThreadProc(LPVOID);
static void PullRecentData();
static void PushRecentData();

static HANDLE hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
static HANDLE hThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL);

void SetScreenObserver(HWND hwnd)
{
    hObserver = hwnd;
}

void DrawAnalysisData(HDC hdc)
{
    PullRecentData();

    p1->bmp.copyToDc(hdc, 0, 0);

    SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    SelectObject(hdc, GetStockObject(DC_PEN));
    SetDCPenColor(hdc, RGB(180, 180, 0));

    for (int i = 0; i < (int) p1->clusters.size(); i++) {
        const ScreenGraph::Cluster & c = p1->clusters[i];
        Rectangle(hdc, c.bb.left, c.bb.top, c.bb.right+1, c.bb.bottom+1);
    }

    // draw cursor's spot
    SetDCPenColor(hdc, RGB(255, 255, 255));
    Rectangle(hdc, 126, 127, 131, 130);
    Rectangle(hdc, 127, 126, 130, 131);

    SetDCPenColor(hdc, RGB(200, 0, 0));
    Rectangle(hdc, 127, 128, 130, 129);
    Rectangle(hdc, 128, 127, 129, 130);
}

bool ChooseNearCluster(POINT & pt, bool upwards)
{
    PullRecentData();

    // Detection box
    RECT db = {pt.x, pt.y, pt.x, pt.y};
    InflateRect(&db, 5, 13);
    if (upwards)
        OffsetRect(&db, 0, -13 -1);
    else
        OffsetRect(&db, 0, +13 +1);


    int dmin = 1000;
    int my = 0;

    for (int i = 0; i < (int) p1->clusters.size(); i++) {
        RECT c = p1->clusters[i].bb;

        // Exclude horizontal lines, dots, etc
        if (c.bottom - c.top < 2)
            continue;

        // Map cluster to screen coordinates
        OffsetRect(&c, p1->pt.x - 128, p1->pt.y - 128);

        RECT dst;
        if (!IntersectRect(&dst, &db, &c))
            continue;

        const int cy = (c.top + c.bottom)/2;
        const int dy = pow(cy - pt.y, 2);

        if (upwards && cy > pt.y) continue;
        if (!upwards && cy < pt.y) continue;

        if (abs(pt.y - cy) <= 2)
            continue;

        if (dy < dmin) {
            dmin = dy;
            my = cy;
        }
    }

    if (dmin == 1000)
        return false;

    pt.y = my;

    return true;
}

void ResumeScreenAnalysis()
{
    SetEvent(hEvent);
}

void SuspendScreenAnalysis()
{
    ResetEvent(hEvent);
}

static DWORD WINAPI ThreadProc(LPVOID)
{
    ScreenGraph graph;

    while (WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0) {
        HDC screen = GetDC(NULL);

        GetCursorPos(& p3->pt);
        p3->bmp.copyDcAt(screen, p3->pt.x-128, p3->pt.y-128, 256, 256);

        DWORD hash = graph.bmpHash;
        graph.analyze(p3->bmp);

        ReleaseDC(NULL, screen);

        if (hash != graph.bmpHash) {
            p3->clusters.assign(graph.clusters.begin(), graph.clusters.end());
            PushRecentData();

            if (hObserver)
                PostMessage(hObserver, WM_SCREEN_OBSERVER, 0, 0);
        }

        Sleep(1000/10);
    }

    return 0;
}

static void PullRecentData()
{
    p1 = (ScreenData*) InterlockedExchangePointer(&p2, p1);
    if (!p1->recent) {
        // we had recent data, get them back
        p1 = (ScreenData*) InterlockedExchangePointer(&p2, p1);
        // at this point we might got new data
    }
    p1->recent = false;
}

static void PushRecentData()
{
    p3->recent = true;
    p3 = (ScreenData*) InterlockedExchangePointer(&p2, p3);
}
