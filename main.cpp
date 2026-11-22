// roomba simulator with 5 home layouts
// BFS finds short paths and 2-opt helps shorten the full route
// the map is already known and each move goes to a neighbouring square
#include <windows.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

// grid size and size of each square in pixels
constexpr int ROWS = 10, COLS = 14, CELL = 48;
// leave some space for the menu and grid numbers
constexpr int LEFT = 42, TOP = 94;
// background and text colours
constexpr COLORREF BACKGROUND = RGB(39, 27, 27);
constexpr COLORREF LABEL_COLOR = RGB(237, 237, 237);

// rectangles use column, row, width and height, starting from 0
struct CellRect {
    int col, row, width, height;
    const char* title = nullptr; // name to show on the furniture
};

// stores one layout and the robot's home position
struct Scenario {
    const char* name;
    int homeCol, homeRow;
    std::vector<CellRect> walls, cutouts, furniture;
};

// the 5 layouts, with walls, furniture and any space outside the room
// home is written as column then row, starting from 0
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
// work out how many layouts there are
constexpr int SCENARIO_COUNT = sizeof(scenarios) / sizeof(scenarios[0]);

// handles for the layout menu and the cleaning button
HWND layoutMenu = nullptr, cleanButton = nullptr;
// ID used for the robot's timer
constexpr UINT_PTR ROBOT_TIMER = 1;
// use row * COLS + column to give each square a number
// track which squares are blocked and which ones are clean
std::array<bool, ROWS * COLS> blocked{}, cleaned{};
// actual moves the robot follows, ending back at home
std::vector<int> path;
// next move in the path and how much of the trail to draw
std::size_t nextStep = 0;
// -1 means no layout has been picked yet
int home = -1, robot = -1;
// count floor squares, cleaned squares and moves, including repeated moves
int floorCount = 0, cleanedCount = 0, moves = 0;
// keep the first route length so we can compare it later
int baselineMoves = 0;
// check if the robot is moving or has finished
bool running = false, finished = false;

// store what BFS found so we can build the paths
struct SearchResult {
    std::array<int, ROWS * COLS> parent;   // square before this one on the shortest path
    std::array<int, ROWS * COLS> distance; // moves needed to reach the square
    std::vector<int> order;               // squares in the order BFS found them
};

// check the closest squares first using BFS
SearchResult BFS(int start) {
    SearchResult search;
    // -1 means this search has not reached the square yet
    search.parent.fill(-1);
    search.distance.fill(-1);
    search.parent[start] = start;
    search.distance[start] = 0;
    search.order.push_back(start);
    // check up, right, down and left
    const int rowChange[] = {-1, 0, 1, 0};
    const int colChange[] = {0, 1, 0, -1};
    for (std::size_t index = 0; index < search.order.size(); ++index) {
        const int current = search.order[index];
        for (int direction = 0; direction < 4; ++direction) {
            const int row = current / COLS + rowChange[direction];
            const int col = current % COLS + colChange[direction];
            // make sure the next square is inside the grid
            if (row < 0 || row >= ROWS || col < 0 || col >= COLS) continue;
            const int next = row * COLS + col;
            if (!blocked[next] && search.parent[next] == -1) {
                // save the path and add the square to the search
                search.parent[next] = current;
                search.distance[next] = search.distance[current] + 1;
                search.order.push_back(next);
            }
        }
    }
    return search;
}

// mark walls, furniture and outside areas as blocked
void BlockAreas(const std::vector<CellRect>& areas) {
    for (const auto& area : areas) {
        for (int row = area.row; row < area.row + area.height; ++row) {
            for (int col = area.col; col < area.col + area.width; ++col) {
                blocked[row * COLS + col] = true;
            }
        }
    }
}

// add the steps between two stops to the full path
void AppendShortestPath(std::vector<int>& steps, int from, int to, const SearchResult& search) {
    std::vector<int> segment;
    // follow the path backwards from the end
    for (int cell = to; cell != from; cell = search.parent[cell]) {
        segment.push_back(cell);
    }
    std::reverse(segment.begin(), segment.end()); // flip the steps so they go in the right order
    steps.insert(steps.end(), segment.begin(), segment.end());
}

// add up the moves for the full route, including going home
int RouteLength(const std::vector<int>& order, const std::vector<SearchResult>& searches) {
    int length = 0;
    for (std::size_t index = 1; index < order.size(); ++index) {
        length += searches[order[index - 1]].distance[order[index]];
    }
    return length;
}

// make a starting route by picking the nearest dirty square
std::vector<int> GreedyOrder(const std::vector<SearchResult>& searches) {
    std::array<bool, ROWS * COLS> visited{};
    std::vector<int> order{home};
    visited[home] = true;
    // pick the nearest square we have not visited yet
    while (order.size() < searches[home].order.size()) {
        for (int cell : searches[order.back()].order) {
            if (!visited[cell]) {
                order.push_back(cell);
                visited[cell] = true;
                break;
            }
        }
    }
    order.push_back(home); // finish the route at home
    return order;
}

// shuffle the choices to try different routes when distances are equal
std::vector<int> RandomNearestOrder(std::vector<int> remaining,
                                   const std::vector<SearchResult>& searches,
                                   std::mt19937& random) {
    std::shuffle(remaining.begin(), remaining.end(), random);
    std::vector<int> order{home};
    while (!remaining.empty()) {
        const int current = order.back();
        // find the nearest square using the shuffled order to break ties
        const auto nearest = std::min_element(remaining.begin(), remaining.end(),
            [&searches, current](int first, int second) {
                return searches[current].distance[first] < searches[current].distance[second];
            });
        order.push_back(*nearest);
        // remove this square since it is already in the route
        remaining.erase(nearest);
    }
    order.push_back(home);
    return order;
}

// add home to the start and end of the route
std::vector<int> ClosedOrder(std::vector<int> cells) {
    cells.insert(cells.begin(), home);
    cells.push_back(home);
    return cells;
}

// flip a section if it saves moves and keep home at both ends
// going through a section backwards has the same length
void Optimize2Opt(std::vector<int>& order, const std::vector<SearchResult>& searches) {
    while (true) {
        // try the section flips and keep track of the biggest saving
        int bestSaving = 0;
        std::size_t bestFirst = 0, bestLast = 0;
        for (std::size_t first = 1; first + 2 < order.size(); ++first) {
            // leave the first and last home positions where they are
            for (std::size_t last = first + 1; last + 1 < order.size(); ++last) {
                const int before = order[first - 1], start = order[first];
                const int end = order[last], after = order[last + 1];
                // compare the two old connections with the two new ones
                const int oldLength = searches[before].distance[start] + searches[end].distance[after];
                const int newLength = searches[before].distance[end] + searches[start].distance[after];
                const int saving = oldLength - newLength;
                if (saving > bestSaving) {
                    bestSaving = saving;
                    bestFirst = first;
                    bestLast = last;
                }
            }
        }
        if (bestSaving == 0) {
            // stop when flipping a section cannot make the route shorter
            return;
        }
        std::reverse(order.begin() + bestFirst, order.begin() + bestLast + 1);
    }
}

// plan the route when a layout is picked
void PlanCleaning() {
    // save BFS results for the floor squares the robot can reach
    std::vector<SearchResult> searches(ROWS * COLS);
    searches[home] = BFS(home);
    const auto& floor = searches[home].order;
    // only count squares connected to home
    floorCount = static_cast<int>(floor.size());
    for (int cell : floor) {
        if (cell != home) {
            searches[cell] = BFS(cell);
        }
    }

    std::vector<int> bestOrder = GreedyOrder(searches);
    // use the same BFS distances to compare all the routes
    baselineMoves = RouteLength(bestOrder, searches);
    int bestLength = baselineMoves;
    std::vector<std::vector<int>> candidates{bestOrder};
    std::vector<int> cells(floor.begin() + 1, floor.end());
    // leave home out of the middle of the route

    // try going across the rows, then down the columns
    std::sort(cells.begin(), cells.end(), [](int first, int second) {
        const int firstRow = first / COLS, secondRow = second / COLS;
        if (firstRow != secondRow) {
            return firstRow < secondRow;
        }
        return firstRow % 2 == 0 ? first < second : first > second;
    });
    candidates.push_back(ClosedOrder(cells));
    std::sort(cells.begin(), cells.end(), [](int first, int second) {
        const int firstCol = first % COLS, secondCol = second % COLS;
        if (firstCol != secondCol) {
            return firstCol < secondCol;
        }
        return firstCol % 2 == 0 ? first < second : first > second;
    });
    candidates.push_back(ClosedOrder(cells));

    // use the same seed so replay gets the same plan on this build
    std::mt19937 random(42);
    for (int trial = 0; trial < 8; ++trial) {
        candidates.push_back(RandomNearestOrder(cells, searches, random));
    }
    for (int trial = 0; trial < 8; ++trial) {
        std::shuffle(cells.begin(), cells.end(), random);
        // also try completely shuffled visiting orders
        candidates.push_back(ClosedOrder(cells));
    }

    // keep the starting route as a backup and try improving every route
    // 2-opt can still miss a shorter route
    for (auto& candidate : candidates) {
        Optimize2Opt(candidate, searches);
        const int length = RouteLength(candidate, searches);
        if (length < bestLength) {
            // only keep the route if it uses fewer moves
            bestOrder = candidate;
            bestLength = length;
        }
    }

    // turn the chosen order into actual moves on the grid
    path.clear();
    for (std::size_t index = 1; index < bestOrder.size(); ++index) {
        const int from = bestOrder[index - 1], to = bestOrder[index];
        AppendShortestPath(path, from, to, searches[from]);
    }
    nextStep = 0;
}

// reset the robot when another layout is picked
void SelectScenario(HWND hwnd) {
    // stop the old run before setting up the new one
    KillTimer(hwnd, ROBOT_TIMER);
    running = finished = false;
    home = robot = -1;
    floorCount = cleanedCount = moves = 0;
    baselineMoves = 0;
    blocked.fill(false);
    cleaned.fill(false);
    path.clear();
    nextStep = 0;
    const LRESULT choice = SendMessageA(layoutMenu, CB_GETCURSEL, 0, 0);
    // 0 is the menu prompt and 1 to 5 are the layouts
    if (choice >= 1 && choice <= SCENARIO_COUNT) {
        const auto& scenario = scenarios[choice - 1];
        BlockAreas(scenario.walls);
        BlockAreas(scenario.cutouts);
        BlockAreas(scenario.furniture);
        // turn the home position into a square number
        home = scenario.homeRow * COLS + scenario.homeCol;
        robot = home;
        PlanCleaning();
        // clean the starting square without counting a move
        cleaned[home] = true;
        cleanedCount = 1;
    }
    SetWindowTextA(cleanButton, "Start cleaning");
    EnableWindow(cleanButton, home >= 0);
    // redraw the window with the new layout
    InvalidateRect(hwnd, nullptr, FALSE);
}

// check the route before starting or resuming
bool PlanIsValid() {
    if (home < 0 || home >= ROWS * COLS || blocked[home]) {
        return false;
    }
    std::array<bool, ROWS * COLS> visited{};
    visited[home] = true;
    int previous = home;
    for (int cell : path) {
        if (cell < 0 || cell >= ROWS * COLS || blocked[cell]) {
            return false;
        }
        const int rowDistance = std::abs(cell / COLS - previous / COLS);
        const int colDistance = std::abs(cell % COLS - previous % COLS);
        // each step must go to a neighbouring square
        if (rowDistance + colDistance != 1) {
            return false;
        }
        visited[cell] = true;
        previous = cell;
    }
    // check it goes home, cleans all reachable squares and stays within the starting move count
    return previous == home && std::count(visited.begin(), visited.end(), true) == floorCount &&
           static_cast<int>(path.size()) <= baselineMoves;
}

// move the robot one square and update the counters
void MoveRobot(HWND hwnd) {
    if (nextStep < path.size()) {
        robot = path[nextStep++];
        ++moves;
        // only count a square as cleaned the first time
        if (!cleaned[robot]) {
            cleaned[robot] = true;
            ++cleanedCount;
        }
    }
    if (nextStep == path.size()) {
        // stop the timer as soon as the last move is done
        KillTimer(hwnd, ROBOT_TIMER);
        running = false;
        finished = true;
        SetWindowTextA(cleanButton, "Replay cleaning");
    }
    InvalidateRect(hwnd, nullptr, FALSE);
}

// use the same button to start, pause, resume and replay
void ToggleCleaning(HWND hwnd) {
    if (home < 0) return;
    if (running) {
        KillTimer(hwnd, ROBOT_TIMER);
        running = false;
        SetWindowTextA(cleanButton, "Resume cleaning");
    } else {
        // reset everything for another run of the selected layout
        if (finished) SelectScenario(hwnd);
        if (!PlanIsValid()) {
            MessageBoxA(hwnd, "The planned route failed its coverage or return-home check. Choose a scenario again.",
                        "Roomba", MB_OK | MB_ICONERROR);
            return;
        }
        if (path.empty()) {
            // if only home is reachable, there are no moves to make
            MoveRobot(hwnd);
            return;
        }
        // move the robot about every 120 milliseconds
        if (!SetTimer(hwnd, ROBOT_TIMER, 120, nullptr)) {
            MessageBoxA(hwnd, "Could not start the timer.", "Roomba", MB_OK | MB_ICONERROR);
            return;
        }
        running = true;
        SetWindowTextA(cleanButton, "Pause cleaning");
    }
}

// turn grid coordinates into pixel coordinates
RECT CellBox(const CellRect& area) {
    return {LEFT + area.col * CELL, TOP + area.row * CELL,
            LEFT + (area.col + area.width) * CELL, TOP + (area.row + area.height) * CELL};
}

// fill a rectangle with the chosen colour
void FillColor(HDC dc, RECT box, COLORREF color) {
    SetDCBrushColor(dc, color);
    FillRect(dc, &box, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
}

// get the middle of a square for the trail and dirt dots
POINT CellCenter(int cell) {
    return {LEFT + (cell % COLS) * CELL + CELL / 2,
            TOP + (cell / COLS) * CELL + CELL / 2};
}

// move the pen to the start and draw the line
void DrawLine(HDC dc, int startX, int startY, int endX, int endY) {
    MoveToEx(dc, startX, startY, nullptr);
    LineTo(dc, endX, endY);
}

// put row and column numbers around the grid
void DrawGridNumbers(HDC dc) {
    const UINT center = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
    for (int col = 0; col < COLS; ++col) {
        RECT label{LEFT + col * CELL, TOP - 24, LEFT + (col + 1) * CELL, TOP - 2};
        const std::string number = std::to_string(col + 1);
        DrawTextA(dc, number.c_str(), -1, &label, center);
    }
    for (int row = 0; row < ROWS; ++row) {
        RECT label{6, TOP + row * CELL, LEFT - 6, TOP + (row + 1) * CELL};
        const std::string number = std::to_string(row + 1);
        DrawTextA(dc, number.c_str(), -1, &label, center);
    }
}

// draw the room outline, including the corner missing from the loft
void DrawRoomOutline(HDC dc, const Scenario& scenario) {
    std::array<bool, ROWS * COLS> inside;
    // start with a full rectangle and remove the outside areas
    inside.fill(true);
    for (const auto& area : scenario.cutouts) {
        for (int row = area.row; row < area.row + area.height; ++row) {
            for (int col = area.col; col < area.col + area.width; ++col) {
                inside[row * COLS + col] = false;
            }
        }
    }
    SelectObject(dc, GetStockObject(DC_PEN));
    SetDCPenColor(dc, LABEL_COLOR);
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            const int cell = row * COLS + col;
            if (!inside[cell]) continue;
            const RECT box = CellBox({col, row, 1, 1});
            // draw the edges that touch the outside of the room
            if (row == 0 || !inside[cell - COLS]) {
                DrawLine(dc, box.left, box.top, box.right, box.top);
            }
            if (row == ROWS - 1 || !inside[cell + COLS]) {
                DrawLine(dc, box.left, box.bottom, box.right, box.bottom);
            }
            if (col == 0 || !inside[cell - 1]) {
                DrawLine(dc, box.left, box.top, box.left, box.bottom);
            }
            if (col == COLS - 1 || !inside[cell + 1]) {
                DrawLine(dc, box.right, box.top, box.right, box.bottom);
            }
        }
    }
}

// draw the trail so far and use dots for repeated paths
void DrawTrail(HDC dc) {
    HPEN solid = CreatePen(PS_SOLID, 1, RGB(167, 135, 113));
    HPEN dotted = CreatePen(PS_DOT, 1, RGB(237, 216, 199));
    HGDIOBJ previousPen = SelectObject(dc, GetStockObject(DC_PEN));
    SetDCPenColor(dc, RGB(167, 135, 113));
    // count going back the other way as using the same path again
    std::array<std::array<bool, ROWS * COLS>, ROWS * COLS> crossed{};
    int previousCell = home;
    for (std::size_t index = 0; index < nextStep; ++index) {
        const int current = path[index];
        HPEN pen = crossed[previousCell][current] ? dotted : solid;
        SelectObject(dc, pen ? pen : GetStockObject(DC_PEN));
        const POINT start = CellCenter(previousCell), end = CellCenter(current);
        DrawLine(dc, start.x, start.y, end.x, end.y);
        crossed[previousCell][current] = true;
        crossed[current][previousCell] = true;
        previousCell = current;
    }
    SelectObject(dc, previousPen);
    // put the old pen back before deleting the temporary pens
    if (solid) DeleteObject(solid);
    if (dotted) DeleteObject(dotted);
}

// draw the floor, trail, furniture and robot
void DrawRoom(HDC dc, const Scenario& scenario) {
    SelectObject(dc, GetStockObject(DC_BRUSH));
    SelectObject(dc, GetStockObject(NULL_PEN));
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            RECT cell = CellBox({col, row, 1, 1});
            // leave a small gap so the grid lines show
            --cell.right;
            --cell.bottom;
            const COLORREF color = cleaned[row * COLS + col] ? RGB(98, 70, 59) : RGB(59, 48, 48);
            FillColor(dc, cell, color);
            if (!blocked[row * COLS + col] && !cleaned[row * COLS + col]) {
                // put a dot on squares that still need cleaning
                const POINT center = CellCenter(row * COLS + col);
                SetDCBrushColor(dc, RGB(139, 124, 117));
                Ellipse(dc, center.x - 1, center.y - 1, center.x + 1, center.y + 1);
            }
        }
    }
    for (const auto& wall : scenario.walls) {
        FillColor(dc, CellBox(wall), RGB(168, 164, 164));
    }
    for (const auto& cutout : scenario.cutouts) {
        // use the background colour for areas outside the room
        FillColor(dc, CellBox(cutout), BACKGROUND);
    }

    DrawGridNumbers(dc);
    DrawRoomOutline(dc, scenario);
    DrawTrail(dc);

    SelectObject(dc, GetStockObject(DC_BRUSH));
    SelectObject(dc, GetStockObject(DC_PEN));
    SetDCPenColor(dc, LABEL_COLOR);
    const UINT center = DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
    // centre the furniture names and shorten them if needed
    for (const auto& item : scenario.furniture) {
        RECT box = CellBox(item);
        SetDCBrushColor(dc, RGB(64, 53, 53));
        RoundRect(dc, box.left, box.top, box.right, box.bottom, 6, 6);
        DrawTextA(dc, item.title, -1, &box, center);
    }

    RECT dock = CellBox({scenario.homeCol, scenario.homeRow, 1, 1});
    // H marks home and R marks the robot
    DrawTextA(dc, "H", -1, &dock, center);
    RECT body = CellBox({robot % COLS, robot / COLS, 1, 1});
    InflateRect(&body, -9, -9);
    SetDCPenColor(dc, BACKGROUND);
    SetDCBrushColor(dc, RGB(249, 179, 140));
    Ellipse(dc, body.left, body.top, body.right, body.bottom);
    SetTextColor(dc, BACKGROUND);
    DrawTextA(dc, "R", -1, &body, center);
}

// draw the room and show the cleaning progress
void DrawScene(HDC dc, RECT client) {
    FillColor(dc, client, BACKGROUND);
    SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT));
    SetTextColor(dc, LABEL_COLOR);
    SetBkMode(dc, TRANSPARENT);

    RECT caption{LEFT, 9, LEFT + 280, 30};
    DrawTextA(dc, "Choose a home layout", -1, &caption, DT_LEFT | DT_SINGLELINE);
    const LRESULT choice = layoutMenu ? SendMessageA(layoutMenu, CB_GETCURSEL, 0, 0) : 0;
    if (choice < 1 || choice > SCENARIO_COUNT) return;
    DrawRoom(dc, scenarios[choice - 1]);
    const std::string status = "Cleaned " + std::to_string(cleanedCount) + "/" +
        std::to_string(floorCount) + " | Moves " + std::to_string(moves) + "/" +
        std::to_string(path.size()) + (finished ? " | Finished at home" : "");
    SetTextColor(dc, LABEL_COLOR);
    RECT statusBox{LEFT + 300, 9, LEFT + COLS * CELL, 30};
    DrawTextA(dc, status.c_str(), -1, &statusBox, DT_LEFT | DT_SINGLELINE);
    RECT legend{LEFT, TOP + ROWS * CELL + 5, LEFT + COLS * CELL, TOP + ROWS * CELL + 27};
    DrawTextA(dc, "Brown = clean   R = robot   H = home   Solid trail = first pass   Dotted = repeat",
              -1, &legend, DT_LEFT | DT_SINGLELINE);
    const int plannedMoves = static_cast<int>(path.size());
    // compare the starting route with the route picked after optimization
    const std::string comparison = "Before optimization " + std::to_string(baselineMoves) +
        " moves | After optimization " + std::to_string(plannedMoves) + " moves | Saved " +
        std::to_string(baselineMoves - plannedMoves);
    RECT comparisonBox{LEFT, TOP + ROWS * CELL + 28, LEFT + COLS * CELL, TOP + ROWS * CELL + 50};
    DrawTextA(dc, comparison.c_str(), -1, &comparisonBox, DT_LEFT | DT_SINGLELINE);
}

// draw the frame first, then show it all at once to reduce flickering
void PaintWindow(HWND hwnd) {
    PAINTSTRUCT paint{};
    // start drawing the part of the window that needs updating
    HDC dc = BeginPaint(hwnd, &paint);
    RECT client{};
    GetClientRect(hwnd, &client);
    const int width = client.right - client.left, height = client.bottom - client.top;
    if (width > 0 && height > 0) {
        HDC buffer = CreateCompatibleDC(dc);
        HBITMAP bitmap = buffer ? CreateCompatibleBitmap(dc, width, height) : nullptr;
        HGDIOBJ previousBitmap = bitmap ? SelectObject(buffer, bitmap) : nullptr;
        // draw straight to the window if the temporary buffer cannot be made
        const bool buffered = previousBitmap && previousBitmap != HGDI_ERROR;
        DrawScene(buffered ? buffer : dc, client);
        if (buffered) {
            if (!BitBlt(dc, 0, 0, width, height, buffer, 0, 0, SRCCOPY)) {
                DrawScene(dc, client);
            }
            SelectObject(buffer, previousBitmap);
        }
        // put the old bitmap back and free the drawing resources
        if (bitmap) DeleteObject(bitmap);
        if (buffer) DeleteDC(buffer);
    }
    EndPaint(hwnd, &paint);
}

// handle menu clicks, button clicks, timer ticks and window changes
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
    switch (message) {
    case WM_COMMAND:
        // check which control was used
        if (reinterpret_cast<HWND>(lp) == layoutMenu && HIWORD(wp) == CBN_SELCHANGE) {
            SelectScenario(hwnd);
        }
        if (reinterpret_cast<HWND>(lp) == cleanButton && HIWORD(wp) == BN_CLICKED) {
            ToggleCleaning(hwnd);
        }
        return 0;
    case WM_TIMER:
        // only move while the cleaning run is active
        if (wp == ROBOT_TIMER && running) MoveRobot(hwnd);
        return 0;
    case WM_ERASEBKGND:
        // skip the extra background erase to reduce flickering
        return 1;
    case WM_PAINT:
        PaintWindow(hwnd);
        return 0;
    case WM_DESTROY:
        // stop the robot and close the program
        KillTimer(hwnd, ROBOT_TIMER);
        running = false;
        PostQuitMessage(0);
        return 0;
    }
    // let Windows handle everything else
    return DefWindowProcA(hwnd, message, wp, lp);
}

// the program starts here
int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
    // set up the window class and its message function
    WNDCLASSA windowClass{};
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "RoombaMenuWindow";
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    if (!RegisterClassA(&windowClass)) {
        MessageBoxA(nullptr, "Could not register the application window.", "Roomba", MB_OK | MB_ICONERROR);
        return 1;
    }

    const DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
    // add space for the title bar and window border
    RECT size{0, 0, 2 * LEFT + COLS * CELL, TOP + ROWS * CELL + 56};
    AdjustWindowRect(&size, style, FALSE);
    HWND hwnd = CreateWindowA(windowClass.lpszClassName, "Roomba - BFS + 2-opt", style,
        CW_USEDEFAULT, CW_USEDEFAULT, size.right - size.left, size.bottom - size.top,
        nullptr, nullptr, instance, nullptr);
    if (!hwnd) {
        MessageBoxA(nullptr, "Could not create the application window.", "Roomba", MB_OK | MB_ICONERROR);
        return 1;
    }

    // add the prompt first, then the 5 layouts
    layoutMenu = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
        LEFT, 31, 280, 180, hwnd, nullptr, instance, nullptr);
    if (!layoutMenu) {
        MessageBoxA(hwnd, "Could not create the scenario menu.", "Roomba", MB_OK | MB_ICONERROR);
        DestroyWindow(hwnd);
        return 1;
    }
    SendMessageA(layoutMenu, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    SendMessageA(layoutMenu, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Choose a scenario..."));
    for (const auto& scenario : scenarios) {
        SendMessageA(layoutMenu, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(scenario.name));
    }
    SendMessageA(layoutMenu, CB_SETCURSEL, 0, 0);
    // keep the button off until a layout is picked
    cleanButton = CreateWindowA("BUTTON", "Start cleaning", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        LEFT + 300, 31, 160, 27, hwnd, nullptr, instance, nullptr);
    if (!cleanButton) {
        MessageBoxA(hwnd, "Could not create the cleaning button.", "Roomba", MB_OK | MB_ICONERROR);
        DestroyWindow(hwnd);
        return 1;
    }
    SendMessageA(cleanButton, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    EnableWindow(cleanButton, FALSE);
    ShowWindow(hwnd, show);
    SetFocus(layoutMenu);

    // keep checking for events until the window closes
    MSG message{};
    BOOL result;
    while ((result = GetMessageA(&message, nullptr, 0, 0)) > 0) {
        // let Tab move between the menu and button
        if (!IsDialogMessageA(hwnd, &message)) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
    if (result < 0) {
        // show an error if the message loop fails
        MessageBoxA(hwnd, "The application message loop failed.", "Roomba", MB_OK | MB_ICONERROR);
        DestroyWindow(hwnd);
        return 1;
    }
    return static_cast<int>(message.wParam);
}