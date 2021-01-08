#include <Arduino.h>
#include "calculations.h"

void setup() {
    delay(500);
    printf("\n\n");


    printf("5! == %d\n", calcFactorial(5));

    for(int i = 0; i < 24; ++i) {
        printf("%d ", calcFibonnaci(i));
    }
    printf("\n");
}

void loop() {

}
