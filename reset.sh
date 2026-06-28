#!/bin/bash
MNT_PT="./tst"
EXE="./main"

echo "🧹 Clearing zombie locks..."
sudo umount -l "$MNT_PT" 2>/dev/null || true
pkill -9 -f "$EXE" 2>/dev/null || true

# Remake the target folder fresh
rm -rf "$MNT_PT"
mkdir -p "$MNT_PT"
chmod 777 "$MNT_PT"

echo "🔨 Recompiling main.cpp..."
g++ -Wall -std=c++20 main.cpp `pkg-config fuse3 --cflags --libs` -o main

if [ $? -eq 0 ]; then
    echo "🚀 Launching FUSE mount..."
    $EXE -f "$MNT_PT"
else
    echo "❌ Compilation failed!"
fi
