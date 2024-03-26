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

class Grid {
public:
    Grid() : m_nodes{} {

    }

    NodeType get(uint8_t x, uint8_t y) const;
    void clear();

    void addEnemy(const Position& pos, Heading ori);
    uint8_t countNonEmpty() const;

private:
    NodeType& at(uint8_t x, uint8_t y);

    std::array<NodeType, GRID_SIZE*GRID_SIZE> m_nodes;
};

typedef std::vector<Position> Path;

static constexpr Position STARTING_POSITION = { .x = 0, .y = 3 };

bool findPath(Grid& grid, Position src, Position dst, Path& out);
Heading orientationToPos(Position src, Position dst);
