#include <stdio.h>
#include "commons.h"

int main() {
    Logger_init("PROC!");
    Logger_log("Hello %s %d", "qwer", 12341);
    Logger_destruct();
    int i;
    scanf("arbabik%d", &i);
    return 0;
}