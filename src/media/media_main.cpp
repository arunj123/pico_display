#include "MediaApplication.h"

// The main entry point is now incredibly simple
extern "C" int btstack_main(void) {
    static MediaApplication app;
    app.run();
    return 0;
}