#include <string.h>

#include <stddef.h>

char *strncpy(char *dest, const char *src, size_t count) {
    size_t i = 0;

    for (; i < count && src[i] != 0; i++) {
        dest[i] = src[i];
    }
    for (; i < count; i++) {
        dest[i] = 0;
    }

    return dest;
}

int strncmp(const char *lhs, const char *rhs, size_t count) {
    int ret = 0;

    for (size_t i = 0; i < count; i++) {
        ret = lhs[i] - rhs[i];
        if (ret || lhs[i] == 0) {
            break;
        }
    }

    return ret;
}

void *memchr(const void *ptr, int ch, size_t count) {
    const unsigned char *byte_ptr = (const unsigned char *)ptr;

    for (int i = 0; i < count; ++i) {
        if (byte_ptr[i] == (unsigned char)ch) {
            return (void *)&byte_ptr[i];
        }
    }

    return NULL;
}

int memcmp(const void *lhs, const void *rhs, size_t count) {
    const unsigned char *lhs_byte_ptr = (const unsigned char *)lhs;
    const unsigned char *rhs_byte_ptr = (const unsigned char *)rhs;

    for (int i = 0; i < count; ++i) {
        if (lhs_byte_ptr[i] < rhs_byte_ptr[i]) {
            return -1;
        } else if (lhs_byte_ptr[i] > rhs_byte_ptr[i]) {
            return 1;
        }
    }

    return 0;
}

void *memset(void *dest, int ch, size_t count) {
    unsigned char *byte_ptr = (unsigned char *)dest;

    for (int i = 0; i < count; ++i) {
        byte_ptr[i] = (unsigned char)ch;
    }

    return dest;
}

void *memcpy(void *restrict dest, const void *restrict src, size_t count) {
    unsigned char *byte_ptr1 = (unsigned char *)dest;
    unsigned char *byte_ptr2 = (unsigned char *)src;

    for (int i = 0; i < count; ++i)
    {
        byte_ptr1[i] = byte_ptr2[i];
    }

    return dest;
}
