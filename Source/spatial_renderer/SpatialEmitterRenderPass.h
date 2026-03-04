#pragma once

#include <array>
#include <limits>

namespace locusq::spatial_emitter_render_pass
{

template <typename CandidateType, std::size_t MaxRenderEmitters>
inline void refreshMinPriority (
    const std::array<CandidateType, MaxRenderEmitters>& selectedEmitters,
    int selectedEmitterCount,
    int& selectedMinPriorityIndex,
    float& selectedMinPriority) noexcept
{
    selectedMinPriorityIndex = -1;
    selectedMinPriority = std::numeric_limits<float>::max();

    for (int i = 0; i < selectedEmitterCount; ++i)
    {
        if (selectedEmitters[static_cast<std::size_t> (i)].priority < selectedMinPriority)
        {
            selectedMinPriority = selectedEmitters[static_cast<std::size_t> (i)].priority;
            selectedMinPriorityIndex = i;
        }
    }
}

template <typename CandidateType, std::size_t MaxRenderEmitters>
inline void insertCandidateWithBudget (
    std::array<CandidateType, MaxRenderEmitters>& selectedEmitters,
    int& selectedEmitterCount,
    int& selectedMinPriorityIndex,
    float& selectedMinPriority,
    const CandidateType& candidate,
    int& budgetCulledEmitterCount) noexcept
{
    if (selectedEmitterCount < static_cast<int> (MaxRenderEmitters))
    {
        selectedEmitters[static_cast<std::size_t> (selectedEmitterCount)] = candidate;
        ++selectedEmitterCount;
        refreshMinPriority (
            selectedEmitters,
            selectedEmitterCount,
            selectedMinPriorityIndex,
            selectedMinPriority);
        return;
    }

    if (candidate.priority <= selectedMinPriority)
    {
        ++budgetCulledEmitterCount;
        return;
    }

    if (selectedMinPriorityIndex >= 0)
    {
        selectedEmitters[static_cast<std::size_t> (selectedMinPriorityIndex)] = candidate;
        ++budgetCulledEmitterCount;
        refreshMinPriority (
            selectedEmitters,
            selectedEmitterCount,
            selectedMinPriorityIndex,
            selectedMinPriority);
    }
}

template <typename CandidateType, std::size_t MaxRenderEmitters>
inline void sortSelectedBySlotIndex (
    std::array<CandidateType, MaxRenderEmitters>& selectedEmitters,
    int selectedEmitterCount) noexcept
{
    for (int i = 1; i < selectedEmitterCount; ++i)
    {
        auto current = selectedEmitters[static_cast<std::size_t> (i)];
        int j = i - 1;

        while (j >= 0 && selectedEmitters[static_cast<std::size_t> (j)].slotIdx > current.slotIdx)
        {
            selectedEmitters[static_cast<std::size_t> (j + 1)] = selectedEmitters[static_cast<std::size_t> (j)];
            --j;
        }

        selectedEmitters[static_cast<std::size_t> (j + 1)] = current;
    }
}

} // namespace locusq::spatial_emitter_render_pass
