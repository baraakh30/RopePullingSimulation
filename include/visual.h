#ifndef VISUAL_H
#define VISUAL_H

void display();
void reshape(int w, int h);
void timer(int value);
void drawTeams();
void drawScore();
void drawRope();
int runVisualizer(int argc, char **argv, int pipe_fd, int ack_fd);

#endif // VISUAL_H