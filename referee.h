#ifndef REFEREE_H
#define REFEREE_H

#include "common.h"
#include "team.h"

// Function declarations
void loadConfig(const char *filename);
void initGame();
void startGame();
void signalTeams(int signal);
void collectPlayerEfforts();
void determineRoundWinner();
void updateScores();
void checkGameEnd();
void cleanupGame();
void renderGame();

// OpenGL related functions
void initOpenGL(int argc, char **argv);
void display();
void reshape(int w, int h);
void timer(int value);
void drawTeams();
void drawScore();
void drawRope();

#endif // REFEREE_H