Team Name: Struct Squad
Team Roster:
SIM KAH YAN (Graphic Design+Audio)
TAN WEI LEONG (Programmer)
ETHAN NG (Tech Lead)
JOSH ONG (Programmer)
GE YONG QI (Programmer (DEBUG))
ZHOU JIAHAO (Collision+physics)
CARL JAMESON Z PADILLA (Programmer)
GERARD LOU (Design lead + Story Champion)
IVAN NG (Product Manager + Programmer)

Game Concept:
The primary cycle of play consists of:
Movement – Players spend actions to move across the grid.
One space per action, positioning strategically toward objectives or enemies.
Combat – When encountering another class or enemy, combat is resolved in a turn-based sequence, with attack order determined by Speed or passives.
Interaction – Players may interact with the objectives/items

Win Con:
Defeat all the enemies, collect all the items in the chests around the map and bring them to the main objective

Lose Con:
When the player Health is reduced to 0

In-Engine Demo:
The current demo of the full game is not completed at the moment. An implementation of the grid and path finding AI for the enemy to track the player
is found in level 3 of the current game engine. Arrow keys can be used to control the player on the grid. Each time the player moves to a grid the
enemy will move towards it accordingly. The shortest path is always recalculated after every move/turn.

Current Levels:
The game loads first into the main menu, with a play and exit button. 
Main Menu: Clicking on play will load up level one, clicking exit will close the application
Level 1: Currently contains an entity with an animated bird sprite rendered. The entity's animation is only active while 
the player is moving via the WASD keys. The player can hit space bar to fire a projectile upwards. There is an enemy
on that level to allow testing for the current subscriber listener system which enables the enemy to take damage upon
being hit by a projectile. Keys 3 and 4 will allow for upscaling and downscaling of the player entity respectively. 
Keys 7 and 8 will allow the player to rotate the entity towards the left and right respectively. Key 5 will send the player
back to the main menu and Key 6 loads up level 2.
Level 2: Level 2 is a sandbox/level editor level. Functionalities of the ImGui can be tested here. More information on the level
editor can be found in the LevelEditor document. Key 3 on this level will bring the player to level 3. Level 3 can only be accessed
via hitting key 3 in level 2.
Level 3: Level 3 is a demonstration of the current grid and pathfinding system that will form the foundation of the game. 