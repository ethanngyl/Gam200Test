#pragma once
#include "Precompiled.h"

namespace Framework {

	enum class TurnPhase : uint8_t {Player, Enemy};

	struct TurnState {
		TurnPhase phase = TurnPhase::Player;
		bool busy = false; //set true if animate a step
		uint64_t turnIndex = 0; //increment when phase flips
	};

	inline TurnState& Turn() {
		static TurnState s{};
		return s;
	}

	inline bool IsPlayerTurn() {
		return Turn().phase == TurnPhase::Player && !Turn().busy;
	}

	inline bool IsEnemyTurn() {
		return Turn().phase == TurnPhase::Enemy && !Turn().busy;
	}

	inline void EndPlayerTurn() {
		auto& t = Turn();
		if (t.phase != TurnPhase::Player) {
			std::cout << "[Turn ERROR] Tried to end player turn during enemy phase!\n";
			return;  // Prevent invalid state
		}
		t.phase = TurnPhase::Enemy;
		++t.turnIndex;
		std::cout << "[Turn] Player turn ended. Turn #" << t.turnIndex << " - Enemy phase.\n";
	}

	inline void EndEnemyTurn() {
		auto& t = Turn();
		if (t.phase != TurnPhase::Enemy) {
			std::cout << "[Turn ERROR] Tried to end enemy turn during player phase!\n";
			return;
		}
		t.phase = TurnPhase::Player;
		++t.turnIndex;
		std::cout << "[Turn] Enemy turn ended. Turn #" << t.turnIndex << " - Player phase.\n";
	}

}