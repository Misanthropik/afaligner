#ifndef DTWBD_H
#define DTWBD_H

#include <stddef.h>
#include <sys/types.h>  // for ssize_t on Unix
#ifdef _MSC_VER
  #include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#endif

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

#endif // DTWBD_H
