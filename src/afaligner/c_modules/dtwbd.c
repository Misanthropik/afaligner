#ifdef _WIN32
  // Force-export a dummy PyInit_dtwbd so MSVC’s /EXPORT flag finds it
  __declspec(dllexport) void PyInit_dtwbd(void) { /* noop */ }
#endif

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

//-----------------------------------------------------
// Forward declarations
//-----------------------------------------------------
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

size_t *get_window(
    size_t n, size_t m,
    size_t *path_buffer, size_t path_len,
    int radius
);

void update_window(
    size_t *window,
    size_t n, size_t m,
    ssize_t i, ssize_t j
);

ssize_t DTWBD(
    double *s, double *t,
    size_t n, size_t m, size_t l,
    double skip_penalty,
    size_t *window,
    double *path_distance,
    size_t *path_buffer
);

double euclid_distance(double *x, double *y, size_t l);

double get_distance(
    D_matrix_element *D,
    size_t n, size_t m,
    size_t *window,
    size_t i, size_t j
);

D_matrix_element get_best_candidate(
    D_matrix_element *cands, size_t n
);

void reverse_path(size_t *path, ssize_t len);


//-----------------------------------------------------
// Fast approximate DTWBD wrapper
//-----------------------------------------------------
ssize_t FastDTWBD(
    double *s, double *t,
    size_t n, size_t m, size_t l,
    double skip_penalty, int radius,
    double *path_distance,
    size_t *path_buffer
) {
    size_t min_len = 2 * (radius + 1) + 1;
    if (n < min_len || m < min_len)
        return DTWBD(s, t, n, m, l, skip_penalty,
                     NULL, path_distance, path_buffer);

    double *cs = get_coarsed_sequence(s, n, l);
    double *ct = get_coarsed_sequence(t, m, l);

    ssize_t path_len = FastDTWBD(
        cs, ct, n/2, m/2, l,
        skip_penalty, radius,
        path_distance, path_buffer
    );

    size_t *window = get_window(n, m, path_buffer, path_len, radius);

    path_len = DTWBD(
        s, t, n, m, l,
        skip_penalty, window,
        path_distance, path_buffer
    );

    free(cs);
    free(ct);
    free(window);
    return path_len;
}


//-----------------------------------------------------
// Build coarsed sequence (half-length)
//-----------------------------------------------------
double *get_coarsed_sequence(double *s, size_t n, size_t l) {
    size_t clen = n / 2;
    double *out = malloc(clen * l * sizeof(double));
    if (!out) return NULL;

    for (size_t i = 0; 2*i+1 < n; i++) {
        for (size_t j = 0; j < l; j++) {
            out[l*i + j] = (s[2*i*l + j] + s[(2*i+1)*l + j]) * 0.5;
        }
    }
    return out;
}


//-----------------------------------------------------
// Build search window around coarse path
//-----------------------------------------------------
size_t *get_window(
    size_t n, size_t m,
    size_t *path_buffer, size_t path_len,
    int radius
) {
    size_t *window = malloc(2 * n * sizeof(size_t));
    if (!window) return NULL;

    // Initialize to [m,0] so update_window shrinks it
    for (size_t i = 0; i < n; i++) {
        window[2*i]   = m;
        window[2*i+1] = 0;
    }

    for (size_t k = 0; k < path_len; k++) {
        size_t i = path_buffer[2*k];
        size_t j = path_buffer[2*k+1];
        for (ssize_t x = -radius; x <= radius; x++) {
            update_window(window, n, m, 2*(i+x),     j - radius);
            update_window(window, n, m, 2*(i+x) + 1, j - radius);
            update_window(window, n, m, 2*(i+x),     j + radius + 1);
            update_window(window, n, m, 2*(i+x) + 1, j + radius + 1);
        }
    }
    return window;
}

void update_window(
    size_t *window, size_t n, size_t m,
    ssize_t i, ssize_t j
) {
    if (i < 0 || (size_t)i >= n) return;
    if (j < 0) j = 0;
    if (j >= (ssize_t)m) j = m - 1;

    size_t idx = 2*(size_t)i;
    if ((size_t)j < window[idx])       window[idx]   = (size_t)j;
    if ((size_t)j + 1 > window[idx+1]) window[idx+1] = (size_t)j + 1;
}


//-----------------------------------------------------
// Core DTWBD algorithm
//-----------------------------------------------------
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
        fprintf(stderr, "ERROR: malloc() failed\n");
        return -1;
    }

    double best_dist = skip_penalty * (n + m);
    ssize_t path_len = 0;
    bool matched = false;
    size_t end_i = 0, end_j = 0;

    for (size_t i = 0; i < n; i++) {
        size_t f = window ? window[2*i]   : 0;
        size_t tto = window ? window[2*i+1] : m;
        for (size_t j = f; j < tto; j++) {
            double d = euclid_distance(&s[i*l], &t[j*l], l);
            D_matrix_element cands[4] = {
                { skip_penalty*(i+j) + d, -1, -1 },
                { get_distance(D,n,m,window,i-1,j-1)+d, i-1, j-1 },
                { get_distance(D,n,m,window,i,  j-1)+d, i,   j-1 },
                { get_distance(D,n,m,window,i-1,j)  +d, i-1, j   }
            };
            D[i*m + j] = get_best_candidate(cands, 4);

            double cur = D[i*m + j].distance
                       + skip_penalty*(n-i + m-j - 2);
            if (cur < best_dist) {
                best_dist = cur;
                end_i = i; end_j = j;
                matched = true;
            }
        }
    }

    if (matched) {
        *path_distance = best_dist;
        ssize_t pi = end_i, pj = end_j;
        while (pi >= 0 && pj >= 0) {
            D_matrix_element *e = &D[pi*m + pj];
            path_buffer[2*path_len]   = (size_t)pi;
            path_buffer[2*path_len+1] = (size_t)pj;
            path_len++;
            pi = e->prev_i;
            pj = e->prev_j;
        }
        reverse_path(path_buffer, path_len);
    }

    free(D);
    return path_len;
}


//-----------------------------------------------------
// Utility functions
//-----------------------------------------------------
double euclid_distance(double *x, double *y, size_t l) {
    double sum = 0.0;
    for (size_t i = 0; i < l; i++) {
        double diff = x[i] - y[i];
        sum += diff*diff;
    }
    return sqrt(sum);
}

double get_distance(
    D_matrix_element *D,
    size_t n, size_t m,
    size_t *window,
    size_t i, size_t j
) {
    if ((ssize_t)i < 0 || i >= n || (ssize_t)j < 0 || j >= m)
        return DBL_MAX;
    size_t idx = i*m + j;
    if (!window || (j >= window[2*i] && j < window[2*i+1]))
        return D[idx].distance;
    return DBL_MAX;
}

D_matrix_element get_best_candidate(
    D_matrix_element *cands, size_t count
) {
    D_matrix_element best = cands[0];
    for (size_t i = 1; i < count; i++) {
        if (cands[i].distance < best.distance)
            best = cands[i];
    }
    return best;
}

void reverse_path(size_t *path, ssize_t len) {
    for (ssize_t i = 0, j = len-1; i < j; i++, j--) {
        size_t si = path[2*i], sj = path[2*i+1];
        size_t ti = path[2*j], tj = path[2*j+1];
        path[2*i]   = ti; path[2*i+1] = tj;
        path[2*j]   = si; path[2*j+1] = sj;
    }
}
