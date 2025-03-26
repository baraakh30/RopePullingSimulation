#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <GL/glut.h>
#include <fcntl.h>
#include <sys/stat.h>

// Configuration structure
typedef struct {
    int minInitialEnergy;
    int maxInitialEnergy;
    int minEnergyDecreaseRate;
    int maxEnergyDecreaseRate;
    int minRejoiningTime;
    int maxRejoiningTime;
    int winThreshold;
    int maxScore;
    int consecutiveWinsNeeded;
    int gameDuration;  // in seconds
} GameConfig;

// Message structure for player to referee communication
typedef struct {
    int playerId;
    int teamId;
    int energy;
    int position;
} PlayerMessage;

// Game state
extern GameConfig config;
extern int team1Score;
extern int team2Score;
extern int consecutiveWins;
extern int lastWinner;
extern bool gameRunning;

// Pipe file descriptors
extern int player_to_referee[2];

// Signals
#define SIG_GET_READY SIGUSR1
#define SIG_START_PULLING SIGUSR2
#define SIG_ROUND_END SIGTERM

#endif // COMMON_H