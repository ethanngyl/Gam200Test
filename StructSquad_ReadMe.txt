STRUCT SQUAD GAME ENGINE

TEAM NAME:
Struct Squad

TEAM ROSTER:
IVAN NG ...................... Product Manager + Programmer
ETHAN NG ..................... Tech Lead
SIM KAH YAN .................. Graphic Design + Audio
TAN WEI LEONG ............... Programmer
JOSH ONG ..................... Programmer
GE YONG QI ................... Programmer (Debug)
ZHOU JIAHAO .................. Collision + Physics
CARL JAMESON Z. PADILLA ..... Programmer
GERARD LOU ................... Design Lead + Story Champion


GAME CONCEPT:
The core gameplay cycle is based around three main actions ?Movement, Combat, and Interaction.

Movement: Players move one grid space per action. Each move should be made strategically toward objectives or enemies.
Combat: When a player encounters an enemy, combat occurs in a turn-based manner. Attack order is determined by Speed or passive abilities.
Interaction: Players can interact with objects or items placed around the map.

WIN CONDITION:
Defeat all enemies, collect every item found in the map chests, and return them to the main objective.

LOSE CONDITION:
The player loses when their Health is reduced to 0.

IN-ENGINE DEMO:
The complete demo version of the game is still in progress.
Currently, Level 3 showcases the grid system and pathfinding AI that allows an enemy to track the player.
Players can move using the WASD. Each time the player moves, the enemy recalculates the shortest path using the A* algorithm and moves toward the player on its turn.

CURRENT LEVELS:

MAIN MENU:
The game starts at the main menu.
Selecting Play loads Level 1.
Selecting Quit Game closes the application.

LEVEL SELECTOR:
Displays buttons for Levels 1 to 3.
Selecting a level loads it.
A Back button allows returning to the main menu.

LEVEL 1:
This level contains an animated bird sprite entity.
The animation plays only while the player is moving using the WASD keys.
Pressing the space bar fires a projectile upward.
An enemy is present for testing the current subscriber-listener system, which allows the enemy to take damage from projectiles.
Keys 3 and 4 upscale and downscale the player entity.
Keys 7 and 8 rotate the player to the left and right.
Key 5 returns to the main menu.
Key 6 loads Level 2.

LEVEL 2:
This level functions as a sandbox and level editor for testing ImGui features.
Further information can be found in the LevelEditor document.
Pressing Key 3 loads Level 3.
Level 3 can only be accessed through Level 2.
Press ESC to pause the game. Use the WS keys, up and down arrow keys, or 1-3 keys to select, and use the space bar or Enter key to confirm the selection.

LEVEL 3:
This level demonstrates the core grid system, entity occupancy, and enemy pathfinding loop.
It verifies tile creation, world-to-tile conversion, occupancy tracking, arrow key movement, and enemy pursuit using the A* algorithm.
The player is locked to one tile per turn and cannot move out of turn.
Enemy movement is recalculated each turn for smarter pursuit behavior.
This level forms the foundation for the turn-based gameplay system the rest of the game will build upon.