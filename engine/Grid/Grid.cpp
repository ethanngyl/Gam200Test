/**
===============================================================================
 File:           Grid.cpp
 Author:         Josh Ong
 Email:          josh.o@digipen.edu
 Date:           2025-10-22
 Contribution:	 100%
 ------------------------------------------------------------------------------

  Brief:
  - Provides the implementation for the global Grid access function.

  Design notes:
  - The single Grid instance is declared as a static global variable within
	the Framework namespace to ensure a single, application-wide grid manager.
  - GetGrid() returns a reference to this static instance, allowing any system
	to interact with the grid properties defined in Grid.h.
===============================================================================
 */
#include "Precompiled.h"
#include "Grid.h"


namespace Framework {

	static Grid grid;
	Grid& GetGrid() {
		return grid;
	}

} // namespace Framework

