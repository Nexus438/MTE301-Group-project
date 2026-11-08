#include <windows.h>
#include <algorithm>
#include <array>
#include <string>
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

HWND layoutMenu = nullptr, cleanButton = nullptr;
constexpr UINT_PTR ROBOT_TIMER = 1;
std::array<bool, ROWS * COLS> blocked{}, cleaned{};
std::vector<int> path;
std::size_t nextStep = 0;
int home = -1, robot = -1;
int floorCount = 0, cleanedCount = 0, moves = 0;
bool running = false, finished = false;

struct SearchResult {
    std::array<int, ROWS * COLS> parent;
    std::vector<int> order;
};

// Process cells in the order discovered. That makes this vector a BFS queue.
SearchResult BFS(int start) {
    SearchResult search;
    search.parent.fill(-1);
    search.parent[start] = start;
    search.order.push_back(start);
    const int rowChange[] = {-1, 0, 1, 0};
    const int colChange[] = {0, 1, 0, -1};
    for (std::size_t index = 0; index < search.order.size(); ++index) {
        const int current = search.order[index];
        for (int direction = 0; direction < 4; ++direction) {
            const int row = current / COLS + rowChange[direction];
            const int col = current % COLS + colChange[direction];
            if (row < 0 || row >= ROWS || col < 0 || col >= COLS) continue;
            const int next = row * COLS + col;
            if (!blocked[next] && search.parent[next] == -1) {
                search.parent[next] = current;
                search.order.push_back(next);
            }
        }
    }
    return search;
}

void BlockAreas(const std::vector<CellRect>& areas) {
    for (const auto& area : areas) {
        for (int row = area.row; row < area.row + area.height; ++row) {
            for (int col = area.col; col < area.col + area.width; ++col) {
                blocked[row * COLS + col] = true;
            }
        }
    }
}

// Changing the selection also resets the robot and the cleaning counters.
void SelectScenario(HWND hwnd) {
    KillTimer(hwnd, ROBOT_TIMER);
    running = finished = false;
    home = robot = -1;
    floorCount = cleanedCount = moves = 0;
    blocked.fill(false);
    cleaned.fill(false);
    path.clear();
    nextStep = 0;
    const LRESULT choice = SendMessageA(layoutMenu, CB_GETCURSEL, 0, 0);
    if (choice >= 1 && choice <= SCENARIO_COUNT) {
        const auto& scenario = scenarios[choice - 1];
        BlockAreas(scenario.walls);
        BlockAreas(scenario.cutouts);
        BlockAreas(scenario.furniture);
        home = scenario.homeRow * COLS + scenario.homeCol;
        robot = home;
        floorCount = static_cast<int>(BFS(home).order.size());
        cleaned[home] = true;
        cleanedCount = 1;
    }
    SetWindowTextA(cleanButton, "Start cleaning");
    EnableWindow(cleanButton, home >= 0);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void PlanNextPath() {
    const SearchResult search = BFS(robot);
    int target = home;
    for (int cell : search.order) {
        if (!cleaned[cell]) {
            target = cell;
            break;
        }
    }
    // With no dirty squares left, the target stays at home.
    path.clear();
    for (int cell = target; cell != robot; cell = search.parent[cell]) {
        path.push_back(cell);
    }
    std::reverse(path.begin(), path.end());
    nextStep = 0;
}

void MoveRobot(HWND hwnd) {
    if (nextStep == path.size()) PlanNextPath();
    if (path.empty()) {
        KillTimer(hwnd, ROBOT_TIMER);
        running = false;
        finished = true;
        SetWindowTextA(cleanButton, "Replay cleaning");
    } else {
        robot = path[nextStep++];
        ++moves;
        if (!cleaned[robot]) {
            cleaned[robot] = true;
            ++cleanedCount;
        }
    }
    InvalidateRect(hwnd, nullptr, FALSE);
}

void ToggleCleaning(HWND hwnd) {
    if (home < 0) return;
    if (running) {
        KillTimer(hwnd, ROBOT_TIMER);
        running = false;
        SetWindowTextA(cleanButton, "Resume cleaning");
    } else {
        if (finished) SelectScenario(hwnd);
        if (!SetTimer(hwnd, ROBOT_TIMER, 120, nullptr)) {
            MessageBoxA(hwnd, "Could not start the timer.", "Roomba", MB_OK | MB_ICONERROR);
            return;
        }
        running = true;
        SetWindowTextA(cleanButton, "Pause cleaning");
    }
}

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
            const COLORREF color = cleaned[row * COLS + col] ? RGB(98, 70, 59) : RGB(59, 48, 48);
            FillColor(dc, cell, color);
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
    DrawTextA(dc, "H", -1, &dock, center);
    RECT body = CellBox({robot % COLS, robot / COLS, 1, 1});
    InflateRect(&body, -9, -9);
    SetDCPenColor(dc, BACKGROUND);
    SetDCBrushColor(dc, RGB(249, 179, 140));
    Ellipse(dc, body.left, body.top, body.right, body.bottom);
    SetTextColor(dc, BACKGROUND);
    DrawTextA(dc, "R", -1, &body, center);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
    switch (message) {
    case WM_COMMAND:
        if (reinterpret_cast<HWND>(lp) == layoutMenu && HIWORD(wp) == CBN_SELCHANGE) {
            SelectScenario(hwnd);
        }
        if (reinterpret_cast<HWND>(lp) == cleanButton && HIWORD(wp) == BN_CLICKED) {
            ToggleCleaning(hwnd);
        }
        return 0;
    case WM_TIMER:
        if (wp == ROBOT_TIMER && running) MoveRobot(hwnd);
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
            const std::string status = "Cleaned " + std::to_string(cleanedCount) + "/" +
                std::to_string(floorCount) + " | Moves " + std::to_string(moves) +
                (finished ? " | Finished at home" : "");
            SetTextColor(dc, LABEL_COLOR);
            RECT statusBox{LEFT + 300, 9, LEFT + COLS * CELL, 30};
            DrawTextA(dc, status.c_str(), -1, &statusBox, DT_LEFT | DT_SINGLELINE);
            RECT legend{LEFT, TOP + ROWS * CELL + 5, LEFT + COLS * CELL, TOP + ROWS * CELL + 27};
            DrawTextA(dc, "Warm brown = clean   R = robot   H = home", -1, &legend, DT_LEFT | DT_SINGLELINE);
        }
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd, ROBOT_TIMER);
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
    RECT size{0, 0, 2 * LEFT + COLS * CELL, TOP + ROWS * CELL + 32};
    AdjustWindowRect(&size, style, FALSE);
    HWND hwnd = CreateWindowA(windowClass.lpszClassName, "Roomba - BFS Cleaning", style,
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
    cleanButton = CreateWindowA("BUTTON", "Start cleaning", WS_CHILD | WS_VISIBLE,
        LEFT + 300, 31, 160, 27, hwnd, nullptr, instance, nullptr);
    if (!cleanButton) return 1;
    SendMessageA(cleanButton, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    EnableWindow(cleanButton, FALSE);
    ShowWindow(hwnd, show);
    SetFocus(layoutMenu);

    MSG message{};
    while (GetMessageA(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return 0;
}
