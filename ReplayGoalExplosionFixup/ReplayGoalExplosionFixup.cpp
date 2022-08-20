#include "pch.h"
#include "ReplayGoalExplosionFixup.h"


BAKKESMOD_PLUGIN(ReplayGoalExplosionFixup, "Fixes missing goal explosions in replays", plugin_version, PLUGINTYPE_REPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

struct ExplodeParams
{
	void* goal_ptr = nullptr;
	Vector location;
	void* scorer_pri_ptr = nullptr;
};

void ReplayGoalExplosionFixup::onLoad()
{
	_globalCvarManager = cvarManager;

	gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.Ball_TA.Explode", [this](auto caller, void* void_params, ...)
	{
		if (!gameWrapper->IsInReplay())
		{
			return;
		}
		const auto params = static_cast<ExplodeParams*>(void_params);
		if (params->goal_ptr != nullptr)
		{
			return;
		}
		const auto closest_goal = GetClosestGoal(params->location);
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
	const auto closest_goal = std::min_element(
		goals.begin(),
		goals.end(),
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
