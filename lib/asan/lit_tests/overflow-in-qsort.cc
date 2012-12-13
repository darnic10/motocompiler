// RUN: %clangxx_asan -O2 %s -o %t && %t 2>&1 | %symbolize | FileCheck %s

// Test how well we unwind in presence of qsort in the stack
// (i.e. if we can unwind through a function compiled w/o frame pointers).
// https://code.google.com/p/address-sanitizer/issues/detail?id=137
#include <stdlib.h>
#include <stdio.h>

int global_array[10];
volatile int one = 1;

extern "C"
int QsortCallback(const void *a, const void *b) {
  char *x = (char*)a;
  char *y = (char*)b;
  printf("Calling QsortCallback\n");
  global_array[one * 10] = 0;  // BOOM
  return (int)*x - (int)*y;
}

__attribute__((noinline))
void MyQsort(char *a, size_t size) {
  printf("Calling qsort\n");
  qsort(a, size, sizeof(char), QsortCallback);
  printf("Done\n");  // Avoid tail call.
}

int main() {
  char a[2] = {1, 2};
  MyQsort(a, 2);
}

// CHECK: ERROR: AddressSanitizer: global-buffer-overflow
// CHECK: #0{{.*}} in {{_?}}QsortCallback
// CHECK: is located 0 bytes to the right of global variable 'global_array
