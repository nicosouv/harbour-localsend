#!/usr/bin/env bash
# Compiles every C++ file in src/ far enough to catch syntax and signature
# errors, without the Sailfish SDK.
#
# The unit tests link only the pieces that run headless, so the networking
# classes would otherwise never see a compiler until CI cross-builds them.
# This is a gate, not a build: no linking, and the objects are thrown away.
set -u

CFLAGS=$(pkg-config --cflags Qt5Core Qt5Gui Qt5Qml Qt5Quick Qt5Network openssl)

status=0
checked=0

for file in src/*.cpp; do
    # Needs sailfishapp.h, which exists only inside the Sailfish SDK
    if [ "$file" = "src/harbour-localsend.cpp" ]; then
        continue
    fi

    if g++ -fsyntax-only -std=c++11 -fPIC $CFLAGS -Isrc \
           -DAPP_VERSION='"0.0.0"' "$file"; then
        checked=$((checked + 1))
    else
        echo "FAILED: $file"
        status=1
    fi
done

if [ $status -eq 0 ]; then
    echo "syntax check passed on $checked files"
fi

exit $status
