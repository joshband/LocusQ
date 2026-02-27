/**
 * SofaHrtfLoader.cpp
 *
 * Isolated compilation unit for SOFA HRTF loading via libmysofa.
 *
 * This file is the ONLY translation unit that includes <mysofa.h>.
 * Keeping libmysofa out of all other TUs prevents C symbol pollution
 * in JUCE's header compilation units and avoids PCH / unity-build clashes.
 *
 * All callable surface is declared in SofaHrtfLoader.h.
 * This .cpp exists solely so the #include of <mysofa.h> is isolated.
 */

// Include the header that contains all inline / template implementations.
// Because SofaHrtfLoader.h is the primary public interface, and all functions
// are inline in that header, this .cpp provides the isolated TU that ensures
// mysofa.h is compiled exactly once within the LocusQ target when
// LOCUSQ_ENABLE_SOFA=ON.
//
// If non-inline helpers are added in future, implement them here.

#include "SofaHrtfLoader.h"

// Explicit instantiation anchor — forces the linker to pull this TU in even
// though all current symbols are inline.  Remove if concrete symbols are added.
namespace locusq::dsp
{
    // Ensure the translation unit is non-empty.
    static constexpr int kSofaHrtfLoaderVersion = 1;
} // namespace locusq::dsp
