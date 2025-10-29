/*!************************************************************************
\file      Grid.cpp
\author
\par DP email:
\par Course:
\par Assignment:
\date     2025-10-22
\brief
  Implementation of GridSystem and GridAPI for a 2D tile grid.
*************************************************************************/
#include "Precompiled.h"
#include "Grid.h"


namespace Framework {

	static Grid grid;
	Grid& GetGrid() {
		return grid;
	}

} // namespace Framework

