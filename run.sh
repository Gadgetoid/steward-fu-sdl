#!/bin/bash
export MOD=mmiyoo

if [ ! -z "$1" ] && [ "$1" == "mmiyoo" ]; then
export CROSS=/opt/mmiyoo/bin/arm-linux-gnueabihf-
export CC=${CROSS}gcc
export AR=${CROSS}ar
export AS=${CROSS}as
export LD=${CROSS}ld
export CXX=${CROSS}g++
export HOST=arm-linux
echo "Building SDL2 for MMiyoo handheld"
fi

./autogen.sh
./configure --disable-joystick-virtual --disable-sensor --disable-power --disable-alsa --disable-diskaudio --disable-video-x11 --disable-video-wayland --disable-video-kmsdrm --disable-video-vulkan --disable-dbus --disable-ime --disable-fcitx --disable-hidapi --disable-pulseaudio --disable-sndio --disable-libudev --disable-jack --disable-video-opengl --disable-video-opengles --disable-video-opengles2 --disable-oss --disable-dummyaudio --disable-video-dummy --host=${HOST}
make clean
make -j4
