#include <SDL3/SDL_main.h>
#include "application.hpp"

int main(int argc, char* argv[])
{
    Application app;
    if (app.initialize())
    {
        app.run();
    }
    app.shutDown();
    return 0;
}