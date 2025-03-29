#ifndef REFEREE_H
#define REFEREE_H

#include "common.h"
#include "team.h"

// Global variables 
extern int roundsPlayed;
extern time_t gameStartTime;
extern Team team1;
extern Team team2;
extern PlayerMessage team1Messages[PLAYERS_PER_TEAM];
extern PlayerMessage team2Messages[PLAYERS_PER_TEAM];

// Game state structure for visualization
typedef struct
{
    int draw;
    int team1Score;
    int team2Score;
    bool gameRunning;
    int roundsPlayed;
    int team1TotalEffort;
    int team2TotalEffort;
    PlayerMessage team1Messages[PLAYERS_PER_TEAM];
    PlayerMessage team2Messages[PLAYERS_PER_TEAM];
    time_t gameStartTime;
} GameState;

// Function declarations
void loadConfig(const char *filename);
void initGame();
void startGame();
void signalTeams(int signal);
void collectPlayerEfforts();
void determineRoundWinner();
void checkGameEnd();
void cleanupGame();
void startNewRound();
void gameLoop();
void sendGameStateToVisualizer(int pipe_fd);
void waitForVisualizerAck();
void requestPlayerStates();



#endif // REFEREE_H