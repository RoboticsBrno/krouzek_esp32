#include <robotka.h>
#include <esp_log.h>
#include <functional>

#include "../pathfinding.hpp"
#include "../states.hpp"
#include "move_utils.hpp"

typedef std::function<void()> GoalCb;

struct Goal {
    Goal(Position target) {
        this->target = target;
    }

    Goal(uint8_t x, uint8_t y) {
        this->target.x = x;
        this->target.y = y;
    }

    Goal&& setOnTargetReached(GoalCb cb) {
        this->onTargetReached = cb;
        return std::move(*this);
    }

    Goal&& setOnPositionChanged(GoalCb cb) {
        this->onPositionChanged = cb;
        return std::move(*this);
    }

    Position target;
    GoalCb onTargetReached;
    GoalCb onPositionChanged;
};

static StateMove gStateMove = StateMove::CHECK_ORIENTATION;
static Grid gGrid = {};

static Position gRobotPos = STARTING_POSITION;
static Heading gRobotHeading = Heading::RIGHT;
static Heading gTargetHeading = gRobotHeading;
static bool gKetchupDoorClosed = false;

static std::vector<Goal> gCurrentGoals;
static Path gCalculatedPath;

static void fillStartingGoals();

static void recalculatePath() {
    Position start = gRobotPos;
    for(const auto& goal : gCurrentGoals) {
        if(!findPath(gGrid, start, goal.target, gCalculatedPath)) {
            abort(); // TODO: bude stačit jen počkat?
        }
        start = goal.target;
    }
}

static void onUnloadPositionReached() {
    startDriveBackwards();
    gCalculatedPath.clear();

    gRobotPos.y += 1; // BACK_TO_LAST_NODE očekává, že v gRobotPos je ještě předchozí pozice
    gStateMove = StateMove::BACK_TO_LAST_NODE;

    fillStartingGoals();
}

static void onKetchupCollectPositionReached() {
    if(!gKetchupDoorClosed) {
        return;
    }

    gCurrentGoals.clear();
    gCalculatedPath.clear();

    gCurrentGoals.push_back(Goal(1, 5));
    gCurrentGoals.push_back(Goal(0, 5));
    gCurrentGoals.push_back(
        Goal(0, 1)
            .setOnTargetReached(onUnloadPositionReached)
    );
}

static void fillStartingGoals() {
    gCurrentGoals.clear();
    gCalculatedPath.clear();

    gCurrentGoals.push_back(Goal(2, 3).setOnPositionChanged(onKetchupCollectPositionReached));
    gCurrentGoals.push_back(Goal(2, 6).setOnPositionChanged(onKetchupCollectPositionReached));
    gCurrentGoals.push_back(Goal(3, 6).setOnPositionChanged(onKetchupCollectPositionReached));
    gCurrentGoals.push_back(Goal(3, 0).setOnPositionChanged(onKetchupCollectPositionReached));
}

void loopMove() {
    // sendDebugData()

    // Kontrola stop tlačítka
    if(rkButtonIsPressed(BTN_UP)) {
        rkMotorsSetPower(0, 0);
        switchState(State::END);
        return;
    }

    // Kontrola a detekce pozice soupeře
    if(checkIsEnemyClose()) {
        rkMotorsSetPower(0, 0);
        startDriveBackwards();

        gCalculatedPath.clear();
        // TODO: přidat "stěnu" soupeře do mapy pathfindingu

        gStateMove = StateMove::BACK_TO_LAST_NODE;
    }

    // kontrola, jestli zrovna bereme kečup
    if(isRobotFullOfKetchup()) {
        // closeRobotKetchupDoor();
        gKetchupDoorClosed = true;
    }

    switch(gStateMove) {
        case StateMove::CHECK_ORIENTATION: {
            if(gCurrentGoals.empty()) {
                fillStartingGoals();
            }

            if(gCalculatedPath.empty()) {
                recalculatePath();
            }

            const Position& next = gCalculatedPath.back();
            const Heading needed = orientationToPos(gRobotPos, next);

            if(needed == gRobotHeading) {
                startDriveForward();
                gStateMove = StateMove::DRIVE_FORWARD;
            }  else {
                gTargetHeading = needed;
                startRotation(needed);
                gStateMove = StateMove::ROTATE;
            }
            break;
        }
        case StateMove::ROTATE: {
            if(checkRotationDoneIR()) {
                rkMotorsSetPower(0, 0);
                gRobotHeading = gTargetHeading;
                gStateMove = StateMove::CHECK_ORIENTATION;
            }
            break;
        }
        case StateMove::DRIVE_FORWARD: {
            if(checkOnIntersectionIR()) {
                rkMotorsSetPower(0, 0);
                gRobotPos = gCalculatedPath.back();
                gCalculatedPath.pop_back();

                gStateMove = StateMove::CHECK_ORIENTATION;

                const Goal goal = gCurrentGoals.back();
                if(memcmp(&goal.target, &gRobotPos, sizeof(Position)) == 0) {
                    gCurrentGoals.pop_back();
                    if(goal.onTargetReached) {
                        goal.onTargetReached();
                    }
                } else if(goal.onPositionChanged) {
                    goal.onPositionChanged();
                }
            }
            break;
        }
        case StateMove::BACK_TO_LAST_NODE: {
            if(checkOnIntersectionIR()) {
                rkMotorsSetPower(0, 0);
                gStateMove = StateMove::CHECK_ORIENTATION;
            }
            break;
        }
    }
}
