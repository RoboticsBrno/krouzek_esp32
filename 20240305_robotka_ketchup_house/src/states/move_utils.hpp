#pragma once

#include "../pathfinding.hpp"

void startRotation(Heading target);

void startDriveForward();

void startDriveBackwards();
bool checkRotationDoneIR();
bool checkOnIntersectionIR();

bool checkIsEnemyClose();

bool isRobotFullOfKetchup();
