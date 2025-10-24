#pragma once

#include "ECSEntity.h"
#include "ECSComponent.h"
#include "Component.h"

namespace Framework {

	struct GridTile : public Component<GridTile> {
		int x{ 0 };
		int y{ 0 };
		Entity occupant{ INVALID_ENTITY };
	};

}
