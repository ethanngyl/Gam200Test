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