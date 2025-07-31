from setuptools import setup
from cffi import FFI
import io

# 1. Instantiate FFI
ffi = FFI()

# 2. Read & clean the header file for cdef()
header_path = "src/afaligner/c_modules/dtwbd.h"
with io.open(header_path, encoding="utf8") as f:
    raw = f.read()

# Strip out preprocessor directives
lines = []
for line in raw.splitlines():
    stripped = line.lstrip()
    if not stripped.startswith("#"):
        lines.append(line)
cleaned_header = "\n".join(lines)

# 3. Tell CFFI about the declarations
ffi.cdef(cleaned_header)

# 4. Configure compilation of the actual C module
ffi.set_source(
    "afaligner.c_modules._dtwbd",
    ["src/afaligner/c_modules/dtwbd.c"],
    # extra_compile_args, libraries, etc. as needed
)

# 5. Hook into setuptools
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
