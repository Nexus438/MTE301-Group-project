#include <windows.h>

const int ROWS = 10, COLS = 14, CELL = 64;
const int DIRTY = 0, WALL = 1, CLEAN = 2;
int grid[ROWS][COLS];  // globals start as all zeros (DIRTY)

// Windows calls this every time something happens to the window.
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {

    // Painting the window
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT whole = {0, 0, COLS * CELL, ROWS * CELL};
        SetDCBrushColor(hdc, RGB(138, 133, 133));   // outline color
        FillRect(hdc, &whole, (HBRUSH)GetStockObject(DC_BRUSH));

        grid[0][1] = WALL;  // mark the top left cell as wall
        grid[1][1] = WALL;  // mark the cell below it as wall

        // paint each cell depending on whether it's a wall or DIRTY space
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                COLORREF color;
                if (grid[r][c] == WALL) {
                    color = RGB(168, 164, 164);
                } else if (grid[r][c] == DIRTY) {
                    color = RGB(59, 48, 48);
                } else if (grid[r][c] == CLEAN) {
                    color = RGB(255, 255, 255);
                }
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