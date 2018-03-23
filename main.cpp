#include <GL/glew.h>
#include <glm/glm.hpp>
#include <thread>
#include "modules/main_window.hpp"
#include "modules/world_ctx.hpp"
#include "modules/water.hpp"
#include "tools/physics.hpp"
#include "tools/tools.hpp"

bool looping = true;

// store vertex offset(according to earth's center) of the updating buffer
// and update this var to physics when using the updated buffer
glm::vec3 dbuf_world_offset;
// store view position used for the updating buffer(updating buffer's center is view position)
// and offset view position by this var when using the updated buffer
glm::vec3 dbuf_view_offset;
// according to earth's center
glm::vec3 initial_pos = calcFPosFromCoord(20.0f, -156.0f, 0.7f);
Object* viewObj;

GLuint render_target_uniform_id = -1;
GLuint programID;

int terrain_texture_unit_id = 0;
int scatter_texture_unit_id = 1;
int spacecube_texture_unit_id = 2;
int water_texture_unit_id = 3;

int main(void) {
    viewObj = new Object();
    main_win_pre_loop();
    world_pre_loop();
    water_pre_loop();
    std::thread first (thread_world_update_data);
    first.detach();
    do{
        world_loop();
        water_loop();
        main_win_loop();
    }
    while(looping);
    world_post_loop();
    water_post_loop();
    main_win_post_loop();

    return 0;
}

