#pragma once

enum State {
    START,
    MOVE,
    TAKE_KETCHUP,
    DROP_KETCHUPS,
    END,
};

enum StateMove {
    CHECK_ORIENTATION,
    ROTATE,
    DRIVE_FORWARD,
    BACK_TO_LAST_NODE,
};

void switchState(State newState);

void loopStart();
void loopMove();
