#include <iostream>

#include "util/allocators.h"

#include "window/window.h"

int main(void)
{
    pyroc::Window window;
    window.init();

    window.loop();

    window.cleanup();
    return 0;
}