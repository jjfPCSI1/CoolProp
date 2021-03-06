.. _MATLAB:

**************
MATLAB Wrapper
**************

Pre-compiled Binaries
=====================
Pre-compiled binaries of the MATLAB wrapper can be downloaded from :sfdownloads:`MATLAB` for your architecture, or from the development buildbot server at :bbbinaries:`MATLAB`.

Download the +CoolProp.7z file and extract it to a folder called +CoolProp using the `7-zip <http://www.7-zip.org/download.html>`_ program.  Place the mex file that is appropriate to your architecture in the directory that contains the directory +CoolProp.

* 32-bit windows: CoolPropMATLAB_wrap.mexw32
* 64-bit windows: CoolPropMATLAB_wrap.mexw64
* 64-bit linux: CoolPropMATLAB_wrap.mexa64
* 64-bit OSX: CoolPropMATLAB_wrap.mexmaci64

You should have a folder layout like::

    main
     |- CoolPropMATLAB_wrap.mexw64
     |- +CoolProp
        |- AbstractState.m
        |- DmassHmass_INPUTS.m
        |- ...
    
Then you need to add the main that contains the +CoolProp folder to the path in MATLAB

Example: adding the folder main that contains CoolProp files to the MATLAB path::

    addpath('/home/USERNAME/Some_folder/main')

Usage
-----

To calculate the normal boiling temperature of water at one atmosphere::

    >> CoolProp.PropsSI('T','P',101325,'Q',0,'Water')

User-Compiled Binaries
======================

.. _swig_matlab:

Swig+Matlab
-----------

As of version 5 of CoolProp, the MATLAB wrapper has been greatly improved in capabilities thanks to the work of the SWIG team.  The MATLAB wrapper will now automatically include complete wrappers of the high-level and low-level code, bringing it in line with the other SWIG-based wrappers (scilab, C#, octave, etc).

The bad part of this is that swig+matlab support has not been integrated into the main codebase of swig (as of October 2014).  Thus it is necessary to compile (or obtain) swig+matlab.

The code below assumes that the swig-matlab-bin folder sits in the build directory.

Precompiled
^^^^^^^^^^^
You can download the pre-compiled versions from `the buildbot slave <http://www.coolprop.dreamhosters.com:8010/nightly/>`_

Do-it-yourself (masochistic)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
On linux and OSX, you can simply run the script dev/scripts/build_swig_matlab.py which will check out the code for swig, obtain PCRE, and build the correct architecture's binary.  The resulting code will reside in the folder dev/scripts/swig-matlab/swig-matlab-bin.

For windows, its pretty much the same story, except that swig has to be cross-compiled from a linux (debian-based OS are good) host.  Install all the mingw packages in Synaptic, and then run the build script like::

    python build_swig_matlab.py --windows
    
which should hopefully yield the binaries in dev/scripts/swig-matlab/swig-matlab-bin.

Common Requirements
-------------------
Compilation of the MATLAB wrapper requires a few :ref:`common wrapper pre-requisites <wrapper_common_prereqs>`
    
Linux
-----

Install MATLAB using installer downloaded from www.mathworks.com.  As of version R2014a, only 64-bit MATLAB is available

OSX
---

Install MATLAB using installer downloaded from www.mathworks.com.  As of version R2014a, only 64-bit MATLAB is available

Windows
-------

Install MATLAB using installer downloaded from www.mathworks.com.  As of version R2014a, both of 32-bit and 64-bit MATLAB are available

Build
-----

Linux and OSX
^^^^^^^^^^^^^

Once the dependencies are installed, you can run the builder and tests using::

    # Check out the sources for CoolProp
    git clone https://github.com/CoolProp/CoolProp --recursive
    # Move into the folder you just created
    cd CoolProp
    # Make a build folder
    mkdir build && cd build
    # Copy the swig-matlab-bin folder here (fix path as necessary) (see above for Swig discussion)
    cp ../../dev/scripts/swig-matlab/swig-matlab-bin .
    # Set an environmental variable that points to your MATLAB installation for use in CMake (adjust if needed)
    export MATLAB_ROOT=/usr/local/MATLAB/R2014a # or /Applications/MATLAB_R2014a.app
    # Build the makefile using CMake with the path hacked to use our swig
    PATH=swig-matlab-bin/bin:%{PATH} cmake .. -DCOOLPROP_MATLAB_MODULE=ON -DSWIG_DIR=swig-matlab-bin/bin
    # Make the MEX files (by default files will be generated in folder install_root/MATLAB relative to CMakeLists.txt file)
    # Setting the SWIG_LIB explictly is dangerous, but for now it doesn't seem there is a better solution
    SWIG_LIB=swig-matlab-bin/share/swig/3.0.3 make install

Windows (32-bit and 64-bit)
^^^^^^^^^^^^^^^^^^^^^^^^^^^ 

You need to just slightly modify the building procedure::

    # Check out the sources for CoolProp
    git clone https://github.com/CoolProp/CoolProp --recursive
    # Move into the folder you just created
    cd CoolProp
    # Make a build folder
    mkdir build && cd build
    # Copy the swig-matlab-bin folder here (fix path as necessary) (see above for Swig discussion)
    cp ../../dev/scripts/swig-matlab/swig-matlab-bin .
    # Set an environmental variable that points to your MATLAB installation for use in CMake (adjust if needed)
    set "MATLAB_ROOT=c:\Program Files\MATLAB\R2014a"
    # Build the makefile using CMake with the path hacked to use our swig
    set "PATH=swig-matlab-bin\bin:%{PATH}" && cmake .. -DCOOLPROP_MATLAB_MODULE=ON -DSWIG_DIR=swig-matlab-bin\bin
    # Make the MEX files (by default files will be generated in folder install_root/MATLAB relative to CMakeLists.txt file)
    # Setting the SWIG_LIB explictly is dangerous, but for now it doesn't seem there is a better solution
    set "SWIG_LIB=swig-matlab-bin\share\swig\3.0.3" && make install