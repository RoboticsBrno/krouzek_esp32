#include "hlavicka.h"

#define PI 3.14
static int tatoFce() {
   return definovanaJinde() ? PI : 42;
}

int main() {
    return tatoFce();
}
