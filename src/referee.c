#include "../include/referee.h"
#include "../include/visualizer.h"

Team team1;
Team team2;
time_t gameStartTime;
int roundsPlayed = 0;
bool roundActive = false;
int draw = 0;
PlayerMessage team1Messages[PLAYERS_PER_TEAM];
PlayerMessage team2Messages[PLAYERS_PER_TEAM];
static bool winPending = false;
static time_t winCandidateStartTime = 0;

void loadConfig(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        perror("Error opening config file");
        exit(EXIT_FAILURE);
    }

    // Read configuration values
    fscanf(fp, "minInitialEnergy = %d\n", &config.minInitialEnergy);
    fscanf(fp, "maxInitialEnergy = %d\n", &config.maxInitialEnergy);
    fscanf(fp, "minEnergyDecreaseRate = %d\n", &config.minEnergyDecreaseRate);
    fscanf(fp, "maxEnergyDecreaseRate = %d\n", &config.maxEnergyDecreaseRate);
    fscanf(fp, "minRejoiningTime = %d\n", &config.minRejoiningTime);
    fscanf(fp, "maxRejoiningTime = %d\n", &config.maxRejoiningTime);
    fscanf(fp, "winThreshold = %d\n", &config.winThreshold);
    fscanf(fp, "maxScore = %d\n", &config.maxScore);
    fscanf(fp, "consecutiveWinsNeeded = %d\n", &config.consecutiveWinsNeeded);
    fscanf(fp, "gameDuration = %d\n", &config.gameDuration);
    fscanf(fp, "waitBeforeWin = %d\n", &config.waitBeforeWin);
    fscanf(fp, "fallProbability = %d\n", &config.fallProbability);
    fclose(fp);
}

void initGame()
{
    // Initialize teams
    initTeam(&team1, 1);
    initTeam(&team2, 2);

    // Create player processes
    createPlayers(&team1);
    createPlayers(&team2);

    // Close write end of player_to_referee pipe in referee
    close(player_to_referee[1]);

    // Record start time
    gameStartTime = time(NULL);
}

void startGame()
{
    // Start the first round
    startNewRound();
}

void startNewRound()
{
    if (!gameRunning)
        return;

    printf("Starting new round...\n");
    roundActive = true;

    // Signal all players to get ready
    signalTeams(SIG_GET_READY);

    // Allow players to get ready
    usleep(500000); // 500ms

    // Signal players to start pulling before requesting state
    signalTeams(SIG_START_PULLING);

    // Allow players to start pulling
    usleep(500000); // 500ms


    requestPlayerStates();

    // Sort players by energy
    sortPlayersByEnergy(&team1);
    sortPlayersByEnergy(&team2);

    sendGameStateToVisualizer(referee_to_visualizer[1]);
}

void signalTeams(int signal)
{
    // Send signal to all players in both teams
    for (int i = 0; i < PLAYERS_PER_TEAM; i++)
    {
        kill(team1.players[i].pid, signal);
        kill(team2.players[i].pid, signal);
    }
}

// New function to request player states
void requestPlayerStates()
{
    // Reset message arrays
    memset(team1Messages, 0, sizeof(team1Messages));
    memset(team2Messages, 0, sizeof(team2Messages));

    // Signal all players to send their state
    for (int i = 0; i < PLAYERS_PER_TEAM; i++)
    {
        kill(team1.players[i].pid, SIG_REQUEST_STATE);
        kill(team2.players[i].pid, SIG_REQUEST_STATE);
    }
    // Now collect the responses
    collectPlayerEfforts();
}

void collectPlayerEfforts()
{
    PlayerMessage msg;
    int messagesToRead = PLAYERS_PER_TEAM * 2; // 4 players per team
    int messagesRead = 0;

    // Set up for select() with timeout
    fd_set readfds;
    struct timeval timeout;

    // Try for a maximum of 5 seconds total
    time_t start_time = time(NULL);

    while (messagesRead < messagesToRead && gameRunning)
    {
        // Check if we've been waiting too long (5 seconds total timeout)
        if (time(NULL) - start_time > 5)
        {
            printf("\nTimeout waiting for player messages: received %d/%d\n",
                   messagesRead, messagesToRead);
            break;
        }

        FD_ZERO(&readfds);
        FD_SET(player_to_referee[0], &readfds);

        // 100ms timeout for each select call
        timeout.tv_sec = 0;
        timeout.tv_usec = 200000;

        int ready = select(player_to_referee[0] + 1, &readfds, NULL, NULL, &timeout);

        if (ready == -1)
        {
            perror("select");
            break;
        }
        else if (ready == 0)
        {
            // Timeout occurred, no data available in this iteration
            continue;
        }

        // Data is available to read
        ssize_t bytes_read = read(player_to_referee[0], &msg, sizeof(PlayerMessage));

        if (bytes_read <= 0)
        {
            perror("Error reading player message");
            break;
        }

        if (msg.teamId == 1)
        {
            team1Messages[msg.playerId] = msg;
        }
        else if (msg.teamId == 2)
        {
            team2Messages[msg.playerId] = msg;
        }

        messagesRead++;
    }
    // Update efforts with whatever data we have, even if incomplete
    updateTotalEffort(&team1, team1Messages);
    updateTotalEffort(&team2, team2Messages);
}

void determineRoundWinner()
{
    int effortDifference = team1.totalEffort - team2.totalEffort;
    static int lastLeadingTeam = 0; // Track which team was leading
    printf("Team 1 Effort: %d\n", team1.totalEffort);
    printf("Team 2 Effort: %d\n", team2.totalEffort);
    printf("Difference: %d\n", effortDifference);
    checkGameEnd();
    if (!gameRunning)
        return;
    // Check if a team has won
    if (abs(effortDifference) >= config.winThreshold)
    {
        int currentLeadingTeam = (effortDifference > 0) ? 1 : 2;

        // If the leading team changed, reset the timer
        if (winPending && lastLeadingTeam != currentLeadingTeam)
        {
            printf("Lead changed from Team %d to Team %d! Resetting countdown.\n",
                   lastLeadingTeam, currentLeadingTeam);
            winPending = false;
        }

        if (!winPending)
        {
            winPending = true;
            winCandidateStartTime = time(NULL);
            lastLeadingTeam = currentLeadingTeam;
            printf("Win threshold exceeded by Team %d! Waiting %d seconds to confirm...\n",
                   currentLeadingTeam, config.waitBeforeWin);
        }
        else if (time(NULL) - winCandidateStartTime >= config.waitBeforeWin)
        {
            // Confirm the winner
            roundActive = false;

            if (effortDifference > 0)
            {
                team1.isWinner = true;
                team2.isWinner = false;
                team1Score++;
                printf("Team 1 wins the round!\n");

                if (lastWinner == 1)
                    consecutiveWins++;
                else
                {
                    consecutiveWins = 1;
                    lastWinner = 1;
                }
            }
            else
            {
                team2.isWinner = true;
                team1.isWinner = false;
                team2Score++;
                printf("Team 2 wins the round!\n");

                if (lastWinner == 2)
                    consecutiveWins++;
                else
                {
                    consecutiveWins = 1;
                    lastWinner = 2;
                }
            }

            roundsPlayed++;
            signalTeams(SIG_ROUND_END);
            checkGameEnd();
            winPending = false;
            lastLeadingTeam = 0; // Reset for next round

            if (gameRunning)
            {
                sendGameStateToVisualizer(referee_to_visualizer[1]);
                startNewRound();
            }
        }
    }
    else
    {
        // Reset win check if effort drops below threshold
        if (winPending && gameRunning)
        {
            printf("Win threshold no longer met. Resetting countdown.\n");
            winPending = false;
            lastLeadingTeam = 0;
        }
    }

    if (team1.totalEffort == 0 && team2.totalEffort == 0)
    {
        roundActive = false;
        draw++;
        printf("Both teams exhausted! Draw!.\n");
        roundsPlayed++;
        signalTeams(SIG_ROUND_END);
        checkGameEnd();
        if (gameRunning)
        {
            sendGameStateToVisualizer(referee_to_visualizer[1]);
            startNewRound();
        }
    }
}

// Wait for visualizer acknowledgment
void waitForVisualizerAck()
{
    // Set up timeout to avoid deadlock
    fd_set readfds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(visualizer_to_referee[0], &readfds);

    // 1 second timeout
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    int ready = select(visualizer_to_referee[0] + 1, &readfds, NULL, NULL, &timeout);

    if (ready == -1)
    {
        perror("select in waitForVisualizerAck");
        return;
    }
    else if (ready == 0)
    {
        printf("Timeout waiting for visualizer acknowledgment\n");
        return;
    }

    // Data is available to read
    char ack;
    if (read(visualizer_to_referee[0], &ack, 1) <= 0)
    {
        perror("Error reading visualizer acknowledgment");
    }
}

void sendGameStateToVisualizer(int pipe_fd)
{
    GameState state = {
        .draw = draw,
        .team1Score = team1Score,
        .team2Score = team2Score,
        .gameRunning = gameRunning,
        .roundsPlayed = roundsPlayed,
        .gameStartTime = gameStartTime,
        .team1TotalEffort = team1.totalEffort,
        .team2TotalEffort = team2.totalEffort};
    memcpy(state.team1Messages, team1Messages, sizeof(team1Messages));
    memcpy(state.team2Messages, team2Messages, sizeof(team2Messages));

    // Write to pipe and check if it succeeds
    ssize_t bytes_written = write(pipe_fd, &state, sizeof(GameState));
    if (bytes_written < 0)
    {
        perror("Error writing to visualizer pipe");
        return;
    }

    // Wait for visualizer acknowledgment
    waitForVisualizerAck();
}

void checkGameEnd()
{
    // Check time limit
    if (time(NULL) - gameStartTime >= config.gameDuration)
    {
        printf("Time's up! Game over.\n");
        gameRunning = false;
    }

    // Check score limit
    if (team1Score >= config.maxScore || team2Score >= config.maxScore)
    {
        printf("Score limit reached! Game over.\n");
        gameRunning = false;
    }

    // Check consecutive wins
    if (consecutiveWins >= config.consecutiveWinsNeeded)
    {
        printf("Team %d has won %d rounds in a row! Game over.\n",
               lastWinner, consecutiveWins);
        gameRunning = false;
    }

    if (!gameRunning)
    {
        printf("Final score - Team 1: %d, Team 2: %d, Draw : %d\n", team1Score, team2Score, draw);
    }
}

void cleanupGame()
{
    // Terminate all player processes
    for (int i = 0; i < PLAYERS_PER_TEAM; i++)
    {
        kill(team1.players[i].pid, SIGKILL);
        kill(team2.players[i].pid, SIGKILL);
    }

    // Close pipes
    close(player_to_referee[0]);
    close(referee_to_visualizer[1]);
    close(visualizer_to_referee[0]);
    close(visualizer_to_referee[1]);
}

// Main game loop function
void gameLoop()
{
    while (gameRunning && roundActive)
    {
        // Request player states
        requestPlayerStates();

        // Determine if there's a winner
        determineRoundWinner();

        // Send game state to visualizer
        sendGameStateToVisualizer(referee_to_visualizer[1]);

        // Sleep for 1 second
        sleep(1);
    }
    cleanupGame();
}