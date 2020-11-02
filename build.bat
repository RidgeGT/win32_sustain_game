@echo off
pushd build
cl -W3 -DSUSTAIN_INTERNAL=1 -DSUSTAIN_SNAIL=1 -DSUSTAIN_WIN32=1 -FC -Zi W:\bin\Win32_Sustain.cpp user32.lib gdi32.lib
popd