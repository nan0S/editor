#!/bin/sh

mkdir -p build

COMMON_COMPILER_FLAGS='-I./code -I./code/third_party/sfml/include -I./code/third_party/imgui -Wall -Wextra -Wno-unused-internal -Wno-unused-variable -Wno-unused-parameter'
COMMON_DEBUG_COMPILER_FLAGS="-O0 -DBUILD_DEBUG=1 -ggdb -fno-omit-frame-pointer $COMMON_COMPILER_FLAGS"
COMMON_RELEASE_COMPILER_FLAGS="-O3 -DBUILD_DEBUG=0 $COMMON_COMPILER_FLAGS"
COMMON_LINKER_FLAGS='-L./code/third_party/sfml/lib -Wl,-rpath,''$ORIGIN/../code/third_party/sfml/lib'' -lsfml-graphics -lsfml-window -lsfml-system -lGL'

# release build
if [ "$1" = "release" ]; then

   if ! [ -e ./build/imgui_unity_release.o ]; then
      echo -n "imgui release build: "
      /bin/time -f"%Us" \
         g++ $COMMON_RELEASE_COMPILER_FLAGS -c ./code/imgui_unity.cpp -o./build/imgui_unity_release.o
   fi

   echo -n "editor release build: "
   /bin/time -f"%Us" \
      g++ $COMMON_RELEASE_COMPILER_FLAGS ./code/editor.cpp ./build/imgui_unity_release.o $COMMON_LINKER_FLAGS -o./build/editor_release

# debug build
else

   if ! [ -e ./build/imgui_unity.o ]; then
      echo -n "imgui debug build: "
      /bin/time -f"%Us" \
         g++ $COMMON_DEBUG_COMPILER_FLAGS -c ./code/imgui_unity.cpp -o./build/imgui_unity.o
   fi

   echo -n "editor debug build: "
   /bin/time -f"%Us" \
      g++ $COMMON_DEBUG_COMPILER_FLAGS ./code/editor.cpp ./build/imgui_unity.o $COMMON_LINKER_FLAGS -o./build/editor
fi
