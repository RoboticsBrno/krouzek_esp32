#include <time.h>

#include "hlavicka.h"

bool definovanaJinde() {
    return time(NULL) % 2 == 0;
}
