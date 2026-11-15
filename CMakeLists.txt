# Electronique : calculateurs d'electronique pour Casio Graph 90+E (fxSDK / gint).
# Compilation : fxsdk build-cg   ->   Electronique.g3a
cmake_minimum_required(VERSION 3.15)
project(Electronique VERSION 2.0 LANGUAGES C CXX)

include(GenerateG3A)
include(Fxconv)
find_package(Gint 2.9 REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Toutes les sources de src/ (calculateurs dans src/calc/, fiches dans src/content/).
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(electronique ${SOURCES})
# Pas d'exceptions, pas de RTTI, pas de garde sur les statiques locales :
# l'application n'utilise pas la bibliotheque C++ (ni new/delete, ni std::),
# ni libm (fonctions mathematiques maison dans src/num/FMath.cpp).
target_compile_options(electronique PRIVATE
    -Wall -Wextra -Wno-missing-field-initializers -O2
    -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit)
target_link_libraries(electronique Gint::Gint)

if("${FXSDK_PLATFORM_LONG}" STREQUAL fxCG50)
  generate_g3a(TARGET electronique OUTPUT "Electronique.g3a"
    NAME "Electronique" ICONS assets-cg/icon-uns.png assets-cg/icon-sel.png)
endif()
