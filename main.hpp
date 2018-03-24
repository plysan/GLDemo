/* included by modules for inter-module communication */

#include "tools/quardtreeTerrain.hpp"
#include "tools/physics.hpp"

extern bool looping;
extern long frame;
extern QTProfile qt_terrain;
extern glm::mat4 view_matrix;
extern glm::mat4 projection_matrix;
void computeMatricesFromInputs(Object* obj);
extern glm::dvec3 dbuf_world_offset;
extern glm::vec3 dbuf_view_offset;
extern glm::vec3 initial_pos;
extern Object* viewObj;
extern GLuint render_target_uniform_id;
extern GLuint programID;
extern int terrain_texture_unit_id;
extern int scatter_texture_unit_id;
extern int spacecube_texture_unit_id;
extern int water_texture_unit_id;
