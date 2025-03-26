#include "common.h"
#include "referee.h"
#include "team.h"
#include "player.h"

GameConfig config;
int team1Score = 0;
int team2Score = 0;
int consecutiveWins = 0;
int lastWinner = 0;
bool gameRunning = true;
int player_to_referee[2];

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <config_file>\n", argv[0]);
        return 1;
    }

    // Seed the random number generator
    srand(time(NULL));
    
    // Load game configuration
    loadConfig(argv[1]);
    
    // Create pipe for communication
    if (pipe(player_to_referee) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    // Initialize OpenGL
    initOpenGL(argc, argv);
    
    // Initialize the game
    initGame();
    
    // Start the game simulation
    startGame();
    
    // Main OpenGL loop - this will be called after processes are forked
    glutMainLoop();
    
    return 0;
}