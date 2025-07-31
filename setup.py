from setuptools import setup
from cffi import FFI

# Step 1: Declare the C API you need
ffi = FFI()
ffi.cdef(open("src/afaligner/c_modules/dtwbd.h").read())

# Step 2: Tell CFFI how to compile it
ffi.set_source(
    "afaligner.c_modules._dtwbd",                # Python module path
    ["src/afaligner/c_modules/dtwbd.c"],          # input C files
    extra_compile_args=[],                        # e.g. ["-O3"]
    # libraries=[],                               # e.g. ["m"]
)

# Step 3: Hook it into setuptools
setup(
    name="afaligner",
    version="0.2.0",
    description="Audio–subtitle aligner with custom DTW core",
    packages=["afaligner"],
    package_dir={"": "src"},
    install_requires=["cffi>=1.14"],
    ext_modules=[ffi.distutils_extension()],
    cmdclass={"build_ext": ffi.distutils_extension().build_ext},
    zip_safe=False,
)
