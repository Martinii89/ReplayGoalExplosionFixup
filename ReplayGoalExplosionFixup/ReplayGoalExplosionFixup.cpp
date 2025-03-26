#include "pch.h"
#include "ReplayGoalExplosionFixup.h"

#include <algorithm>


BAKKESMOD_PLUGIN(ReplayGoalExplosionFixup, "Fixes missing goal explosions in replays", plugin_version, PLUGINTYPE_REPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

struct ExplodeParams
{
	void* goal_ptr = nullptr;
	Vector location;
	void* scorer_pri_ptr = nullptr;
};

namespace 
{
	
bool close_to_zero(const Vector& vec)
{
	return std::abs(vec.X) < 1e-6 && std::abs(vec.Y) < 1e-6 && std::abs(vec.Z) < 1e-6;
}


bool GoalWrapperLikelyInvalid(GoalWrapper& goal)
{
	int has_no_orientation = goal.GetGoalOrientation().IsNull();
	int location_is_zero = close_to_zero(goal.GetLocation());
	int extent_is_zero = close_to_zero(goal.GetLocalExtent());
	return (has_no_orientation + location_is_zero + extent_is_zero) > 2;
}

}
void ReplayGoalExplosionFixup::onLoad()
{
	_globalCvarManager = cvarManager;

	gameWrapper->HookEventWithCaller<BallWrapper>("Function TAGame.Ball_TA.Explode", [this](auto caller, void* void_params, ...)
	{
		// LOG("on explode");
		if (!gameWrapper->IsInReplay())
		{
			// LOG("not in replay");
			return;
		}
		const auto params = static_cast<ExplodeParams*>(void_params);
		auto goal = GoalWrapper{reinterpret_cast<std::uintptr_t>(params->goal_ptr)};
		// LOG("Ball location: x:{}, y:{}, z:{}", caller.GetLocation().X, caller.GetLocation().Y, caller.GetLocation().Z);
		// LOG("GoalScoreing location: x:{}, y:{}, z:{}", params->location.X, params->location.Y, params->location.Z);
		if (goal && !GoalWrapperLikelyInvalid(goal))
		{
			// LOG("{}", goal.GetGoalOrientation().memory_address);
			// LOG("Goal location: x:{}, y:{}, z:{}", goal.GetLocation().X, goal.GetLocation().Y, goal.GetLocation().Z);
			// LOG("goal not null");
			return;
		}
		const auto closest_goal = GetClosestGoal(params->location);
		LOG("override goal");
		params->goal_ptr = reinterpret_cast<void*>(closest_goal.memory_address);
	});
}

void ReplayGoalExplosionFixup::onUnload()
{
}

template <typename T>
std::vector<T> ToVector(ArrayWrapper<T> arr)
{
	std::vector<T> res;
	if (arr.IsNull())
	{
		return res;
	}
	for (auto element : arr)
	{
		res.push_back(element);
	}
	return res;
}

GoalWrapper ReplayGoalExplosionFixup::GetClosestGoal(Vector location) const
{
	auto server = gameWrapper->GetGameEventAsReplay();
	if (!server)
	{
		return {0};
	}
	auto goals = ToVector(server.GetGoals());
	// LOG("goal count: {}", goals.size());
	// for (auto goal : goals)
	// {
	// 	LOG("{}", goal.GetGoalOrientation().memory_address);
	// }
	const auto closest_goal = std::ranges::min_element(
		goals,
		[location](GoalWrapper& a, GoalWrapper& b)
		{
			const auto dist_a = (location - a.GetLocation()).magnitude();
			const auto dist_b = (location - b.GetLocation()).magnitude();
			return dist_a < dist_b;
		});
	if (closest_goal == goals.end())
	{
		return {0};
	}
	return *closest_goal;
}
