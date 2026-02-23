# CMake generated Testfile for 
# Source directory: /Users/artbox/Documents/Repos/audio-dsp-qa-harness
# Build directory: /Users/artbox/Documents/Repos/LocusQ/build_bl010/qa_harness
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(VT01 "/Users/artbox/Documents/Repos/LocusQ/build_bl010/qa_harness/vt01_vocal_default")
set_tests_properties(VT01 PROPERTIES  SKIP_RETURN_CODE "77" _BACKTRACE_TRIPLES "/Users/artbox/Documents/Repos/audio-dsp-qa-harness/CMakeLists.txt;469;add_test;/Users/artbox/Documents/Repos/audio-dsp-qa-harness/CMakeLists.txt;0;")
subdirs("qa_state")
subdirs("../_deps/yaml-cpp-build")
subdirs("external/saf")
