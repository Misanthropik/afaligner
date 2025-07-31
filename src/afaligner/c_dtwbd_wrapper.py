# src/afaligner/c_dtwbd_wrapper.py

from cffi import FFI
import numpy as np

# ----------------------------------------------------------------------------
# 1) Define the C API
# ----------------------------------------------------------------------------
ffi = FFI()
ffi.cdef("""
    typedef struct {
        double distance;
        size_t prev_i;
        size_t prev_j;
    } D_matrix_element;

    ssize_t FastDTWBD(
        double *s,
        double *t,
        size_t n,
        size_t m,
        size_t l,
        double skip_penalty,
        int    radius,
        double *path_distance,
        size_t *path_buffer
    );
""")

# ----------------------------------------------------------------------------
# 2) Point at your C sources and header
# ----------------------------------------------------------------------------
ffi.set_source(
    "afaligner.c_modules._dtwbd",       # this becomes the compiled extension
    '#include "dtwbd.h"',                # your header in c_modules/
    sources=["src/afaligner/c_modules/dtwbd.c"],
    include_dirs=["src/afaligner/c_modules"],
)

# ----------------------------------------------------------------------------
# 3) Python wrapper signature (same as before)
# ----------------------------------------------------------------------------
class FastDTWBDError(Exception):
    """Raised when the C FastDTWBD call returns an error code."""


def c_FastDTWBD(s: np.ndarray,
                t: np.ndarray,
                skip_penalty: float,
                radius: int):
    """
    Run the FastDTWBD algorithm (via the C extension).
    Returns (distance, path_buffer—as an (L,2) np.uintp array).
    """
    # Import the compiled extension
    from afaligner.c_modules import _dtwbd
    lib = _dtwbd.lib
    _ffi = _dtwbd.ffi

    # Ensure double-precision contiguous arrays
    s_arr = np.ascontiguousarray(s, dtype=np.double)
    t_arr = np.ascontiguousarray(t, dtype=np.double)

    # Dimensions
    n, l = s_arr.shape
    m, _ = t_arr.shape

    # Prepare output holders
    path_distance = _ffi.new("double *")
    buf_len = (n + m) * 2
    path_buffer = _ffi.new(f"size_t[{buf_len}]")

    # Call into C
    ret = lib.FastDTWBD(
        _ffi.cast("double *", _ffi.from_buffer(s_arr)),
        _ffi.cast("double *", _ffi.from_buffer(t_arr)),
        n, m, l,
        skip_penalty,
        radius,
        path_distance,
        path_buffer
    )

    if ret < 0:
        raise FastDTWBDError(
            "FastDTWBD() returned error code %d" % ret
        )

    # Slice out only the used entries and reshape to (ret, 2)
    raw = np.frombuffer(
        _ffi.buffer(path_buffer, buf_len * _ffi.sizeof("size_t")),
        dtype=np.uintp
    )
    path = raw.reshape(-1, 2)[:ret]

    return float(path_distance[0]), path


# ----------------------------------------------------------------------------
# 4) Allow ad-hoc compile if you ever run this file directly
# ----------------------------------------------------------------------------
if __name__ == "__main__":
    ffi.compile(verbose=True)
