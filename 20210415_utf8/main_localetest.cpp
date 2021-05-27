#include <stdio.h>
#include <locale.h>

int main(int argc, const char** argv) {
    char *loc = setlocale(LC_CTYPE, NULL);
    printf("Locale: %s\n", loc);
    printf("Locale: %s\n", argv[1]);
}
