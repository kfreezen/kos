#ifndef ERR_H
#define ERR_H

// Here are some global errors that we should set if something "common" to all
// of our code, e.g. out of memory
#define SUCCESS 0
#define ERR_OUT_OF_MEMORY 1
#define ERR_INVALID_ARG 2
#define ERR_NULL_VALUE_ENCOUNTERED 3

#define ERR_DEFINED_ELSEWHERE_BOTTOM 20

int GetErr();
void SetErr(int err);

#endif