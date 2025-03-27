#include "../include/player.h"

static int playerId;
static int teamId;
static int energy;
static int position;
static int decreaseRate;
static bool fallen = false;
static bool pulling = false;

void playerProcess(int id, int tid) {
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
    signal(SIGALRM, handleAlarmSignal);
    // Player main loop
    while (gameRunning) {
        // Player is just waiting for signals
        pause();
    }
    
    exit(EXIT_SUCCESS);
}

void handleGetReadySignal(int sig) {
    // Reset for new round
    fallen = false;
    pulling = false;
    
    // Send current energy to referee so teams can be aligned
    PlayerMessage msg;
    msg.playerId = playerId;
    msg.teamId = teamId;
    msg.energy = energy;
    msg.position = -1; // Not assigned yet
    
    write(player_to_referee[1], &msg, sizeof(PlayerMessage));
}

void handleStartPullingSignal(int sig) {
    pulling = true;
    
    // Pulling loop that runs in player process
    while (pulling && !fallen && energy >= 0) {
        // Reduce energy based on decreaseRate
        energy -= decreaseRate + rand() % 2;  // slightly random drain
        if (energy < 0) energy = 0;
        
        // Random chance of falling
        maybePlayerFalls();
        
        // Send current effort to referee
        calculateEffort();
        
        // Sleep briefly
        usleep(100000); // 100ms
    }
}

void handleRoundEndSignal(int sig) {
    pulling = false;
    
    // Regain some energy for next round
    energy = rand() % (config.maxInitialEnergy - config.minInitialEnergy + 1) + config.minInitialEnergy;
    decreaseRate = rand() % (config.maxEnergyDecreaseRate - config.minEnergyDecreaseRate + 1) + config.minEnergyDecreaseRate;

    pause();
}
static int energyBeforeFall = 0;
void maybePlayerFalls() {
    // 1% chance of falling each second
    if (rand() % 100 == 0) {
        pulling = false;
        fallen = true;
        printf("Player %d from Team %d has fallen!\n", playerId, teamId);
        energyBeforeFall = energy;
        // Notify referee of zero energy
        PlayerMessage msg;
        msg.playerId = playerId;
        msg.teamId = teamId;
        msg.energy = 0;
        msg.position = position;
        
        write(player_to_referee[1], &msg, sizeof(PlayerMessage));
        
        // Wait some time to rejoin
        rejoinAfterFall();
    }
}

void rejoinAfterFall() {
    int rejoinTime = rand() % (config.maxRejoiningTime - config.minRejoiningTime + 1) + config.minRejoiningTime;
    
    // Set an alarm to handle rejoining
    alarm(rejoinTime);
}

void handleAlarmSignal(int sig) {
    if (fallen) {
        pulling = true;
        printf("Player %d from Team %d has rejoined!\n", playerId, teamId);
        fallen = false;
        // Regain some energy when rejoining
        energy = energyBeforeFall;    
        
        // Notify referee of rejoining
        calculateEffort();
    }
}

void calculateEffort() {
    if (!pulling || fallen) return;
    
    // Calculate weighted effort based on position (1, 2, 3, or 4)
    int weightedEffort = energy * (position + 1) ; 
    
    // Send effort to referee
    PlayerMessage msg;
    msg.playerId = playerId;
    msg.teamId = teamId;
    msg.energy = weightedEffort;
    msg.position = position;
    
    write(player_to_referee[1], &msg, sizeof(PlayerMessage));
}