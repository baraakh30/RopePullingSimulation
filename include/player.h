#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"

typedef struct
{
    int id;
    int teamId;
    int energy;
    int position;
    int decreaseRate;
    pid_t pid;
    bool fallen;
} Player;

// Function declarations
void playerProcess(int id, int tid, int initialEnergy, int initialDecreaseRate, int initialPosition);
void handleGetReadySignal(int sig);
void handleStartPullingSignal(int sig);
void handleRoundEndSignal(int sig);
void handleAlarmSignal(int sig);
void maybePlayerFalls();
void rejoinAfterFall();
void calculateEffort();
void handleStateRequestSignal(int sig);
void handlePositionUpdateSignal(int sig);

#endif // PLAYER_H