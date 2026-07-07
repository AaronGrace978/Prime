add_executable(prime_smoke smoke.cpp smoke_texture.cpp smoke_synth.cpp)
target_link_libraries(prime_smoke PRIVATE prime_texture prime_synth)

add_test(NAME smoke
  COMMAND prime_smoke ${CMAKE_SOURCE_DIR}/assets/v2m/pzero_new.v2m)

if(PRIME_BUILD_TOOLS)
  add_test(NAME primetex_cli
    COMMAND primetex -o ${CMAKE_CURRENT_BINARY_DIR} -s 64)

  add_test(NAME primeplay_cli
    COMMAND primeplay ${CMAKE_SOURCE_DIR}/assets/v2m/v2_zeitmaschine_new.v2m
            -o ${CMAKE_CURRENT_BINARY_DIR}/zeitmaschine.wav -t 10)
endif()
