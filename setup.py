from setuptools import setup
from cffi import FFI
import io

# 1) Build a single FFI builder and clean out all '#...' lines:
ffibuilder = FFI()
hdr = io.open("src/afaligner/c_modules/dtwbd.h", encoding="utf8").read()
decls = "\n".join(
    line for line in hdr.splitlines()
    if not line.lstrip().startswith("#")
)
ffibuilder.cdef(decls)

# 2) Tell it how to compile the real C code:
ffibuilder.set_source(
    "afaligner.c_modules._dtwbd",   # import path of your native module
    '#include "dtwbd.h"\n',         # string preamble only
    sources=["src/afaligner/c_modules/dtwbd.c"],
    include_dirs=["src/afaligner/c_modules"],
)

# 3) Extract the Extension object and its build_ext handler:
extension = ffibuilder.distutils_extension()
cmdclass = {"build_ext": extension.build_ext}

# 4) Standard setuptools.setup() with CFFI extension hooked in
setup(
    name="afaligner",
    version="0.2.0",
    description="Audio–subtitle aligner with custom DTW core",
    packages=["afaligner"],
    package_dir={"": "src"},
    install_requires=["cffi>=1.14"],
    ext_modules=[extension],
    cmdclass=cmdclass,
    zip_safe=False,
)
