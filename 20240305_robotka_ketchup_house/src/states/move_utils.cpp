#include "robotka.h"
#include "../pathfinding.hpp"

void startRotation(Heading target) {
    // TODO: ify nebo matika
    // na zjisteni smeru motoru
    const int leftMm = 150;
    const int rightMm = -150;

    rkMotorsDriveAsync(leftMm, rightMm, 100, []() {
        // TODO: implementovat failover
        ESP_LOGE("Move", "rkMotorsDriveAsync callback sooner than we found line!");
    });
}

void startDriveForward() {
    const int leftMm = 150;
    const int rightMm = 150;

    rkMotorsDriveAsync(leftMm, rightMm, 100, []() {
        // TODO: implementovat failover
        ESP_LOGE("Move", "rkMotorsDriveAsync callback sooner than we found line!");
    });
}

void startDriveBackwards() {
    const int leftMm = -150;
    const int rightMm = -150;

    rkMotorsDriveAsync(leftMm, rightMm, 100, []() {
        // TODO: implementovat failover
        ESP_LOGE("Move", "rkMotorsDriveAsync callback sooner than we found line!");
    });
}

bool checkRotationDoneIR() {
    // TODO: implement
    return false;
}

bool checkOnIntersectionIR() {
    // TODO: implement
    return false;
}

bool checkIsEnemyClose() {
    // TODO: implement check via ultrasound range meters
    //
    return false;
}

bool isRobotFullOfKetchup() {
    // TODO: implement check based on some button
    return false;
}
