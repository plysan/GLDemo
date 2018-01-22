#include "modules/main_window.hpp"
#include "modules/world_ctx.hpp"

bool looping = true;

int main(void) {
    main_win_pre_loop();
    world_pre_loop();
    do{
        world_loop();
        main_win_loop();
    }
    while(looping);
    world_post_loop();
    main_win_post_loop();

    return 0;
}

