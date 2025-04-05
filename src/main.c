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
pid_t viz_pid;

// Signal handler for visualizer exit
void handle_viz_exit(int sig)
{
    int status;
    pid_t pid;

    // Check if the visualizer has exited
    pid = waitpid(viz_pid, &status, WNOHANG);

    if (pid == viz_pid)
    {
        printf("Visualizer has closed. Cleaning up and exiting...\n");
        gameRunning = false;
        cleanupGame();
        exit(0);
    }
}

// Signal handler for keyboard interrupt
void handle_keyboard_interrupt(int sig)
{
    printf("Keyboard interrupt received. Cleaning up and exiting...\n");
    gameRunning = false;

    // Kill visualizer process if it's still running
    kill(viz_pid, SIGTERM);

    // Clean up game resources
    cleanupGame();
    exit(0);
}

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

    signal(SIGCHLD, handle_viz_exit);          // Handle visualizer exit
    signal(SIGINT, handle_keyboard_interrupt); // Handle Ctrl+C

    // Create pipes for communication
    if (pipe(player_to_referee) == -1 || pipe(referee_to_visualizer) == -1 || pipe(visualizer_to_referee) == -1 || pipe(position_pipe) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    // Fork for visualization process
    viz_pid = fork();

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
        close(referee_to_visualizer[1]);
        close(visualizer_to_referee[0]);

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
    int status;
    waitpid(viz_pid, &status, 0);

    printf("Game terminated. Cleaning up...\n");
    cleanupGame();
    return 0;
}
