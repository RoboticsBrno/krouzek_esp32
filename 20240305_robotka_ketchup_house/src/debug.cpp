#include <vector>

#include "debug.hpp"
#include "states.hpp"
#include "pathfinding.hpp"

enum DebugUartState {
    START_BYTE,
    COMMAND,
    LENGTH,
    DATA,
};

static DebugUartState gState = DebugUartState::START_BYTE;
static uint8_t gCommand = 0;
static uint8_t gDataLength = 0;
static std::vector<uint8_t> gData;

void debugSendHeader(uint8_t command, uint8_t length) {
    const uint8_t header[] = {
        0xFF,
        command,
        length
    };
    uart_write_bytes(UART_NUM_0, (char*)header, sizeof(header));
}



static void debugHandleCommand() {
    switch(gCommand) {
        case DebugCommand::SET_STATE:
            switchState((State)gData[0]);
            break;
        case DebugCommand::TEST_PATHFINDING: {
            Grid grid;
            Path path;
            bool last_res = true;

            Position enemy = { .x = gData[0], .y = gData[1] };
            Heading enemyOri = (Heading)gData[2];
            grid.addEnemy(enemy, enemyOri);

            Position start = { .x = gData[3], .y = gData[4] };
            Position dst;

            for(int i = 5; i < gData.size() && last_res; i += 2) {
                dst.x = gData[i];
                dst.y = gData[i+1];
                last_res = findPath(
                    grid,
                    start,
                    dst,
                    path
                );
                start = dst;
            }

            const uint8_t grid_len = grid.countNonEmpty();

            debugSendHeader(DebugCommand::PATHFINDING_RESULT, 1 + 1 + path.size()*2 + 1 + grid_len*3);
            uart_write_bytes(UART_NUM_0, (char*)&last_res, 1);

            const uint8_t path_len = path.size();
            uart_write_bytes(UART_NUM_0, (char*)&path_len, 1);
            for(auto& pos : path) {
                uart_write_bytes(UART_NUM_0, (char*)&pos, 2);
            }

            uart_write_bytes(UART_NUM_0, (char*)&grid_len, 1);
            for(uint8_t x = 0; x < GRID_SIZE; ++x) {
                for(uint8_t y = 0; y < GRID_SIZE; ++y) {
                    const auto n = grid.get(x, y);
                    if(n == NodeType::EMPTY) {
                        continue;
                    }

                    uart_write_bytes(UART_NUM_0, (char*)&x, 1);
                    uart_write_bytes(UART_NUM_0, (char*)&y, 1);
                    uart_write_bytes(UART_NUM_0, (char*)&n, 1);
                }
            }

            break;
        }
    }
    gData.clear();
    gState = DebugUartState::START_BYTE;
}

void debugPollUartRx() {
    while(true) {
        uint8_t ch;
        if(uart_read_bytes(UART_NUM_0, &ch, 1, 0) < 1) {
            return;
        }

        switch(gState) {
            case DebugUartState::START_BYTE:
                if(ch == 0xFF) {
                    gState = DebugUartState::COMMAND;
                }
                break;
            case DebugUartState::COMMAND:
                gCommand = ch;
                gState = DebugUartState::LENGTH;
                break;
            case DebugUartState::LENGTH:
                if(ch == 0) {
                    debugHandleCommand();
                } else {
                    gDataLength = ch;
                    gState = DebugUartState::DATA;
                }
                break;
            case DebugUartState::DATA:
                gData.push_back(ch);
                if(--gDataLength == 0) {
                    debugHandleCommand();
                }
                break;
        }
    }
}
