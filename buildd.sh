#!/bin/sh

build="./build"
projectstate="$build/projectstate.json"

if [ ! -d "$build" ]; then
  echo "build directory does exist."
  echo "Creating build directory."
  mkdir build
fi

if [ ! -f "$projectstate" ]; then
  echo "{\"debug\": 0}" > "$projectstate"
fi

unset DEBUG

debug="$(jq '.debug' $projectstate)"

if [ ! "$debug" -eq "0" ]; then
  echo "Previously used a debug build"
  newjson="$(jq '.debug=0' "$projectstate")"
  echo "$newjson" > "$projectstate"

  cmake -DCMAKE_BUILD_TYPE="Debug" -B build 
fi

cmake --build build
./build/cellular
