#include "team.h"

void initTeam(Team *team, int teamId) {
    team->id = teamId;
    team->totalEffort = 0;
    team->isWinner = false;
    
    for (int i = 0; i < PLAYERS_PER_TEAM; i++) {
        team->players[i].id = i;
        team->players[i].teamId = teamId;
        team->players[i].energy = rand() % (config.maxInitialEnergy - config.minInitialEnergy + 1) + config.minInitialEnergy;
        team->players[i].decreaseRate = rand() % (config.maxEnergyDecreaseRate - config.minEnergyDecreaseRate + 1) + config.minEnergyDecreaseRate;
        team->players[i].position = i;
        team->players[i].fallen = false;
    }
}

void sortPlayersByEnergy(Team *team) {
    // Simple bubble sort (for a small array it's fine)
    for (int i = 0; i < PLAYERS_PER_TEAM - 1; i++) {
        for (int j = 0; j < PLAYERS_PER_TEAM - i - 1; j++) {
            if (team->players[j].energy > team->players[j + 1].energy) {
                // Swap players
                Player temp = team->players[j];
                team->players[j] = team->players[j + 1];
                team->players[j + 1] = temp;
            }
        }
    }
    
    // Update positions after sorting
    for (int i = 0; i < PLAYERS_PER_TEAM; i++) {
        team->players[i].position = i;
    }
}

void updateTotalEffort(Team *team, PlayerMessage messages[PLAYERS_PER_TEAM]) {
    team->totalEffort = 0;
    for (int i = 0; i < PLAYERS_PER_TEAM; i++) {
        team->totalEffort += messages[i].energy;
    }
}

void createPlayers(Team *team) {
    for (int i = 0; i < PLAYERS_PER_TEAM; i++) {
        pid_t pid = fork();
        
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process (player)
            playerProcess(i, team->id);
            exit(EXIT_SUCCESS); // Should not reach here
        } else {
            // Parent process (referee)
            team->players[i].pid = pid;
        }
    }
}