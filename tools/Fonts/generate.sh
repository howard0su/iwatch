#!/bin/sh
../grlib/ftrasterize/ftrasterize.exe -f Red -s 13  "C&C Red Alert [LAN].ttf"
../grlib/ftrasterize/ftrasterize.exe -f Baby -s 16  babyblue.ttf
../grlib/ftrasterize/ftrasterize.exe -f Baby -s 12  babyblue.ttf
../grlib/ftrasterize/ftrasterize.exe -f Nova -s 13  ProximaNova-Regular.otf
../grlib/ftrasterize/ftrasterize.exe -f Nova -s 16  ProximaNova-Regular.otf
../grlib/ftrasterize/ftrasterize.exe -f Nova -s 38  ProximaNova-Regular.otf
../grlib/ftrasterize/ftrasterize.exe -f Nova -s 12 -b  ProximaNova-Bold.otf
../grlib/ftrasterize/ftrasterize.exe -f Nova -s 16 -b  ProximaNova-Bold.otf
../grlib/ftrasterize/ftrasterize.exe -f Nova -s 38 -b -v ProximaNova-Bold.otf
../grlib/ftrasterize/ftrasterize.exe -f Nova -s 50 -b  ProximaNova-Bold.otf
../grlib/ftrasterize/ftrasterize.exe -f Icon -s 16 -v icons_16x16_all.pbm
../grlib/ftrasterize/ftrasterize.exe -f Icon -s 48 -v icons_48x48_all.pbm

