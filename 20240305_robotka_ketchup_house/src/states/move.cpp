#include <robotka.h>
#include <esp_log.h>

#include "../pathfinding.hpp"
#include "../states.hpp"


static StateMove gStateMove = StateMove::CHECK_ORIENTATION;
static Grid gGrid = {};

static Position gRobotPos = STARTING_POSITION;
static Heading gRobotHeading = Heading::RIGHT;
static Heading gTargetHeading = gRobotHeading;

static Path gCurrentPath;

static void fillStartingPath() {
    if(!findPath(gGrid, STARTING_POSITION, Position{ .x = 2, .y = 3}, gCurrentPath)) {
        abort();
    }
    // TODO: more points
}

static void startRotation(Heading target) {
    gTargetHeading = target;

    // TODO: ify nebo matika
    // na zjisteni smeru motoru
    const int leftMm = 150;
    const int rightMm = -150;

    rkMotorsDriveAsync(leftMm, rightMm, 100, []() {
        // TODO: implementovat failover
        ESP_LOGE("Move", "rkMotorsDriveAsync callback sooner than we found line!");
    });
}

static void startDriveForward() {
    const int leftMm = 150;
    const int rightMm = 150;

    rkMotorsDriveAsync(leftMm, rightMm, 100, []() {
        // TODO: implementovat failover
        ESP_LOGE("Move", "rkMotorsDriveAsync callback sooner than we found line!");
    });
}

static bool checkRotationDoneIR() {
    // TODO: implement
    return false;
}

static bool checkOnIntersectionIR() {
    // TODO: implement
    return false;
}



void loopMove() {
    // sendDebugData()

    switch(gStateMove) {
        case StateMove::CHECK_ORIENTATION: {
            if(gCurrentPath.empty()) {
                fillStartingPath();
            }

            const Position& next = gCurrentPath.back();
            const Heading needed = orientationToPos(gRobotPos, next);

            if(needed == gRobotHeading) {
                startDriveForward();
                gStateMove = StateMove::DRIVE_FORWARD;
            }  else {
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
                gRobotPos = gCurrentPath.back();
                gCurrentPath.pop_back();

                gStateMove = StateMove::CHECK_ORIENTATION;
            }
        }
    }
}
