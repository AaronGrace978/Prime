# prime_synth: V2 software synthesizer + .v2m module player
# Derived from Farbrausch V2 (BSD / Artistic License 2.0, see licenses/).

add_library(prime_synth STATIC
  synth_core.cpp
  ronan.cpp
  v2mplayer.cpp
  v2mconv.cpp
  sounddef.cpp
  types.h
  synth.h
  libv2.h
  ronan.h
  phonemtab.h
  v2mplayer.h
  v2mconv.h
  sounddef.h)

# RONAN enables the speech synthesizer ("Ronan"), used by several
# original Farbrausch tunes.
target_compile_definitions(prime_synth PRIVATE RONAN)

target_include_directories(prime_synth PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
