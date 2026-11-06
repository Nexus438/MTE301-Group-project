#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <queue>
#include <random>
#include <string>
#include <vector>

constexpr int ROWS = 10;
constexpr int COLS = 14;
constexpr int CELL_COUNT = ROWS * COLS;

enum Tile { FREE, WALL, FURNITURE, OUTSIDE };
Tile grid[ROWS][COLS]{};

// Room coordinates start at zero. Each rectangle is column, row, width, height.
struct CellRect {
    int col;
    int row;
    int width;
    int height;
};

struct Furniture {
    CellRect area;
    const char* title;
};

struct Scenario {
    const char* name;
    int homeCol;
    int homeRow;
    std::vector<CellRect> walls;
    std::vector<CellRect> cutouts;
    std::vector<Furniture> furniture;
};

const Scenario scenarios[] = {
    {"Open studio", 0, 9, {{10, 0, 1, 2}, {10, 3, 4, 1}}, {}, {
        {{0, 0, 5, 1}, "Kitchen"}, {{0, 1, 1, 1}, "Fridge"},
        {{3, 2, 2, 1}, "Island"}, {{6, 1, 2, 2}, "Dining"},
        {{11, 0, 1, 1}, "Sink"}, {{12, 0, 1, 2}, "Shower"},
        {{13, 0, 1, 1}, "Toilet"}, {{11, 5, 3, 2}, "Bed"},
        {{13, 8, 1, 2}, "Wardrobe"}, {{1, 6, 4, 1}, "Sofa"},
        {{2, 8, 2, 1}, "Coffee table"}, {{0, 7, 1, 2}, "TV"}
    }},
    {"One-bedroom apartment", 0, 9,
        {{8, 0, 1, 3}, {8, 4, 1, 3}, {8, 8, 1, 2}, {9, 5, 2, 1}, {12, 5, 2, 1}}, {}, {
        {{0, 0, 6, 1}, "Kitchen"}, {{0, 1, 1, 1}, "Fridge"},
        {{4, 2, 2, 2}, "Dining"}, {{1, 6, 4, 1}, "Sofa"},
        {{2, 8, 2, 1}, "Coffee table"}, {{6, 6, 1, 2}, "TV"},
        {{10, 0, 3, 2}, "Bed"}, {{13, 0, 1, 4}, "Wardrobe"},
        {{9, 4, 2, 1}, "Desk"}, {{12, 6, 2, 2}, "Shower"},
        {{9, 9, 1, 1}, "Toilet"}, {{10, 9, 2, 1}, "Sink"}, {{9, 6, 1, 1}, "Laundry"}
    }},
    {"Two-bedroom home", 0, 9,
        {{6, 0, 1, 4}, {0, 4, 3, 1}, {4, 4, 6, 1}, {11, 4, 3, 1},
         {10, 6, 4, 1}, {10, 7, 1, 1}, {10, 9, 1, 1}}, {}, {
        {{1, 0, 3, 2}, "Bed A"}, {{5, 0, 1, 3}, "Wardrobe"},
        {{8, 0, 3, 2}, "Bed B"}, {{12, 0, 2, 1}, "Desk"},
        {{13, 2, 1, 2}, "Wardrobe"}, {{11, 5, 3, 1}, "Kitchen"},
        {{9, 6, 1, 1}, "Fridge"}, {{1, 6, 4, 1}, "Sofa"},
        {{2, 8, 2, 1}, "Coffee table"}, {{6, 7, 2, 2}, "Dining"},
        {{12, 7, 2, 1}, "Shower"}, {{13, 9, 1, 1}, "Toilet"}, {{11, 9, 1, 1}, "Sink"}
    }},
    {"Central hallway home", 6, 9,
        {{5, 0, 1, 2}, {5, 3, 1, 4}, {5, 8, 1, 2}, {0, 4, 5, 1},
         {8, 0, 1, 3}, {8, 4, 1, 3}, {8, 8, 1, 2}, {9, 5, 5, 1}}, {}, {
        {{0, 0, 4, 1}, "Sofa"}, {{1, 2, 2, 1}, "Coffee table"},
        {{4, 0, 1, 2}, "TV"}, {{1, 6, 3, 2}, "Bed"},
        {{0, 9, 3, 1}, "Wardrobe"}, {{9, 0, 5, 1}, "Kitchen"},
        {{10, 2, 2, 1}, "Island"}, {{10, 4, 2, 1}, "Dining"},
        {{12, 6, 2, 2}, "Shower"}, {{9, 6, 2, 1}, "Sink"},
        {{13, 9, 1, 1}, "Toilet"}, {{9, 9, 1, 1}, "Laundry"}
    }},
    {"L-shaped loft", 6, 9, {{0, 5, 5, 1}, {4, 6, 1, 2}, {4, 9, 1, 1}}, {{11, 0, 3, 3}}, {
        {{0, 0, 6, 1}, "Kitchen"}, {{0, 1, 1, 1}, "Fridge"},
        {{3, 2, 2, 1}, "Island"}, {{7, 1, 2, 2}, "Dining"},
        {{0, 3, 3, 2}, "Bed"}, {{7, 5, 4, 1}, "Sofa"},
        {{13, 4, 1, 3}, "TV"}, {{8, 7, 2, 1}, "Coffee table"},
        {{6, 8, 2, 1}, "Desk"}, {{12, 9, 1, 1}, "Plant"},
        {{0, 6, 2, 1}, "Shower"}, {{0, 9, 1, 1}, "Toilet"}, {{3, 6, 1, 1}, "Sink"}
    }}
};
constexpr int SCENARIO_COUNT = sizeof(scenarios) / sizeof(scenarios[0]);

bool InGrid(int row, int col) {
    return row >= 0 && row < ROWS && col >= 0 && col < COLS;
}

// A cell ID is row * COLS + column. Division and remainder recover its coordinates.
bool IsFree(int row, int col) {
    return InGrid(row, col) && grid[row][col] == FREE;
}