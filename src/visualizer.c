#include "../include/common.h"
#include "../include/referee.h"
#include "../include/visualizer.h"

static int windowWidth = 800;
static int windowHeight = 600;
static GameState currentGameState;
static int ack_pipe_fd;

int runVisualizer(int argc, char **argv, int pipe_fd, int ack_fd)
{
    ack_pipe_fd = ack_fd;

    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Rope Pulling Game");

    // Set up GLUT callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(100, timer, pipe_fd);

    // OpenGL setup
    glClearColor(0.0, 0.0, 0.0, 1.0);

    // Start GLUT main loop
    glutMainLoop();

    return 0;
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    drawRope();
    drawTeams();
    drawScore();

    glutSwapBuffers();
}

void timer(int pipe_fd)
{
    // Read game state from pipe
    ssize_t bytes_read = read(pipe_fd, &currentGameState, sizeof(GameState));
    if (bytes_read > 0)
    {
        // Redraw the scene
        glutPostRedisplay();

        char ack = 1;
        write(ack_pipe_fd, &ack, 1);
    }
    else if (bytes_read < 0 && errno != EAGAIN)
    {
        perror("Error reading from pipe");
    }

    // Call timer again for next frame
    if (currentGameState.gameRunning)
    {
        glutTimerFunc(100, timer, pipe_fd);
    }
}

void reshape(int w, int h)
{
    windowWidth = w;
    windowHeight = h;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
}

void drawTeams()
{
    // Team 1 (left side)
    glColor3f(1.0, 0.0, 0.0); // Red
    for (int i = 0; i < PLAYERS_PER_TEAM; i++)
    {
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
        sprintf(energyText, "E: %d", currentGameState.team1Messages[i].energy);
        glColor3f(1.0, 1.0, 1.0);
        glRasterPos2f(x - 15, y - 50);
        for (char *c = energyText; *c != '\0'; c++)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }
        glColor3f(1.0, 0.0, 0.0);
    }

    // Team 2 (right side)
    glColor3f(0.0, 0.0, 1.0); // Blue
    for (int i = 0; i < PLAYERS_PER_TEAM; i++)
    {
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
        sprintf(energyText, "E: %d", currentGameState.team2Messages[i].energy);
        glColor3f(1.0, 1.0, 1.0);
        glRasterPos2f(x - 15, y - 50);
        for (char *c = energyText; *c != '\0'; c++)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }
        glColor3f(0.0, 0.0, 1.0);
    }
}

void drawRope()
{
    // Calculate rope position based on effort difference
    float effortDifference = currentGameState.team1TotalEffort - currentGameState.team2TotalEffort;
    float maxOffset = windowWidth * 0.2;

    // FIXED: Negative the offset so rope moves toward winning team
    float offset = -1 * (effortDifference / config.winThreshold) * maxOffset;

    // Clamp the offset
    if (offset > maxOffset)
        offset = maxOffset;
    if (offset < -maxOffset)
        offset = -maxOffset;

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

void drawScore()
{
    char scoreText[100];
    sprintf(scoreText, "Team 1: %d | Team 2: %d | Draw: %d | Round: %d",
            currentGameState.team1Score, currentGameState.team2Score, currentGameState.draw, currentGameState.roundsPlayed + 1);

    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2f(windowWidth / 2 - 100, windowHeight - 20);
    for (char *c = scoreText; *c != '\0'; c++)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // Draw game status
    char statusText[100];

    if (!currentGameState.gameRunning)
    {
        if (currentGameState.team1Score > currentGameState.team2Score)
        {
            sprintf(statusText, "Game Over! Winner: Team 1");
        }
        else if (currentGameState.team2Score > currentGameState.team1Score)
        {
            sprintf(statusText, "Game Over! Winner: Team 2");
        }
        else
        {
            sprintf(statusText, "Game Over! Result: Draw");
        }
    }

    else
    {
        int remainingTime = config.gameDuration - (time(NULL) - currentGameState.gameStartTime);
        sprintf(statusText, "Difference: %d   Time Remaining: %d seconds", currentGameState.team1TotalEffort - currentGameState.team2TotalEffort, remainingTime > 0 ? remainingTime : 0);
    }

    glRasterPos2f(windowWidth / 2 - 100, windowHeight - 40);
    for (char *c = statusText; *c != '\0'; c++)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
}