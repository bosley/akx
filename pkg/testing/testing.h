#ifndef TESTING_H
#define TESTING_H

#include <ak24/kernel.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT_TRUE(condition)                                                 \
  do {                                                                         \
    if (!(condition)) {                                                        \
      printf("ASSERT_TRUE failed: %s\n", #condition);                          \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_FALSE(condition)                                                \
  do {                                                                         \
    if (condition) {                                                           \
      printf("ASSERT_FALSE failed: %s\n", #condition);                         \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_FLOAT_EQ(a, b, epsilon)                                         \
  do {                                                                         \
    if (fabs(a - b) > epsilon) {                                               \
      printf("ASSERT_FLOAT_EQ failed: %f != %f (epsilon: %f)\n", (double)(a),  \
             (double)(b), (double)(epsilon));                                  \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_DOUBLE_EQ(a, b, epsilon)                                        \
  do {                                                                         \
    if (fabs(a - b) > epsilon) {                                               \
      printf("ASSERT_DOUBLE_EQ failed: %f != %f (epsilon: %f)\n", (double)(a), \
             (double)(b), (double)(epsilon));                                  \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_EQ(a, b)                                                        \
  do {                                                                         \
    if ((a) != (b)) {                                                          \
      printf("ASSERT_EQ failed: %lld != %lld\n", (long long)(a),               \
             (long long)(b));                                                  \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_NE(a, b)                                                        \
  do {                                                                         \
    if ((a) == (b)) {                                                          \
      printf("ASSERT_NE failed: %lld == %lld\n", (long long)(a),               \
             (long long)(b));                                                  \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_STREQ(a, b)                                                     \
  do {                                                                         \
    if (strcmp(a, b) != 0) {                                                   \
      printf("ASSERT_STREQ failed: %s != %s\n", a, b);                         \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_STRNE(a, b)                                                     \
  do {                                                                         \
    if (strcmp(a, b) == 0) {                                                   \
      printf("ASSERT_STRNE failed: %s == %s\n", a, b);                         \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_NULL(ptr)                                                       \
  do {                                                                         \
    if ((ptr) != NULL) {                                                       \
      printf("ASSERT_NULL failed: %p != NULL\n", (void *)(ptr));               \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define ASSERT_NOT_NULL(ptr)                                                   \
  do {                                                                         \
    if ((ptr) == NULL) {                                                       \
      printf("ASSERT_NOT_NULL failed: %p == NULL\n", (void *)(ptr));           \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#endif
