#include "../include/player.h"

static int playerId;
static int teamId;
static int energy;
static int position;
static int decreaseRate;
static bool fallen = false;
static bool pulling = false;
static int energyBeforeFall = 0;

void playerProcess(int id, int tid, int initialEnergy, int initialDecreaseRate, int initialPosition)
{
    playerId = id;
    teamId = tid;
    energy = initialEnergy;
    decreaseRate = initialDecreaseRate;
    position = initialPosition;    

    srand(time(NULL) ^ (getpid() << 16));

    // Set up signal handlers
    signal(SIG_GET_READY, handleGetReadySignal);
    signal(SIG_START_PULLING, handleStartPullingSignal);
    signal(SIG_ROUND_END, handleRoundEndSignal);
    signal(SIG_REQUEST_STATE, handleStateRequestSignal);  // New signal handler
    signal(SIGALRM, handleAlarmSignal);
    signal(SIG_UPDATE_POSITION, handlePositionUpdateSignal);
    
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

void handlePositionUpdateSignal(int sig)
{
    char positionBuf[3];  // Buffer to hold [teamId, playerId, position]
    ssize_t bytesRead;
    
    // Read the position message
    bytesRead = read(position_pipe[0], positionBuf, 3);
    
    if (bytesRead == 3) {
        // Check if this message is intended for this player
        int msgTeamId = positionBuf[0] - '0';
        int msgPlayerId = positionBuf[1] - '0';
        
        if (msgTeamId == teamId && msgPlayerId == playerId) {
            // Update position if the message is for this player
            position = positionBuf[2] - '0';
            
            printf("Player %d from Team %d position updated to %d\n", 
                   playerId, teamId, position);
            playerId = position;
        } else {
            // Return the message to the pipe for other players to read
            write(position_pipe[1], positionBuf, 3);
        }
    } else if (bytesRead > 0) {
        // Partial read, put back what we read
        write(position_pipe[1], positionBuf, bytesRead);
        fprintf(stderr, "Player %d from Team %d: partial position message read\n", 
                playerId, teamId);
    } else {
        // Handle error
        fprintf(stderr, "Player %d from Team %d could not read new position\n", 
                playerId, teamId);
    }
}

void calculateEffort()
{
    // Calculate weighted effort based on position (1, 2, 3, or 4)
   int weightedEffort = (pulling && !fallen) ? energy * (position + 1) : (pulling || fallen ? 0 : energy);

    // Send effort to referee
    PlayerMessage msg;
    msg.playerId = playerId;
    msg.teamId = teamId;
    msg.energy = weightedEffort;
    msg.position = position;

    write(player_to_referee[1], &msg, sizeof(PlayerMessage));
}