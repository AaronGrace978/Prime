# prime_texture: procedural texture generator
# Derived from Farbrausch OpenKTG (public domain / BSD, see licenses/).

add_library(prime_texture STATIC
  gentexture.cpp
  recipe.cpp
  graph.cpp
  gentexture.hpp
  recipe.hpp
  graph.hpp
  types.hpp)

target_include_directories(prime_texture PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
