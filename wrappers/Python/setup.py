from __future__ import print_function

def copy_files():
    import shutil
    shutil.rmtree(os.path.join('CoolProp','include'), ignore_errors = True)
    shutil.copytree(os.path.join(CProot, 'include'), os.path.join('CoolProp','include'))
    shutil.copy2(os.path.join(CProot, 'CoolPropBibTeXLibrary.bib'), os.path.join('CoolProp', 'CoolPropBibTeXLibrary.bib'))
    print('files copied.')
    
def remove_files():
    import shutil
    shutil.rmtree(os.path.join('CoolProp','include'), ignore_errors = True)
    os.remove(os.path.join('CoolProp', 'CoolPropBibTeXLibrary.bib'))
    print('files removed.')
    
def touch(fname):
    open(fname, 'a').close()
    os.utime(fname, None)
    
if __name__=='__main__':
    
    
    
    import subprocess, shutil, os, sys, glob
        
      
    # ******************************
    #       CMAKE OPTIONS
    # ******************************

    # Example using CMake to build static library:
    # python setup.py install --cmake-compiler vc9 --cmake-bitness 64
    
    if '--cmake-compiler' in sys.argv:
        i = sys.argv.index('--cmake-compiler')
        sys.argv.pop(i)
        cmake_compiler = sys.argv.pop(i)
    else:
        cmake_compiler = ''
        
    if '--cmake-bitness' in sys.argv:
        i = sys.argv.index('--cmake-bitness')
        sys.argv.pop(i)
        cmake_bitness = sys.argv.pop(i)
    else:
        cmake_bitness = ''
        
    USING_CMAKE = cmake_compiler or cmake_bitness
        
    cmake_config_args = []
    cmake_build_args = ['--config','"Release"']
    STATIC_LIBRARY_BUILT = False
    if USING_CMAKE:
        
        # Always force build since any changes in the C++ files will not force a rebuild
        touch('CoolProp/CoolProp.pyx')
        
        if 'clean' in sys.argv:
            if os.path.exists('cmake_build'):
                print('removing cmake_build folder...')
                shutil.rmtree('cmake_build')
                print('removed.')
            
        if cmake_compiler == 'vc9':
            if cmake_bitness == '32':
                generator = ['-G','"Visual Studio 9 2008"']
            elif cmake_bitness == '64':
                generator = ['-G','"Visual Studio 9 2008 Win64"']
            else:
                raise ValueError('cmake_bitness must be either 32 or 64; got ' + cmake_bitness)
        elif cmake_compiler == 'vc10':
            if cmake_bitness == '32':
                generator = ['-G','"Visual Studio 10 2010"']
            elif cmake_bitness == '64':
                generator = ['-G','"Visual Studio 10 2010 Win64"']
            else:
                raise ValueError('cmake_bitness must be either 32 or 64; got ' + cmake_bitness)
        else:
            raise ValueError('cmake_compiler [' + cmake_compiler + '] is invalid')
            
        cmake_build_dir = os.path.join('cmake_build', '{compiler}-{bitness}bit'.format(compiler=cmake_compiler, bitness=cmake_bitness))
        if not os.path.exists(cmake_build_dir):
            os.makedirs(cmake_build_dir)
        subprocess.check_call(' '.join(['cmake','../../../..','-DCOOLPROP_STATIC_LIBRARY=ON']+generator+cmake_config_args), shell = True, stdout = sys.stdout, stderr = sys.stderr, cwd = cmake_build_dir)
        subprocess.check_call(' '.join(['cmake','--build', '.']+cmake_build_args), shell = True, stdout = sys.stdout, stderr = sys.stderr, cwd = cmake_build_dir)
        
        # Now find the static library that we just built
        if sys.platform == 'win32':
            static_libs = []
            for search_suffix in ['Release/*.lib','Release/*.a', 'Debug/*.lib', 'Debug/*.a']:
                static_libs += glob.glob(os.path.join(cmake_build_dir,search_suffix))
        
        if len(static_libs) != 1:
            raise ValueError("Found more than one static library using CMake build.  Found: "+str(static_libs))
        else:
            STATIC_LIBRARY_BUILT = True
            static_library_path = os.path.dirname(static_libs[0])
    
    # Check if a sdist build for pypi
    pypi = os.path.exists('.use_this_directory_as_root')
    
    """
    Modes of operation:
    1) Building the source distro (generate_headers.py must have been run before making the repo)
    2) Installing from source (generate_headers.py must have been run before making the repo)
    3) Installing from git repo (need to make sure to run generate_headers.py)
    4) 
    """ 
    
    # Determine whether or not to use Cython - default is to use cython unless the file .build_without_cython is found in the current working directory
    USE_CYTHON = not os.path.exists('.build_without_cython')
    cy_ext = 'pyx' if USE_CYTHON else 'cpp'

    if USE_CYTHON:
        # Check for cython >= 0.21 due to the use of cpdef enum
        try:
            import Cython
        except ImportError:
            raise ImportError("Cython not found, please install it.  You can do a pip install Cython")
            
        from pkg_resources import parse_version
        if parse_version(Cython.__version__) < parse_version('0.20'):
            raise ImportError('Your version of Cython (%s) must be >= 0.20 .  Please update your version of cython' % (Cython.__version__,))

        if parse_version(Cython.__version__) >= parse_version('0.20'):
            _profiling_enabled = True
        else:
            _profiling_enabled = False
            
        if _profiling_enabled:
            cython_directives = dict(profile = True,
                                     embed_signature = True)
        else:
            cython_directives = dict(embed_signature = True)
    else:
        cython_directives = {}

    # Determine the path to the root of the repository, the folder that contains the CMakeLists.txt file 
    # for normal builds, or the main directory for sdist builds
    if pypi:
        CProot = '.'
    else:
        if os.path.exists(os.path.join('..','..','CMakeLists.txt')):
            # Good working directory
            CProot = os.path.join('..','..')
        else:
            raise ValueError('Could not run script from this folder(' + os.path.abspath(os.path.curdir) + '). Run from wrappers/Python folder')
    
        sys.path.append(os.path.join(CProot, 'dev'))
        if not USING_CMAKE:
            import generate_headers
            # Generate the headers - does nothing if up to date - but only if not pypi
            generate_headers.generate()
            del generate_headers
                    
    # Read the version from a bare string stored in file in root directory
    version = open(os.path.join(CProot,'.version'),'r').read().strip()

    setup_kwargs = {}
    from setuptools import setup, Extension, find_packages
    if USE_CYTHON:
        import Cython.Compiler
        from Cython.Distutils.extension import Extension
        from Cython.Build import cythonize
        from Cython.Distutils import build_ext
        
        # This will always generate HTML to show where there are still pythonic bits hiding out
        Cython.Compiler.Options.annotate = True
        
        setup_kwargs['cmdclass'] = dict(build_ext = build_ext)
        
        print('Cython will be used; cy_ext is ' + cy_ext)
    else:
        print('Cython will not be used; cy_ext is ' + cy_ext)

    def find_cpp_sources(root = os.path.join('..','..','src'), extensions = ['.cpp'], skip_files = None):
        file_listing = []
        for path, dirs, files in os.walk(root):
            for file in files:
                n,ext = os.path.splitext(file)
                fname = os.path.relpath(os.path.join(path, file))
                if skip_files is not None and fname in skip_files: continue
                if ext in extensions:
                    file_listing.append(fname)
        return file_listing
        
    # Set variables for C++ sources and include directories
    sources = find_cpp_sources(os.path.join(CProot,'src'), '*.cpp') 
    include_dirs = [os.path.join(CProot, 'include'), os.path.join(CProot, 'src'), os.path.join(CProot, 'externals', 'Eigen')]

    ## If the file is run directly without any parameters, clean, build and install
    if len(sys.argv)==1:
       sys.argv += ['clean', 'install']
        
    common_args = dict(include_dirs = include_dirs,
                       language='c++')
   
    if USE_CYTHON:
        common_args.update(dict(cython_c_in_temp = True,
                                cython_directives = cython_directives
                                )
                           )
                       
    if STATIC_LIBRARY_BUILT == True:
        CoolProp_module = Extension('CoolProp.CoolProp',
                            [os.path.join('CoolProp','CoolProp.' + cy_ext)],
                            libraries = ['CoolProp'],
                            library_dirs = [static_library_path],
                            **common_args)
    else:
        CoolProp_module = Extension('CoolProp.CoolProp',
                            [os.path.join('CoolProp','CoolProp.' + cy_ext)] + sources,
                            **common_args)
    constants_module = Extension('CoolProp.constants',
                        [os.path.join('CoolProp','constants.' + cy_ext)],
                        **common_args)
     
    if not pypi:
        copy_files()

    ext_modules = [CoolProp_module, constants_module]
    
    if USE_CYTHON:
        ext_modules = cythonize(ext_modules)
        
    try:
        setup (name = 'CoolProp',
               version = version, # look above for the definition of version variable - don't modify it here
               author = "Ian Bell",
               author_email='ian.h.bell@gmail.com',
               url='http://www.coolprop.org',
               description = """Open-source thermodynamic and transport properties database""",
               packages = find_packages(),
               ext_modules = ext_modules,
               package_dir = {'CoolProp':'CoolProp',},
               package_data = {'CoolProp':['*.pxd',
                                           'CoolPropBibTeXLibrary.bib',
                                           'include/*.h',
                                           'include/rapidjson/*.h',
                                           'include/rapidjson/rapidjson/*.h',
                                           'include/rapidjson/rapidjson/internal/*.h']},
               classifiers = [
                "Programming Language :: Python",
                "Development Status :: 4 - Beta",
                "Environment :: Other Environment",
                "Intended Audience :: Developers",
                "License :: OSI Approved :: MIT License",
                "Operating System :: OS Independent",
                "Topic :: Software Development :: Libraries :: Python Modules"
                ],
               **setup_kwargs
               )
    except BaseException as E:
        if not pypi:
            remove_files()
        raise
    else:
        if not pypi:
            remove_files()
