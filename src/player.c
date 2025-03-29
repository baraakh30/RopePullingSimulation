#include "../include/player.h"

static int playerId;
static int teamId;
static int energy;
static int position;
static int decreaseRate;
static bool fallen = false;
static bool pulling = false;
static int energyBeforeFall = 0;

void playerProcess(int id, int tid)
{
    playerId = id;
    teamId = tid;
    srand(time(NULL) ^ (getpid() << 16));
    
    // Initialize player properties
    energy = rand() % (config.maxInitialEnergy - config.minInitialEnergy + 1) + config.minInitialEnergy;
    decreaseRate = rand() % (config.maxEnergyDecreaseRate - config.minEnergyDecreaseRate + 1) + config.minEnergyDecreaseRate;

    // Set up signal handlers
    signal(SIG_GET_READY, handleGetReadySignal);
    signal(SIG_START_PULLING, handleStartPullingSignal);
    signal(SIG_ROUND_END, handleRoundEndSignal);
    signal(SIG_REQUEST_STATE, handleStateRequestSignal);  // New signal handler
    signal(SIGALRM, handleAlarmSignal);
    
    // Player main loop
    while (gameRunning)
    {
        // Player is just waiting for signals
        pause();
    }

    exit(EXIT_SUCCESS);
}

void handleGetReadySignal(int sig)
{
    // Reset for new round
    fallen = false;
    pulling = false;

}

void handleStartPullingSignal(int sig)
{
    pulling = true;

}

void handleRoundEndSignal(int sig)
{
    pulling = false;

    // Regain some energy for next round
    energy = rand() % (config.maxInitialEnergy - config.minInitialEnergy + 1) + config.minInitialEnergy;
    decreaseRate = rand() % (config.maxEnergyDecreaseRate - config.minEnergyDecreaseRate + 1) + config.minEnergyDecreaseRate;
}

// handler for state requests from referee
void handleStateRequestSignal(int sig)
{
    if (pulling && !fallen)
    {
        // Reduce energy based on decreaseRate
        energy -= decreaseRate + rand() % 2; 
        if (energy < 0)
            energy = 0;

        // Random chance of falling
        maybePlayerFalls();
    }

    // Send current state to referee regardless of pulling status
    calculateEffort();
}

void maybePlayerFalls()
{
    // 1% chance of falling each second
    if (rand() % 100 < config.fallProbability)
    {
        pulling = false;
        fallen = true;
        printf("Player %d from Team %d has fallen!\n", playerId, teamId);
        energyBeforeFall = energy;
        energy = 0; // Set energy to zero when fallen

        // Wait some time to rejoin
        rejoinAfterFall();
    }
}

void rejoinAfterFall()
{
    int rejoinTime = rand() % (config.maxRejoiningTime - config.minRejoiningTime + 1) + config.minRejoiningTime;

    // Set an alarm to handle rejoining
    alarm(rejoinTime);
}

void handleAlarmSignal(int sig)
{
    if (fallen)
    {
        printf("Player %d from Team %d has rejoined!\n", playerId, teamId);
        fallen = false;
        // Regain some energy when rejoining
        energy = energyBeforeFall;

        pulling = true; 
 
    }
}

void calculateEffort()
{
    // Calculate weighted effort based on position (1, 2, 3, or 4)
    int weightedEffort = pulling && !fallen ? energy * (position + 1) : 0;

    // Send effort to referee
    PlayerMessage msg;
    msg.playerId = playerId;
    msg.teamId = teamId;
    msg.energy = weightedEffort;
    msg.position = position;

    write(player_to_referee[1], &msg, sizeof(PlayerMessage));
}