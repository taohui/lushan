/* Authors: */
/*     Tao Hui <taohui3@gmail.com> */
#ifndef VLONG_H
#define VLONG_H

#ifdef __cplusplus
extern "C"
{
#endif

int write_vlong(char **dest, int *left, long i);
int read_vlong(char **src, int *left, long *i);

#ifdef __cplusplus
}
#endif

#endif
