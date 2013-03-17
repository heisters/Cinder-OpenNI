OpenNI Cinder Block
===================

A block for using OpenNI with Cinder. Based on OpenNI 2, NITE 2, and libfreenect.

https://github.com/piedar/OpenNI2-FreenectDriver
https://github.com/OpenKinect/libfreenect

Installation
------------

Install latest libusb, compiled for i386 and x86\_64.

    brew install --universal libusb

If you're using OpenNI2-Freenect, install libfreenect from github.

    git clone git://github.com/OpenKinect/libfreenect.git
    cd libfreenect
    mkdir build
    cd build
    CMAKE_OSX_ARCHITECTURES='i386;x86_64' cmake ..
    make && make install

Compiling OpenNI2
-----------------

This is unnecessary because the block contains static libraries for OpenNI.
However, if you need to rebuild them for whatever reason:

    git clone git://github.com/OpenNI/OpenNI2.git
    # or, for libfreenect support:
    git clone git://github.com/piedar/OpenNI2-FreenectDriver.git OpenNI2
    cd OpenNI2

Modify the core makefile to compile OpenNI2 as a static libary by replacing
+LIB\_NAME+ with +SLIB\_NAME+:

    vim Source/Core/Makefile # :%s/LIB_NAME/SLIB_NAME

Build OpenNI2:

    make core

