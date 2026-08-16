// minimp3 is header only and the shared Mp3Decoder.cpp is the one translation
// unit that instantiates it, which every target has to add to its own build.
// PlatformIO only compiles what lives under src/, and pointing its source
// filter outside that leaves object files in the shared tree, so the file is
// pulled in from here instead.
#include "../../../../Src/Src/Decoders/Mp3Decoder.cpp"
