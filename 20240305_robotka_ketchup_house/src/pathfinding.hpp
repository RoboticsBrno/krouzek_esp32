#pragma once

#include <stdint.h>
#include <vector>
#include <array>

static constexpr uint8_t GRID_SIZE = 7;

enum NodeType : uint8_t {
    EMPTY = 0,
    ENEMY = (1 << 0),
    WALL = (1 << 1),
};

typedef std::array<NodeType, GRID_SIZE*GRID_SIZE> Grid;

enum Heading {
    UP,
    RIGHT,
    DOWN,
    LEFT,
};

struct Position {
    uint8_t x;
    uint8_t y;
};

static constexpr Position STARTING_POSITION = { .x = 0, .y = 3 };

typedef std::vector<Position> Path;

bool findPath(Grid& grid, Position src, Position dst, Path& out);

Heading orientationToPos(Position src, Position dst);
