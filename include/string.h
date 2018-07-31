#ifndef STRING_H_INCLUDED_
#define STRING_H_INCLUDED_

#include <int_types.h>

// string manipulation
char *strncpy(char *dest, const char *src, size_t count);

// string examination
int strncmp(const char *lhs, const char *rhs, size_t count);

// Memory manipulation
void *memchr(const void *ptr, int ch, size_t count);
int memcmp(const void *lhs, const void *rhs, size_t count);
void *memset(void *dest, int ch, size_t count);
void *memcpy(void *restrict dest, const void *restrict src, size_t count);

#endif // STRING_H_INCLUDED_
