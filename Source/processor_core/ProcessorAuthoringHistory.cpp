#include "../PluginProcessor.h"
#include "../shared_contracts/BridgeStatusContract.h"

namespace
{
namespace bridge_status = locusq::shared_contracts::bridge_status;
} // namespace

juce::var LocusQAudioProcessor::cloneJsonLikeVar (const juce::var& value)
{
    return juce::JSON::parse (juce::JSON::toString (value));
}

bool LocusQAudioProcessor::jsonLikeVarsEqual (const juce::var& lhs, const juce::var& rhs)
{
    return juce::JSON::toString (lhs) == juce::JSON::toString (rhs);
}

LocusQAudioProcessor::AuthoringFileState LocusQAudioProcessor::captureAuthoringFileState (const juce::File& file)
{
    AuthoringFileState state;
    state.path = file.getFullPathName();
    state.exists = file.existsAsFile();

    if (state.exists)
        state.contents = file.loadFileAsString();

    return state;
}

bool LocusQAudioProcessor::restoreAuthoringFileStates (const std::vector<AuthoringFileState>& fileStates)
{
    for (const auto& fileState : fileStates)
    {
        const juce::File file (fileState.path);

        if (! fileState.exists)
        {
            if (file.existsAsFile() && ! file.deleteFile())
                return false;

            continue;
        }

        const auto parentDirectory = file.getParentDirectory();
        if (! parentDirectory.exists() && ! parentDirectory.createDirectory())
            return false;

        if (! file.replaceWithText (fileState.contents))
            return false;
    }

    return true;
}

juce::var LocusQAudioProcessor::captureAuthoringStateSnapshotLocked() const
{
    juce::var stateVar (new juce::DynamicObject());
    auto* state = stateVar.getDynamicObject();

    state->setProperty ("parameters", captureEmitterParameterState());
    state->setProperty ("timeline", serialiseKeyframeTimelineLocked());
    state->setProperty ("uiState", getUIStateFromUI());
    return stateVar;
}

bool LocusQAudioProcessor::applyAuthoringStateSnapshotLocked (const juce::var& snapshot)
{
    auto* state = snapshot.getDynamicObject();
    if (state == nullptr)
        return false;

    if (state->hasProperty ("parameters")
        && ! applyEmitterParameterState (state->getProperty ("parameters")))
    {
        return false;
    }

    if (state->hasProperty ("timeline")
        && ! applyKeyframeTimelineLocked (state->getProperty ("timeline")))
    {
        return false;
    }

    if (state->hasProperty ("uiState"))
    {
        const auto uiState = state->getProperty ("uiState");
        if (uiState.getDynamicObject() == nullptr)
            return false;

        setUIStateFromUI (uiState);
    }

    return true;
}

void LocusQAudioProcessor::pushAuthoringHistoryEntry (AuthoringHistoryEntry entry)
{
    const juce::ScopedLock historyLock (authoringHistoryLock);

    authoringUndoHistory.push_back (std::move (entry));
    if (authoringUndoHistory.size() > kMaxAuthoringHistoryEntries)
        authoringUndoHistory.erase (authoringUndoHistory.begin());

    authoringRedoHistory.clear();
}

LocusQAudioProcessor::AuthoringHistorySelectionHint LocusQAudioProcessor::makeSelectionHint (
    const juce::String& preferredPresetPath,
    const juce::String& preferredPresetType)
{
    AuthoringHistorySelectionHint hint;
    hint.preferredPresetPath = preferredPresetPath;
    hint.preferredPresetType = preferredPresetType;
    return hint;
}

juce::var LocusQAudioProcessor::buildAuthoringHistoryStatusResponse (
    bool ok,
    const juce::String& label,
    const juce::String& message,
    const AuthoringHistorySelectionHint& selectionHint) const
{
    juce::var response (new juce::DynamicObject());
    auto* result = response.getDynamicObject();

    result->setProperty (bridge_status::kOk, ok);

    if (label.isNotEmpty())
        result->setProperty ("label", label);

    if (message.isNotEmpty())
        result->setProperty (bridge_status::kMessage, message);

    if (selectionHint.preferredPresetPath.isNotEmpty())
        result->setProperty ("preferredPresetPath", selectionHint.preferredPresetPath);

    if (selectionHint.preferredPresetType.isNotEmpty())
        result->setProperty ("preferredPresetType", selectionHint.preferredPresetType);

    const juce::ScopedLock historyLock (authoringHistoryLock);
    const auto canUndo = ! authoringUndoHistory.empty();
    const auto canRedo = ! authoringRedoHistory.empty();

    result->setProperty ("canUndo", canUndo);
    result->setProperty ("canRedo", canRedo);
    result->setProperty ("undoDepth", static_cast<int> (authoringUndoHistory.size()));
    result->setProperty ("redoDepth", static_cast<int> (authoringRedoHistory.size()));

    if (canUndo)
        result->setProperty ("undoLabel", authoringUndoHistory.back().label);
    if (canRedo)
        result->setProperty ("redoLabel", authoringRedoHistory.back().label);

    return response;
}

juce::var LocusQAudioProcessor::commitAuthoringHistoryEntry (
    const juce::String& actionId,
    const juce::String& label,
    const juce::var& beforeState,
    const juce::var& afterState,
    std::vector<AuthoringFileState> beforeFiles,
    std::vector<AuthoringFileState> afterFiles,
    const AuthoringHistorySelectionHint& beforeSelection,
    const AuthoringHistorySelectionHint& afterSelection)
{
    auto fileStatesEqual = [] (const std::vector<AuthoringFileState>& lhs,
                               const std::vector<AuthoringFileState>& rhs)
    {
        if (lhs.size() != rhs.size())
            return false;

        for (size_t index = 0; index < lhs.size(); ++index)
        {
            if (lhs[index].path != rhs[index].path
                || lhs[index].exists != rhs[index].exists
                || lhs[index].contents != rhs[index].contents)
            {
                return false;
            }
        }

        return true;
    };

    const auto stateChanged = ! jsonLikeVarsEqual (beforeState, afterState);
    const auto filesChanged = ! fileStatesEqual (beforeFiles, afterFiles);
    const auto selectionChanged = beforeSelection.preferredPresetPath != afterSelection.preferredPresetPath
        || beforeSelection.preferredPresetType != afterSelection.preferredPresetType;

    if (! stateChanged && ! filesChanged && ! selectionChanged)
        return buildAuthoringHistoryStatusResponse (true, label, {}, afterSelection);

    AuthoringHistoryEntry entry;
    entry.actionId = actionId;
    entry.label = label;
    entry.beforeState = cloneJsonLikeVar (beforeState);
    entry.afterState = cloneJsonLikeVar (afterState);
    entry.beforeFiles = std::move (beforeFiles);
    entry.afterFiles = std::move (afterFiles);
    entry.beforeSelection = beforeSelection;
    entry.afterSelection = afterSelection;

    pushAuthoringHistoryEntry (std::move (entry));
    return buildAuthoringHistoryStatusResponse (true, label, {}, afterSelection);
}

juce::var LocusQAudioProcessor::applyAuthoringHistoryEntryFromUI (bool redo)
{
    AuthoringHistoryEntry entry;

    {
        const juce::ScopedLock historyLock (authoringHistoryLock);
        auto& sourceHistory = redo ? authoringRedoHistory : authoringUndoHistory;

        if (sourceHistory.empty())
        {
            return buildAuthoringHistoryStatusResponse (false,
                                                        redo ? "Redo" : "Undo",
                                                        redo ? "Nothing to redo." : "Nothing to undo.",
                                                        {});
        }

        entry = sourceHistory.back();
        sourceHistory.pop_back();
    }

    const auto& targetFiles = redo ? entry.afterFiles : entry.beforeFiles;
    if (! restoreAuthoringFileStates (targetFiles))
    {
        const juce::ScopedLock historyLock (authoringHistoryLock);
        auto& sourceHistory = redo ? authoringRedoHistory : authoringUndoHistory;
        sourceHistory.push_back (entry);

        return buildAuthoringHistoryStatusResponse (false,
                                                    entry.label,
                                                    redo ? "Redo failed while restoring preset files."
                                                         : "Undo failed while restoring preset files.",
                                                    redo ? entry.afterSelection : entry.beforeSelection);
    }

    const auto& targetState = redo ? entry.afterState : entry.beforeState;
    {
        const juce::ScopedLock timelineLock (keyframeTimelineStateLock);
        if (! applyAuthoringStateSnapshotLocked (targetState))
        {
            const juce::ScopedLock historyLock (authoringHistoryLock);
            auto& sourceHistory = redo ? authoringRedoHistory : authoringUndoHistory;
            sourceHistory.push_back (entry);

            return buildAuthoringHistoryStatusResponse (false,
                                                        entry.label,
                                                        redo ? "Redo failed while restoring authoring state."
                                                             : "Undo failed while restoring authoring state.",
                                                        redo ? entry.afterSelection : entry.beforeSelection);
        }
    }

    {
        const juce::ScopedLock historyLock (authoringHistoryLock);
        auto& destinationHistory = redo ? authoringUndoHistory : authoringRedoHistory;
        destinationHistory.push_back (entry);

        if (destinationHistory.size() > kMaxAuthoringHistoryEntries)
            destinationHistory.erase (destinationHistory.begin());
    }

    return buildAuthoringHistoryStatusResponse (true,
                                                entry.label,
                                                redo ? "Redo applied." : "Undo applied.",
                                                redo ? entry.afterSelection : entry.beforeSelection);
}

juce::var LocusQAudioProcessor::getAuthoringHistoryStatusFromUI() const
{
    return buildAuthoringHistoryStatusResponse (true, {}, {}, {});
}

juce::var LocusQAudioProcessor::undoAuthoringActionFromUI()
{
    return applyAuthoringHistoryEntryFromUI (false);
}

juce::var LocusQAudioProcessor::redoAuthoringActionFromUI()
{
    return applyAuthoringHistoryEntryFromUI (true);
}

juce::var LocusQAudioProcessor::commitKeyframeTimelineFromUI (const juce::var& payload)
{
    const auto* payloadObject = payload.getDynamicObject();
    const auto timelineState = (payloadObject != nullptr && payloadObject->hasProperty ("timeline"))
        ? payloadObject->getProperty ("timeline")
        : payload;
    const auto uiState = (payloadObject != nullptr && payloadObject->hasProperty ("uiState"))
        ? payloadObject->getProperty ("uiState")
        : juce::var();
    const auto historyLabel = (payloadObject != nullptr && payloadObject->hasProperty ("historyLabel"))
        ? payloadObject->getProperty ("historyLabel").toString()
        : juce::String ("Edit Timeline");

    if (payloadObject != nullptr && payloadObject->hasProperty ("uiState") && uiState.getDynamicObject() == nullptr)
    {
        return buildAuthoringHistoryStatusResponse (false,
                                                    historyLabel,
                                                    "Timeline commit payload is missing a compatible UI state object.",
                                                    {});
    }

    juce::var beforeState;
    juce::var afterState;

    {
        const juce::ScopedLock timelineLock (keyframeTimelineStateLock);
        beforeState = captureAuthoringStateSnapshotLocked();

        if (! applyKeyframeTimelineLocked (timelineState))
        {
            return buildAuthoringHistoryStatusResponse (false,
                                                        historyLabel,
                                                        "Timeline payload is not compatible.",
                                                        {});
        }

        if (payloadObject != nullptr && payloadObject->hasProperty ("uiState"))
            setUIStateFromUI (uiState);

        afterState = captureAuthoringStateSnapshotLocked();
    }

    return commitAuthoringHistoryEntry ("timeline_edit",
                                        historyLabel,
                                        beforeState,
                                        afterState,
                                        {},
                                        {},
                                        {},
                                        {});
}
