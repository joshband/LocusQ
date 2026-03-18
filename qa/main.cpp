// LocusQ QA Harness - main.cpp
//
// Thin process entrypoint for the shared LocusQ QA runner implementation.

#include "LocusQQARunner.h"

#include <juce_events/juce_events.h>

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    return runLocusQQA(argc, argv);
}
