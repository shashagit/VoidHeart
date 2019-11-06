#pragma once
#include "System.h"

namespace Hollow
{
	class Animator;
	
	class AnimationSystem : public System
	{
		REGISTERSYSTEM(AnimationSystem, 3);
	public:
		virtual ~AnimationSystem() {}
		void Update();
		void AddGameObject(GameObject* object);

	private:
		void Animate(unsigned int start, unsigned int end);
		void CreateSkeleton(Animator* animator);
		template<typename T>
		inline std::pair<unsigned int, unsigned int> BinarySearch(unsigned int start, unsigned int end, double time, const std::vector<std::pair<double, T>>& list)
		{
			unsigned int mid = start + (end - start)/2;
			if (time == list[mid].first || mid == list.size() - 1 || mid == 0)
			{
				return std::make_pair(mid, -1);
			}
			else if (time > list[mid].first && time < list[mid + 1].first)
			{
				return std::make_pair(mid, mid + 1);
			}
			else if (time > list[mid - 1].first && time < list[mid].first)
			{
				return std::make_pair(mid - 1, mid);
			}
			else if (time < list[mid].first)
			{
				return BinarySearch(start, mid - 1, time, list);
			}
			else if (time > list[mid].first)
			{
				return BinarySearch(mid + 1, end, time, list);
			}
		}

		template<typename T>
		inline std::pair<unsigned int,unsigned int> FindT2inList(double time, const std::vector<std::pair<double, T>>& list)
		{
			return BinarySearch(0, list.size() - 1, time, list);
		}
	};
}