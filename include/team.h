#ifndef TEAM_H
#define TEAM_H

#include "common.h"
#include "player.h"

#define PLAYERS_PER_TEAM 4

typedef struct {
    int id;
    Player players[PLAYERS_PER_TEAM];
    int totalEffort;
    bool isWinner;
} Team;

// Function declarations
void initTeam(Team *team, int teamId);
void sortPlayersByEnergy(Team *team);
void updateTotalEffort(Team *team, PlayerMessage messages[PLAYERS_PER_TEAM]);
void createPlayers(Team *team);

#endif // TEAM_H