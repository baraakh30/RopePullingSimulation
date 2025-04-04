#include "../include/common.h"
#include "../include/referee.h"
#include "../include/team.h"
#include "../include/player.h"
#include "../include/visualizer.h"

GameConfig config;
int team1Score = 0;
int team2Score = 0;
int consecutiveWins = 0;
int lastWinner = 0;
bool gameRunning = true;
int player_to_referee[2];
int referee_to_visualizer[2];
int visualizer_to_referee[2]; 
int position_pipe[2];

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s <config_file>\n", argv[0]);
        return 1;
    }

    // Seed the random number generator
    srand(time(NULL));

    // Load game configuration
    loadConfig(argv[1]);

    // Create pipes for communication
    if (pipe(player_to_referee) == -1 || pipe(referee_to_visualizer) == -1 || pipe(visualizer_to_referee) == -1 || pipe(position_pipe) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    // Fork for visualization process
    pid_t viz_pid = fork();

    if (viz_pid == -1)
    {
        perror("fork for visualization");
        exit(EXIT_FAILURE);
    }

    if (viz_pid == 0)
    {
        // Child process (Visualizer)
        close(player_to_referee[0]);
        close(player_to_referee[1]);
        close(position_pipe[0]);
        close(position_pipe[1]);
        close(referee_to_visualizer[1]); // Close write end in visualizer
        close(visualizer_to_referee[0]); // Close read end in visualizer

        // Run visualization
        runVisualizer(argc, argv, referee_to_visualizer[0], visualizer_to_referee[1]);
        exit(0);
    }
    sleep(1); // Give visualizer time to initialize

    // Parent process (Referee)
    close(referee_to_visualizer[0]); // Close read end in referee
    close(visualizer_to_referee[1]); // Close write end in referee

    // Initialize the game
    initGame();

    // Start the game simulation
    startGame();

    // Run the game loop
    gameLoop();

    return 0;
}
