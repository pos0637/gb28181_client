#!/bin/bash

WORKSPACE=`pwd`
cd $WORKSPACE/3rd_party

VERSION=4.1.0

export NDK_ROOT=/home/alex/Android/Sdk/ndk/23.1.7779620
export PREBUILT=$NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64
export PLATFORM=$PREBUILT/sysroot

export TARGET=aarch64-linux-android
# export TARGET=armv7a-linux-androideabi
# export TARGET=i686-linux-android
# export TARGET=x86_64-linux-android
export API=31

export AR=$PREBUILT/bin/llvm-ar
export CC=$PREBUILT/bin/$TARGET$API-clang
export AS=$CC
export CXX=$PREBUILT/bin/$TARGET$API-clang++
export LD=$PREBUILT/bin/ld
export RANLIB=$PREBUILT/bin/llvm-ranlib
export STRIP=$PREBUILT/bin/llvm-strip

export CFLAGS="--sysroot=$PLATFORM -I$PLATFORM/usr/include -fPIC -pthread"
export LDFLAGS="-Wl,-rpath-link=$PLATFORM/usr/lib -L$PLATFORM/usr/lib"

# libosip2
rm -rf libosip2-$VERSION
tar xvf libosip2-$VERSION.tar.gz
cd libosip2-$VERSION

./configure --prefix=$WORKSPACE --host=$TARGET --disable-shared --enable-static
make install

cd $WORKSPACE/3rd_party
rm -rf libosip2-$VERSION

# libexosip2
tar xvf libeXosip2-$VERSION.tar.gz
cd libeXosip2-$VERSION

./configure --prefix=$WORKSPACE --host=$TARGET --disable-shared --enable-static LDFLAGS="-L$WORKSPACE/lib" CFLAGS="-L$WORKSPACE/include"
make install

cd $WORKSPACE/3rd_party
rm -rf libeXosip2-$VERSION

# rm unused files
cd $WORKSPACE

rm -rf share
rm -rf lib/pkgconfig
rm lib/*.dylib || true
rm lib/*.so* || true
rm lib/*.la || true