#include "calculations.h"

int calcFactorial(int n) {
    int f = 1;
    for(int i = 1; i <= n; ++i) {
        f *= i;
    }
    return f;
}

int calcFibonnaci(int n) {
    if(n == 0 || n == 1) {
        return n;
    }
    return calcFibonnaci(n-1) + calcFibonnaci(n-2);
}
