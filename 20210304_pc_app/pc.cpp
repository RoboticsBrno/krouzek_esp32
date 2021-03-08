#include <stdio.h>

#include <unistd.h>

int main(int argc, const char* argv[]) {
    printf("Hello world!\n");
    printf("%d\n", argc);
    for (int i = 0; i < argc; ++i) {
        printf("  %s\n", argv[i]);
    }

    int* x;
    x[45123] = 4;
}
