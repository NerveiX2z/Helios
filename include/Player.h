#ifndef PLAYER_H
#define PLAYER_H

#include <vector>

class Player {
public:
    double x, y;
    double dirX, dirY;
    double planeX, planeY;
    double speed, rotSpeed;

    Player(double startX, double startY);
    // deltaTime is elapsed time in seconds since the previous update.
    void update(const std::vector<std::vector<int>>& map, double deltaTime);
};

#endif
