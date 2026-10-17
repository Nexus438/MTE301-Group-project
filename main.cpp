#include <windows.h>

const int ROWS = 10, COLS = 30, CELL = 30;
const int FREE = 0, WALL = 1;
int grid[ROWS][COLS];  // globals start as all zeros (FREE)

// Windows calls this every time something happens to the window.
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT whole = {0, 0, COLS * CELL, ROWS * CELL};
        SetDCBrushColor(hdc, RGB(0, 0, 0));   // outline color
        FillRect(hdc, &whole, (HBRUSH)GetStockObject(DC_BRUSH));
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                COLORREF color = (grid[r][c] == WALL) ? RGB(70, 70, 70) : RGB(245, 245, 245);
                SetDCBrushColor(hdc, color);
                RECT cell = {c * CELL, r * CELL, (c + 1) * CELL - 1, (r + 1) * CELL - 1};
                FillRect(hdc, &cell, (HBRUSH)GetStockObject(DC_BRUSH));
            }
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:  // the user closed the window
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);  // default behaviour for everything else
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int show) {
    // put walls around the border
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            if (r == 0 || c == 0 || r == ROWS - 1 || c == COLS - 1) grid[r][c] = WALL;

    // describe the window
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.lpszClassName = "RoomWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&wc);

    // work out the outer window size so the drawing area is exactly COLS*CELL x ROWS*CELL
    RECT rc = {0, 0, COLS * CELL, ROWS * CELL};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowA("RoomWindow", "Window", WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
                              nullptr, nullptr, inst, nullptr);
    ShowWindow(hwnd, show);

    // message loop: runs until the window closes
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}