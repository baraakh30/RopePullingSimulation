# Rope Pulling Game Simulation

This project implements a multi-processing application that simulates a rope pulling game between two teams using Linux process management, signals, and pipes with OpenGL visualization.

## Prerequisites

- Linux operating system
- GCC compiler
- Make build system
- OpenGL libraries
- GLUT (OpenGL Utility Toolkit)

## Installation

### Install dependencies (Ubuntu/Debian)

```bash
sudo apt-get update && sudo apt-get install -y gcc gdb build-essential freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev
```

### Install dependencies (Fedora/CentOS/RHEL)

```bash
sudo dnf install -y gcc make freeglut-devel mesa-libGL-devel mesa-libGLU-devel
```

### Install dependencies (Arch Linux)

```bash
sudo pacman -S --noconfirm base-devel freeglut mesa
```

## Building the Project

1. Clone or extract the project files into a directory
2. Navigate to the project directory
3. Compile the project using make:

```bash
make
```

This will create an executable called `rope_game`.

## Configuration

Before running the game, configure the simulation parameters in the `config.txt` file:

```
minInitialEnergy = 100
maxInitialEnergy = 200
minEnergyDecreaseRate = 1
maxEnergyDecreaseRate = 5
minRejoiningTime = 1
maxRejoiningTime = 3
winThreshold = 500
maxScore = 3
consecutiveWinsNeeded = 2
gameDuration = 60
waitBeforeWin = 5
```

Parameter descriptions:
- `minInitialEnergy` / `maxInitialEnergy`: Range for player starting energy
- `minEnergyDecreaseRate` / `maxEnergyDecreaseRate`: Range for energy decrease rate
- `minRejoiningTime` / `maxRejoiningTime`: Range for how long a fallen player takes to rejoin
- `winThreshold`: Effort difference needed to win a round
- `maxScore`: Maximum score to end the game
- `consecutiveWinsNeeded`: Number of consecutive round wins to end the game
- `gameDuration`: Maximum game duration in seconds
- `waitBeforeWin`: Seconds a team must maintain their lead to win a round

## Running the Simulation

Run the game with:

```bash
./rope_game config.txt
```

## How the Game Works

1. Each team has 4 players with random initial energy levels
2. Players position themselves along the rope based on energy levels
3. Players pull the rope and exert effort based on their energy and position
4. A team wins a round when their total effort exceeds the opponent's by the threshold
5. Players may randomly fall during the game and take time to rejoin
6. The game ends when:
   - A team reaches the maximum score
   - A team wins the required number of consecutive rounds
   - The game time expires

## Project Structure

- `main.c`: Entry point and process management
- `common.h`: Shared definitions and data structures
- `player.c/h`: Player process implementation
- `team.c/h`: Team management functions
- `referee.c/h`: Game coordination and rules
- `visualizer.c/h`: OpenGL visualization Process

## Troubleshooting

- If the game doesn't run, check if all OpenGL libraries are correctly installed
- Make sure all the necessary files are in the same directory
- If there are build errors, try running `make clean` followed by `make`

## Cleaning Up

To clean the project (remove object files and executable):

```bash
make clean
```
