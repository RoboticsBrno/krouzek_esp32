#include <vector>
#include <memory>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <cmath>
#include <algorithm>

#include "pathfinding.hpp"

struct AsNode {
    uint8_t x;
    uint8_t y;

    uint16_t gScore;
    uint16_t fScore;
    AsNode *parent;

    bool in_queue;
};

class UsedNodesHolder {
public:
    AsNode *get(uint8_t x, uint8_t y) {
        const uint16_t key = x*GRID_SIZE + y;

        auto itr = m_nodes.find(key);
        if(itr != m_nodes.end()) {
            return itr->second.get();
        }

        auto *node = new AsNode {
            .x = x,
            .y = y,
            .gScore = 0xFFFF,
            .fScore = 0xFFFF,
            .parent = nullptr,
            .in_queue = false,
        };
        m_nodes[key] = std::unique_ptr<AsNode>(node);
        return node;
    }

    AsNode *getClosest() {
        AsNode *closest = nullptr;
        for(const auto& it : m_nodes) {
            if(closest == nullptr || closest->fScore > it.second->fScore) {
                closest = it.second.get();
            }
        }
        return closest;
    }

private:
    std::unordered_map<uint16_t, std::unique_ptr<AsNode>> m_nodes;
};



static uint16_t manhattanDistance(AsNode *a, Position dest) {
    return std::abs(a->x - dest.x) + std::abs(a->y - dest.y);
}

bool findPath(Grid& grid, Position src, Position dst, Path& out) {
    UsedNodesHolder all_nodes;

    auto asNodeCompare = [](AsNode *left, AsNode *right) {
        return left->fScore > right->fScore;
    };
    std::priority_queue<AsNode*, std::vector<AsNode*>, decltype(asNodeCompare)>
        open_set_queue(asNodeCompare);

    auto *start_node = all_nodes.get(src.x, src.y);
    start_node->gScore = 0;
    start_node->fScore = UINT16_MAX;
    open_set_queue.push(start_node);

    bool found = false;
    AsNode *current = nullptr;

    while(!open_set_queue.empty()) {
        current = open_set_queue.top();
        current->in_queue = false;
        open_set_queue.pop();

        // Found destination
        if(current->x == dst.x && current->y == dst.y) {
            found = true;
            break;
        }

        const Position neighbors[4] = {
            { .x = current->x - 1, .y = current->y },
            { .x = current->x + 1, .y = current->y },
            { .x = current->x, .y = current->y - 1 },
            { .x = current->x, .y = current->y + 1 },
        };

        for(auto neighborPos : neighbors) {
            // ignore outside of grid
            if(neighborPos.x >= GRID_SIZE || neighborPos.y >= GRID_SIZE) {
                continue;
            }

            // ignore non-empty cells
            if(grid.get(neighborPos.x, neighborPos.y) != NodeType::EMPTY) {
                continue;
            }

            auto *neighbor = all_nodes.get(neighborPos.x, neighborPos.y);

            uint16_t turning_penalty = 0;
            if(current->parent && current->parent->x != neighbor->x && current->parent->y != neighbor->y) {
                turning_penalty = 1;
            }

            const auto current_gScore = current->gScore + 1 + turning_penalty; // TODO add cost to turning

            if(current_gScore < neighbor->gScore) {
                neighbor->parent = current;
                neighbor->gScore = current_gScore;
                neighbor->fScore = current_gScore + manhattanDistance(neighbor, dst);

                if(!neighbor->in_queue) {
                    neighbor->in_queue = true;
                    open_set_queue.push(neighbor);
                }
            }
        }
    }

    // if not found, try to pick nearest
    if(!found) {
        current = all_nodes.getClosest();
    }

    const auto original_out_size = out.size();
    while(current != nullptr) {
        out.insert(out.begin() + original_out_size, Position{ .x = current->x, .y = current->y });
        current = current->parent;
    }

    return found;
}

Heading orientationToPos(Position src, Position dst) {
    // TODO: implement
    return Heading::DOWN;
}

NodeType Grid::get(uint8_t x, uint8_t y) const {
    return m_nodes[x + y*GRID_SIZE];
}

NodeType& Grid::at(uint8_t x, uint8_t y) {
    return m_nodes[x + y*GRID_SIZE];
}

void Grid::clear() {
    m_nodes.fill(NodeType::EMPTY);
}

void Grid::addEnemy(const Position& pos, Heading ori) {
    static constexpr float PI = 3.141592f;

    const float oriRads = ((float(ori)/4.f) * PI*2) - PI/2;
    const float diff_x = round(cos(oriRads));
    const float diff_y = round(sin(oriRads)) * -1;

    uint8_t x = pos.x;
    uint8_t y = pos.y;
    while(x < GRID_SIZE && y < GRID_SIZE) {
        at(x, y) = NodeType::ENEMY;

        x += diff_x;
        y += diff_y;
    }
}

uint8_t Grid::countNonEmpty() const {
    return std::count_if(m_nodes.begin(), m_nodes.end(), [](NodeType n) { return n != NodeType::EMPTY; });
}
