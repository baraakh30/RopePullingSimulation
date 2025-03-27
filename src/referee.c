#include "../include/referee.h"

static Team team1;
static Team team2;
static time_t gameStartTime;
static int roundsPlayed = 0;
static bool roundActive = false;

static PlayerMessage team1Messages[PLAYERS_PER_TEAM];
static PlayerMessage team2Messages[PLAYERS_PER_TEAM];

// Window dimensions
static int windowWidth = 800;
static int windowHeight = 600;
static bool winPending = false;
static time_t winCandidateStartTime = 0;
void startNewRound();

void loadConfig(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
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
    fclose(fp);
}

void initGame() {
    // Initialize teams
    initTeam(&team1, 1);
    initTeam(&team2, 2);
    
    // Set pipes to be non-blocking
    fcntl(player_to_referee[0], F_SETFL, O_NONBLOCK);
    
    // Create player processes
    createPlayers(&team1);
    createPlayers(&team2);
    
    // Record start time
    gameStartTime = time(NULL);
}

void startGame() {
    // Start the first round
    startNewRound();
}

void startNewRound() {
    if (!gameRunning) return;
    
    printf("Starting new round...\n");
    roundActive = true;
    
    // Signal all players to get ready
    signalTeams(SIG_GET_READY);
    
    // Allow time for all players to respond
    sleep(1);
    
    // Collect energy information
    collectPlayerEfforts();
    
    // Sort players by energy
    sortPlayersByEnergy(&team1);
    sortPlayersByEnergy(&team2);
    
    // Signal players to start pulling
    signalTeams(SIG_START_PULLING);
}

void signalTeams(int signal) {
    // Send signal to all players in both teams
    for (int i = 0; i < PLAYERS_PER_TEAM; i++) {
        kill(team1.players[i].pid, signal);
        kill(team2.players[i].pid, signal);
    }
}

void collectPlayerEfforts() {
    // Reset message arrays
    memset(team1Messages, 0, sizeof(team1Messages));
    memset(team2Messages, 0, sizeof(team2Messages));
    
    PlayerMessage msg;
    int bytesRead;
    
    // Read all available messages
    while ((bytesRead = read(player_to_referee[0], &msg, sizeof(PlayerMessage))) > 0) {
        // Process the message
        if (msg.teamId == 1) {
            team1Messages[msg.playerId] = msg;
        } else if (msg.teamId == 2) {
            team2Messages[msg.playerId] = msg;
        }
    }
    
    // Update team efforts
    updateTotalEffort(&team1, team1Messages);
    updateTotalEffort(&team2, team2Messages);
}

void determineRoundWinner() {
    int effortDifference = team1.totalEffort - team2.totalEffort;
    static int lastLeadingTeam = 0; // Track which team was leading
    
    printf("Team 1 Effort: %d\n", team1.totalEffort);
    printf("Team 2 Effort: %d\n", team2.totalEffort);
    printf("Difference: %d\n", effortDifference);
    
    // Check if a team has won
    if (abs(effortDifference) >= config.winThreshold) {
        int currentLeadingTeam = (effortDifference > 0) ? 1 : 2;
        
        // If the leading team changed, reset the timer
        if (winPending && lastLeadingTeam != currentLeadingTeam) {
            printf("Lead changed from Team %d to Team %d! Resetting countdown.\n", 
                   lastLeadingTeam, currentLeadingTeam);
            winPending = false;
        }
        
        if (!winPending) {
            winPending = true;
            winCandidateStartTime = time(NULL);
            lastLeadingTeam = currentLeadingTeam;
            printf("Win threshold exceeded by Team %d! Waiting %d seconds to confirm...\n", 
                   currentLeadingTeam,config.waitBeforeWin);
        } else if (time(NULL) - winCandidateStartTime >= config.waitBeforeWin) {
            // Confirm the winner
            roundActive = false;
    
            if (effortDifference > 0) {
                team1.isWinner = true;
                team2.isWinner = false;
                team1Score++;
                printf("Team 1 wins the round!\n");
    
                if (lastWinner == 1) consecutiveWins++;
                else { consecutiveWins = 1; lastWinner = 1; }
            } else {
                team2.isWinner = true;
                team1.isWinner = false;
                team2Score++;
                printf("Team 2 wins the round!\n");
    
                if (lastWinner == 2) consecutiveWins++;
                else { consecutiveWins = 1; lastWinner = 2; }
            }
    
            roundsPlayed++;
            signalTeams(SIG_ROUND_END);
            checkGameEnd();
            winPending = false;
            lastLeadingTeam = 0; // Reset for next round
    
            if (gameRunning) {
                sleep(1);
                startNewRound();
            }
        }
    } else {
        // Reset win check if effort drops below threshold
        if (winPending) {
            printf("Win threshold no longer met. Resetting countdown.\n");
            winPending = false;
            lastLeadingTeam = 0;
        }
    }
    
    if (team1.totalEffort == 0 && team2.totalEffort == 0) {
        roundActive = false;
        int winner = rand() % 2 + 1;
        if (winner == 1) {
            team1.isWinner = true;
            team2.isWinner = false;
            team1Score++;
            if (lastWinner == 1) consecutiveWins++;
            else { consecutiveWins = 1; lastWinner = 1; }
            lastWinner = 1;
        } else {
            team2.isWinner = true;
            team1.isWinner = false;
            team2Score++;
            if (lastWinner == 2) consecutiveWins++;
            else { consecutiveWins = 1; lastWinner = 2; }
            lastWinner = 2;
        }
        printf("Both teams exhausted! Randomly picking Team %d as winner.\n", winner);
        roundsPlayed++;
        signalTeams(SIG_ROUND_END);
        checkGameEnd();
        if (gameRunning) {
            sleep(1);
            startNewRound();
        }
    }
}

void checkGameEnd() {
    // Check time limit
    if (time(NULL) - gameStartTime >= config.gameDuration) {
        printf("Time's up! Game over.\n");
        gameRunning = false;
    }
    
    // Check score limit
    if (team1Score >= config.maxScore || team2Score >= config.maxScore) {
        printf("Score limit reached! Game over.\n");
        gameRunning = false;
    }
    
    // Check consecutive wins
    if (consecutiveWins >= config.consecutiveWinsNeeded) {
        printf("Team %d has won %d rounds in a row! Game over.\n", 
               lastWinner, consecutiveWins);
        gameRunning = false;
    }
    
    if (!gameRunning) {
        printf("Final score - Team 1: %d, Team 2: %d\n", team1Score, team2Score);
        
        // Clean up game resources when finished
        cleanupGame();
    }
}

void cleanupGame() {
    // Terminate all player processes
    for (int i = 0; i < PLAYERS_PER_TEAM; i++) {
        kill(team1.players[i].pid, SIGKILL);
        kill(team2.players[i].pid, SIGKILL);
    }
    
    // Close pipes
    close(player_to_referee[0]);
    close(player_to_referee[1]);
}

// OpenGL Functions

void initOpenGL(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Rope Pulling Game");
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(1000, timer, 0);
    
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    drawRope();
    drawTeams();
    drawScore();
    
    glutSwapBuffers();
}

void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
}

void timer(int value) {
    // Update game state
    if (gameRunning && roundActive) {
        collectPlayerEfforts();
        determineRoundWinner();
    }
    
    // Redraw the scene
    glutPostRedisplay();
    
    // Call timer again after 1s
    glutTimerFunc(1000, timer, 0);
}

void drawTeams() {
    // Team 1 (left side)
    glColor3f(1.0, 0.0, 0.0); // Red
    for (int i = 0; i < PLAYERS_PER_TEAM; i++) {
        float x = windowWidth * 0.25 - (i * 50);
        float y = windowHeight / 2;
        
        // Draw player
        glBegin(GL_QUADS);
        glVertex2f(x - 20, y - 40);
        glVertex2f(x + 20, y - 40);
        glVertex2f(x + 20, y + 40);
        glVertex2f(x - 20, y + 40);
        glEnd();
        
        // Draw player energy
        char energyText[20];
        sprintf(energyText, "E: %d", team1Messages[i].energy);
        glColor3f(1.0, 1.0, 1.0);
        glRasterPos2f(x - 15, y - 50);
        for (char *c = energyText; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }
        glColor3f(1.0, 0.0, 0.0);
    }
    
    // Team 2 (right side)
    glColor3f(0.0, 0.0, 1.0); // Blue
    for (int i = 0; i < PLAYERS_PER_TEAM; i++) {
        float x = windowWidth * 0.75 + (i * 50);
        float y = windowHeight / 2;
        
        // Draw player
        glBegin(GL_QUADS);
        glVertex2f(x - 20, y - 40);
        glVertex2f(x + 20, y - 40);
        glVertex2f(x + 20, y + 40);
        glVertex2f(x - 20, y + 40);
        glEnd();
        
        // Draw player energy
        char energyText[20];
        sprintf(energyText, "E: %d", team2Messages[i].energy);
        glColor3f(1.0, 1.0, 1.0);
        glRasterPos2f(x - 15, y - 50);
        for (char *c = energyText; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }
        glColor3f(0.0, 0.0, 1.0);
    }
}

void drawRope() {
    // Calculate rope position based on effort difference
    float effortDifference = team1.totalEffort - team2.totalEffort;
    float maxOffset = windowWidth * 0.2;
    
    // FIXED: Negative the offset so rope moves toward winning team
    float offset = -1 * (effortDifference / config.winThreshold) * maxOffset;
    
    // Clamp the offset
    if (offset > maxOffset) offset = maxOffset;
    if (offset < -maxOffset) offset = -maxOffset;
    
    // Draw the rope
    glColor3f(0.6, 0.4, 0.2); // Brown
    glLineWidth(10.0f);
    glBegin(GL_LINES);
    glVertex2f(windowWidth / 2 - 200 + offset, windowHeight / 2);
    glVertex2f(windowWidth / 2 + 200 + offset, windowHeight / 2);
    glEnd();
    
    // Draw the center mark
    glColor3f(1.0, 1.0, 1.0);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(windowWidth / 2, windowHeight / 2 - 50);
    glVertex2f(windowWidth / 2, windowHeight / 2 + 50);
    glEnd();
}

void drawScore() {
    char scoreText[100];
    sprintf(scoreText, "Team 1: %d | Team 2: %d | Round: %d", 
            team1Score, team2Score, roundsPlayed + 1);
    
    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2f(windowWidth / 2 - 100, windowHeight - 20);
    for (char *c = scoreText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
    
    // Draw game status
    char statusText[100];
    if (!gameRunning) {
        sprintf(statusText, "Game Over! Winner: Team %d", 
                team1Score > team2Score ? 1 : 2);
    } else {
        int remainingTime = config.gameDuration - (time(NULL) - gameStartTime);
        sprintf(statusText, "Time Remaining: %d seconds", remainingTime > 0 ? remainingTime : 0);
    }
    
    glRasterPos2f(windowWidth / 2 - 100, windowHeight - 40);
    for (char *c = statusText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
}