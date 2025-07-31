#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <float.h>
#include <stdio.h>

// On Windows, ssize_t isn’t defined by default
#if defined(_MSC_VER)
  #include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#endif

// Forward declarations for the algorithms
typedef struct {
    double distance;
    ssize_t prev_i;
    ssize_t prev_j;
} D_matrix_element;

ssize_t FastDTWBD(
    double *s, double *t,
    size_t n, size_t m, size_t l,
    double skip_penalty, int radius,
    double *path_distance,
    size_t *path_buffer
);

double *get_coarsed_sequence(double *s, size_t n, size_t l);

size_t *get_window(size_t n, size_t m,
                   size_t *path_buffer, size_t path_len,
                   int radius);

void update_window(size_t *window,
                   size_t n, size_t m,
                   ssize_t i, ssize_t j);

ssize_t DTWBD(
    double *s, double *t,
    size_t n, size_t m, size_t l,
    double skip_penalty,
    size_t *window,
    double *path_distance,
    size_t *path_buffer
);

double euclid_distance(double *x, double *y, size_t l);

double get_distance(D_matrix_element *D_matrix,
                    size_t n, size_t m,
                    size_t *window,
                    size_t i, size_t j);

D_matrix_element get_best_candidate(D_matrix_element *candidates, size_t n);

void reverse_path(size_t *path, ssize_t path_len);

//====================
// FastDTWBD wrapper
//====================
ssize_t FastDTWBD(
    double *s, double *t,
    size_t n, size_t m, size_t l,
    double skip_penalty, int radius,
    double *path_distance,
    size_t *path_buffer
) {
    ssize_t path_len;
    size_t min_sequence_len = 2 * (radius + 1) + 1;

    if (n < min_sequence_len || m < min_sequence_len) {
        return DTWBD(s, t, n, m, l,
                     skip_penalty, NULL,
                     path_distance, path_buffer);
    }

    double *coarsed_s = get_coarsed_sequence(s, n, l);
    double *coarsed_t = get_coarsed_sequence(t, m, l);

    path_len = FastDTWBD(
        coarsed_s, coarsed_t,
        n/2, m/2, l, skip_penalty,
        radius, path_distance, path_buffer
    );

    size_t *window = get_window(
        n, m, path_buffer, path_len, radius
    );

    path_len = DTWBD(
        s, t, n, m, l,
        skip_penalty, window,
        path_distance, path_buffer
    );

    free(coarsed_s);
    free(coarsed_t);
    free(window);

    return path_len;
}

//====================
// Coarsing helper
//====================
double *get_coarsed_sequence(double *s,
                             size_t n, size_t l)
{
    size_t coarsed_len = n / 2;
    double *seq = malloc(coarsed_len * l * sizeof(double));
    if (!seq) return NULL;

    for (size_t i = 0; 2*i + 1 < n; i++) {
        for (size_t j = 0; j < l; j++) {
            seq[l*i + j] = (s[l*(2*i) + j] + s[l*(2*i+1) + j]) * 0.5;
        }
    }
    return seq;
}

//====================
// Window generation
//====================
size_t *get_window(size_t n, size_t m,
                   size_t *path_buffer,
                   size_t path_len, int radius)
{
    size_t *window = malloc(2 * n * sizeof(size_t));
    if (!window) return NULL;

    // init to [m,0] so update_window shrinks ranges
    for (size_t i = 0; i < n; i++) {
        window[2*i]   = m;
        window[2*i+1] = 0;
    }

    for (size_t k = 0; k < path_len; k++) {
        size_t i = path_buffer[2*k];
        size_t j = path_buffer[2*k+1];
        for (ssize_t x = -radius; x <= radius; x++) {
            update_window(window, n, m, 2*(i+x), j - radius);
            update_window(window, n, m, 2*(i+x)+1, j - radius);
            update_window(window, n, m, 2*(i+x), j + radius + 1);
            update_window(window, n, m, 2*(i+x)+1, j + radius + 1);
        }
    }
    return window;
}

void update_window(size_t *window,
                   size_t n, size_t m,
                   ssize_t i, ssize_t j)
{
    if (i < 0 || (size_t)i >= n) return;
    if (j < 0)        j = 0;
    if (j >= (ssize_t)m) j = m - 1;

    size_t idx = 2*(size_t)i;
    if ((size_t)j < window[idx])       window[idx]   = (size_t)j;
    if ((size_t)j + 1 > window[idx+1]) window[idx+1] = (size_t)j + 1;
}

//====================
// Core DTWBD
//====================
ssize_t DTWBD(
    double *s, double *t,
    size_t n, size_t m, size_t l,
    double skip_penalty,
    size_t *window,
    double *path_distance,
    size_t *path_buffer
) {
    D_matrix_element *D = malloc(n * m * sizeof(D_matrix_element));
    if (!D) {
        fprintf(stderr, "ERROR: cannot allocate D_matrix\n");
        return -1;
    }

    double min_dist = skip_penalty * (n + m);
    double cur_dist;
    size_t end_i = 0, end_j = 0;
    bool match = false;

    for (size_t i = 0; i < n; i++) {
        size_t from = window ? window[2*i] : 0;
        size_t to   = window ? window[2*i+1] : m;
        for (size_t j = from; j < to; j++) {
            double d = euclid_distance(&s[i*l], &t[j*l], l);
            D_matrix_element cand[4] = {
                { skip_penalty*(i + j) + d, -1, -1 },
                { get_distance(D, n, m, window, i-1, j-1) + d, i-1, j-1 },
                { get_distance(D, n, m, window, i,   j-1) + d, i,   j-1 },
                { get_distance(D, n, m, window, i-1, j)   + d, i-1, j   }
            };
            D[i*m + j] = get_best_candidate(cand, 4);
            cur_dist   = D[i*m + j].distance + skip_penalty*(n-i + m-j -2);
            if (cur_dist < min_dist) {
                min_dist = cur_dist;
                end_i = i; end_j = j;
                match = true;
            }
        }
    }

    ssize_t path_len = 0;
    if (match) {
        *path_distance = min_dist;
        ssize_t ii = end_i, jj = end_j;
        while (ii >= 0 && jj >= 0) {
            D_matrix_element *e = &D[ii*m + jj];
            path_buffer[2*path_len]   = (size_t)ii;
            path_buffer[2*path_len+1] = (size_t)jj;
            path_len++;
            ii = e->prev_i; jj = e->prev_j;
        }
        reverse_path(path_buffer, path_len);
    }

    free(D);
    return path_len;
}

//====================
// Utilities
//====================
double euclid_distance(double *x, double *y, size_t l) {
    double sum = 0.0;
    for (size_t i = 0; i < l; i++) {
        double diff = x[i] - y[i];
        sum += diff * diff;
    }
    return sqrt(sum);
}

double get_distance(D_matrix_element *D,
                    size_t n, size_t m,
                    size_t *window,
                    size_t i, size_t j)
{
    // out-of-bounds -> infinite cost
    if ((ssize_t)i < 0 || i >= n || (ssize_t)j < 0 || j >= m)
        return DBL_MAX;
    size_t idx = i*m + j;
    if (!window ||
        (j >= window[2*i] && j < window[2*i+1]))
    {
        return D[idx].distance;
    }
    return DBL_MAX;
}

D_matrix_element get_best_candidate(D_matrix_element *cands, size_t n) {
    D_matrix_element best = cands[0];
    for (size_t i = 1; i < n; i++) {
        if (cands[i].distance < best.distance) {
            best = cands[i];
        }
    }
    return best;
}

void reverse_path(size_t *path, ssize_t len) {
    for (ssize_t i = 0, j = len - 1; i < j; i++, j--) {
        size_t si = path[2*i], sj = path[2*i+1];
        path[2*i]   = path[2*j];
        path[2*i+1] = path[2*j+1];
        path[2*j]   = si;
        path[2*j+1] = sj;
    }
}

#ifdef _WIN32
  #include <Python.h>
  // Stub PyInit so Windows linking succeeds
  PyMODINIT_FUNC PyInit_dtwbd(void) { return NULL; }
#endif
