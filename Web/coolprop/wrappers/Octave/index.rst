.. _Octave:

**************
Octave Wrapper
**************

Pre-compiled Binaries
=====================
Pre-compiled release binaries can be downloaded from :sfdownloads:`Octave`.  Development binaries coming from the buildbot server can be found at :bbbinaries:`Octave`.  Download the oct file appropriate to your system.

On Linux systems you can put the generated .oct file in
``/usr/share/octave/?octave.version.number?/m`` folder. You will need superuser
privileges to do this.

If you place .oct file somewhere outside octave path, you have to use
"addpath" function at begining of your code.

Example: adding the folder that contains CoolProp.oct file to the Octave path::

    addpath('/home/?user_name?/Some_folder/CoolProp')
    
User-Compiled Binaries
======================

Common Requirements
-------------------
Compilation of the Octave wrapper requires a few :ref:`common wrapper pre-requisites <wrapper_common_prereqs>`

Octave Requirements
-------------------
* SWIG
* Octave (and development headers)
    
Linux
-----

For ubuntu and friends, you can install build dependencies using::

    sudo apt-get install swig octave liboctave-dev

OSX
---
For OSX, your best best is a binary installer (see http://wiki.octave.org/Octave_for_MacOS_X), alternatively, you can install from Homebrew, though as of July 6, 2014, this functionality was broken in OSX 10.9.  If you use the installer, you might want to add the octave binary folder onto the path.  To do so, add to the file .profile (or create it) in your home directory::

    export PATH="/usr/local/octave/3.8.0/bin:$PATH"

Windows
-------
For windows, the situation is ok, but not great.  Only the MinGW builds are supported, and not comfortably

1. Download a MinGW build from `http://wiki.octave.org/Octave_for_Microsoft_Windows`_.

2. Extract the zip file to somewhere on your computer without any spaces in the path (c:\\octave-x.x.x is a good choice)

3. Rename the sh.exe in the bin folder of your installation to _sh.exe

Build
-----

Once the dependencies are installed, you can run the builder and tests using::

    # Check out the sources for CoolProp
    git clone https://github.com/CoolProp/CoolProp --recursive
    # Move into the folder you just created
    cd CoolProp
    # Make a build folder
    mkdir -p build &&  cd build
    # Build the makefile using CMake
    cmake .. -DCOOLPROP_OCTAVE_MODULE=ON -DBUILD_TESTING=ON
    # Make the OCT files (by default files will be generated in folder install_root/Octave relative to CMakeLists.txt file)
    make install
    # Run the integration tests
    ctest --extra-verbose

On windows, you need to just slightly modify the building procedure::

    # The folder containing the folders bin, mingw, include, etc. (or set a system variable in your windows installation)
    set OCTAVE_ROOT=c:\path\to\octave\root 
    # Add the bin folder of your octave install to the system path
    set PATH=c:\path\to\octave\bin;%PATH%
    # Check out the sources for CoolProp
    git clone https://github.com/CoolProp/CoolProp --recursive
    # Move into the folder you just created
    cd CoolProp
    # Make a build folder
    mkdir build/Octave
    # Move into that folder
    cd build/Octave
    # Build the makefile using CMake
    cmake .. -G "MinGW Makefiles" -DCOOLPROP_OCTAVE_MODULE=ON -DBUILD_TESTING=ON
    # Make the OCT files (by default files will be generated in folder install_root/Octave relative to CMakeLists.txt file)
    make install
    # Run the integration tests
    ctest --extra-verbose

