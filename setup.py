# setup.py
from setuptools import setup
from cffi import FFI
import io

ffi = FFI()

# 1) Read & strip your header of ANY '#' lines (guards, includes, etc.)
with io.open("src/afaligner/c_modules/dtwbd.h", encoding="utf8") as f:
    lines = [
        line for line in f.read().splitlines()
        if not line.lstrip().startswith("#")
    ]
cleaned_cdefs = "\n".join(lines)

# 2) Tell CFFI about the *clean* declarations
ffi.cdef(cleaned_cdefs)

# 3) Correctly call set_source:
#    - First arg: module name
#    - Second arg: PREAMBLE string, not a list
#    - Then pass your C sources via the 'sources' kw
ffi.set_source(
    "afaligner.c_modules._dtwbd",
    '#include "dtwbd.h"\n',               # this is a *string* preamble
    sources=["src/afaligner/c_modules/dtwbd.c"],
    include_dirs=["src/afaligner/c_modules"],
)

# 4) Plug into setuptools as usual
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
