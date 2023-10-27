#!/bin/bash
export MOD=mmiyoo
export CC=${CROSS_COMPILE}gcc
export AR=${CROSS_COMPILE}ar
export AS=${CROSS_COMPILE}as
export LD=${CROSS_COMPILE}ld
export CXX=${CROSS_COMPILE}g++
export HOST=arm-linux

echo "Building SDL2 for Miyoo Mini / Miyoo Mini Plus"
if [ ! -z "$1" ] && [ "$1" == "config" ]; then
    ./autogen.sh
    ./configure --disable-joystick-virtual --disable-sensor --disable-power --disable-alsa --disable-diskaudio --disable-video-x11 --disable-video-wayland --disable-video-kmsdrm --disable-video-vulkan --disable-dbus --disable-ime --disable-fcitx --disable-hidapi --disable-pulseaudio --disable-sndio --disable-libudev --disable-jack --disable-video-opengl --disable-video-opengles --disable-video-opengles2 --disable-oss --disable-dummyaudio --disable-video-dummy --host=${HOST}
fi

make -j4 V=99
