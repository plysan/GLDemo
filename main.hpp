/* included by modules for inter-module communication */

#include "tools/quardtreeTerrain.hpp"
#include "tools/physics.hpp"

extern bool looping;
extern long frame;
extern QTProfile qt_terrain;
extern glm::mat4 view_matrix;
extern glm::mat4 projection_matrix;
void computeMatricesFromInputs(Object* obj);
