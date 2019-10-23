#include <stdio.h>

#define KONSTANTA 4

#define VYRAZ 4+4
#define VYRAZ_ZAVORKY (4+4)

#define nasob(kolik, telo) \
    for(int i = 0; i < kolik; ++i) { \
        telo; \
    };

const double double_const = 4;
#define double_define 4

void metoda() {

    int x = 2*VYRAZ; // 2*4 + 4 = 12
    int y = 2*VYRAZ_ZAVORKY; // 2*(4+4) = 16

    nasob(10, {
        printf("aaa");
    });

    double z = 21/double_const; // 5.2
    double w = 21/double_define; // 5
}
