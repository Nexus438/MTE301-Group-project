#include <windows.h>
#include <vector>

constexpr int ROWS = 10, COLS = 14, CELL = 48;
constexpr int LEFT = 20, TOP = 72;
constexpr COLORREF BACKGROUND = RGB(39, 27, 27);
constexpr COLORREF LABEL_COLOR = RGB(237, 237, 237);

// Room coordinates start at zero. Each rectangle is column, row, width, height.
struct CellRect {
    int col, row, width, height;
    const char* title = nullptr;
};

struct Scenario {
    const char* name;
    int homeCol, homeRow;
    std::vector<CellRect> walls, cutouts, furniture;
};

const Scenario scenarios[] = {
    {"Open studio", 0, 9, {{10, 0, 1, 2}, {10, 3, 4, 1}}, {}, {
        {0, 0, 5, 1, "Kitchen"}, {0, 1, 1, 1, "Fridge"},
        {3, 2, 2, 1, "Island"}, {6, 1, 2, 2, "Dining"},
        {11, 0, 1, 1, "Sink"}, {12, 0, 1, 2, "Shower"},
        {13, 0, 1, 1, "Toilet"}, {11, 5, 3, 2, "Bed"},
        {13, 8, 1, 2, "Wardrobe"}, {1, 6, 4, 1, "Sofa"},
        {2, 8, 2, 1, "Coffee table"}, {0, 7, 1, 2, "TV"}
    }},
    {"One-bedroom apartment", 0, 9,
        {{8, 0, 1, 3}, {8, 4, 1, 3}, {8, 8, 1, 2}, {9, 5, 2, 1}, {12, 5, 2, 1}}, {}, {
        {0, 0, 6, 1, "Kitchen"}, {0, 1, 1, 1, "Fridge"},
        {4, 2, 2, 2, "Dining"}, {1, 6, 4, 1, "Sofa"},
        {2, 8, 2, 1, "Coffee table"}, {6, 6, 1, 2, "TV"},
        {10, 0, 3, 2, "Bed"}, {13, 0, 1, 4, "Wardrobe"},
        {9, 4, 2, 1, "Desk"}, {12, 6, 2, 2, "Shower"},
        {9, 9, 1, 1, "Toilet"}, {10, 9, 2, 1, "Sink"}, {9, 6, 1, 1, "Laundry"}
    }},
    {"Two-bedroom home", 0, 9,
        {{6, 0, 1, 4}, {0, 4, 3, 1}, {4, 4, 6, 1}, {11, 4, 3, 1},
         {10, 6, 4, 1}, {10, 7, 1, 1}, {10, 9, 1, 1}}, {}, {
        {1, 0, 3, 2, "Bed A"}, {5, 0, 1, 3, "Wardrobe"},
        {8, 0, 3, 2, "Bed B"}, {12, 0, 2, 1, "Desk"},
        {13, 2, 1, 2, "Wardrobe"}, {11, 5, 3, 1, "Kitchen"},
        {9, 6, 1, 1, "Fridge"}, {1, 6, 4, 1, "Sofa"},
        {2, 8, 2, 1, "Coffee table"}, {6, 7, 2, 2, "Dining"},
        {12, 7, 2, 1, "Shower"}, {13, 9, 1, 1, "Toilet"}, {11, 9, 1, 1, "Sink"}
    }},
    {"Central hallway home", 6, 9,
        {{5, 0, 1, 2}, {5, 3, 1, 4}, {5, 8, 1, 2}, {0, 4, 5, 1},
         {8, 0, 1, 3}, {8, 4, 1, 3}, {8, 8, 1, 2}, {9, 5, 5, 1}}, {}, {
        {0, 0, 4, 1, "Sofa"}, {1, 2, 2, 1, "Coffee table"},
        {4, 0, 1, 2, "TV"}, {1, 6, 3, 2, "Bed"},
        {0, 9, 3, 1, "Wardrobe"}, {9, 0, 5, 1, "Kitchen"},
        {10, 2, 2, 1, "Island"}, {10, 4, 2, 1, "Dining"},
        {12, 6, 2, 2, "Shower"}, {9, 6, 2, 1, "Sink"},
        {13, 9, 1, 1, "Toilet"}, {9, 9, 1, 1, "Laundry"}
    }},
    {"L-shaped loft", 6, 9, {{0, 5, 5, 1}, {4, 6, 1, 2}, {4, 9, 1, 1}}, {{11, 0, 3, 3}}, {
        {0, 0, 6, 1, "Kitchen"}, {0, 1, 1, 1, "Fridge"},
        {3, 2, 2, 1, "Island"}, {7, 1, 2, 2, "Dining"},
        {0, 3, 3, 2, "Bed"}, {7, 5, 4, 1, "Sofa"},
        {13, 4, 1, 3, "TV"}, {8, 7, 2, 1, "Coffee table"},
        {6, 8, 2, 1, "Desk"}, {12, 9, 1, 1, "Plant"},
        {0, 6, 2, 1, "Shower"}, {0, 9, 1, 1, "Toilet"}, {3, 6, 1, 1, "Sink"}
    }}
};
constexpr int SCENARIO_COUNT = sizeof(scenarios) / sizeof(scenarios[0]);

HWND layoutMenu = nullptr;

// Convert a rectangle in grid cells to a rectangle in screen pixels.
RECT CellBox(const CellRect& area) {
    return {LEFT + area.col * CELL, TOP + area.row * CELL,
            LEFT + (area.col + area.width) * CELL, TOP + (area.row + area.height) * CELL};
}

void FillColor(HDC dc, RECT box, COLORREF color) {
    SetDCBrushColor(dc, color);
    FillRect(dc, &box, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
}

void DrawRoom(HDC dc, const Scenario& scenario) {
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            RECT cell = CellBox({col, row, 1, 1});
            --cell.right;
            --cell.bottom;
            FillColor(dc, cell, RGB(59, 48, 48));
        }
    }
    for (const auto& wall : scenario.walls) {
        FillColor(dc, CellBox(wall), RGB(168, 164, 164));
    }
    for (const auto& cutout : scenario.cutouts) {
        FillColor(dc, CellBox(cutout), BACKGROUND);
    }

    SelectObject(dc, GetStockObject(DC_BRUSH));
    SelectObject(dc, GetStockObject(DC_PEN));
    SetDCPenColor(dc, LABEL_COLOR);
    const UINT center = DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
    for (const auto& item : scenario.furniture) {
        RECT box = CellBox(item);
        SetDCBrushColor(dc, RGB(64, 53, 53));
        RoundRect(dc, box.left, box.top, box.right, box.bottom, 6, 6);
        DrawTextA(dc, item.title, -1, &box, center);
    }

    RECT dock = CellBox({scenario.homeCol, scenario.homeRow, 1, 1});
    InflateRect(&dock, -9, -9);
    SetDCPenColor(dc, BACKGROUND);
    SetDCBrushColor(dc, RGB(249, 179, 140));
    Ellipse(dc, dock.left, dock.top, dock.right, dock.bottom);
    SetTextColor(dc, BACKGROUND);
    DrawTextA(dc, "R", -1, &dock, center);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
    switch (message) {
    case WM_COMMAND:
        if (reinterpret_cast<HWND>(lp) == layoutMenu && HIWORD(wp) == CBN_SELCHANGE) {
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(hwnd, &paint);
        RECT client{};
        GetClientRect(hwnd, &client);
        FillColor(dc, client, BACKGROUND);
        SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT));
        SetTextColor(dc, LABEL_COLOR);
        SetBkMode(dc, TRANSPARENT);

        RECT caption{LEFT, 9, LEFT + 280, 30};
        DrawTextA(dc, "Choose a home layout", -1, &caption, DT_LEFT | DT_SINGLELINE);
        const LRESULT choice = layoutMenu ? SendMessageA(layoutMenu, CB_GETCURSEL, 0, 0) : 0;
        if (choice >= 1 && choice <= SCENARIO_COUNT) {
            DrawRoom(dc, scenarios[choice - 1]);
        }
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, message, wp, lp);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
    WNDCLASSA windowClass{};
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "RoombaMenuWindow";
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    if (!RegisterClassA(&windowClass)) return 1;

    const DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
    RECT size{0, 0, 2 * LEFT + COLS * CELL, TOP + ROWS * CELL + 20};
    AdjustWindowRect(&size, style, FALSE);
    HWND hwnd = CreateWindowA(windowClass.lpszClassName, "Roomba - Scenario Preview", style,
        CW_USEDEFAULT, CW_USEDEFAULT, size.right - size.left, size.bottom - size.top,
        nullptr, nullptr, instance, nullptr);
    if (!hwnd) return 1;

    // The first menu item is a prompt. The five layouts follow it.
    layoutMenu = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        LEFT, 31, 280, 180, hwnd, nullptr, instance, nullptr);
    if (!layoutMenu) return 1;
    SendMessageA(layoutMenu, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    SendMessageA(layoutMenu, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Choose a scenario..."));
    for (const auto& scenario : scenarios) {
        SendMessageA(layoutMenu, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(scenario.name));
    }
    SendMessageA(layoutMenu, CB_SETCURSEL, 0, 0);
    ShowWindow(hwnd, show);
    SetFocus(layoutMenu);

    MSG message{};
    while (GetMessageA(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return 0;
}
