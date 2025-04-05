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
#include <sys/select.h>
#include <errno.h>

// Configuration structure
typedef struct
{
    int minInitialEnergy;
    int maxInitialEnergy;
    int minEnergyDecreaseRate;
    int maxEnergyDecreaseRate;
    int minRejoiningTime;
    int maxRejoiningTime;
    int winThreshold;
    int maxScore;
    int consecutiveWinsNeeded;
    int gameDuration;    // in seconds
    int waitBeforeWin;   // in seconds
    int fallProbability; // in % i.e. (5 = 5%)
} GameConfig;

// Message structure for player to referee communication
typedef struct
{
    int playerId;
    int teamId;
    int energy;
    int position;
} PlayerMessage;

extern GameConfig config;
extern int team1Score;
extern int team2Score;
extern int consecutiveWins;
extern int lastWinner;
extern bool gameRunning;
extern int draw;

// Pipe file descriptors
extern int player_to_referee[2];
extern int referee_to_visualizer[2];
extern int visualizer_to_referee[2];
extern int position_pipe[2];

// Signals
#define SIG_GET_READY SIGUSR1      
#define SIG_START_PULLING SIGUSR2   
#define SIG_ROUND_END SIGQUIT      
#define SIG_REQUEST_STATE SIGPIPE  
#define SIG_UPDATE_POSITION SIGSTKFLT 


#endif // COMMON_H