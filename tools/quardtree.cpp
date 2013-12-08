#include <stdio.h>
#include <stdlib.h>
#include <glm/glm.hpp>
#include <glm/gtx/constants.hpp>
#include "constants.hpp"

extern glm::vec3 viewPos;

static glm::vec3* result;
static glm::vec2* result_uv;
static int interpolate_count = 5;
static int dinmension = interpolate_count * interpolate_count;
static int maxNodes = 1000;
static int nodeIndex = 0;
static float minNodeSize = 5.0f;
static float maxNodeSize = 200.0f;

static glm::vec2 bl_coord_context;
static glm::vec2 coord_delta_context;

glm::vec3* createQuardTreePos() {
    result = new glm::vec3[maxNodes * dinmension * dinmension];
    return result;
}

glm::vec2* createQuardTreeUV() {
    result_uv = new glm::vec2[maxNodes * dinmension * dinmension];
    return result_uv;
}

glm::vec3 calcPosFromCoord(float lat, float lng) {
    lat = lat/180*localcons::pi;
    lng = lng/180*localcons::pi;
    return glm::vec3(
            localcons::earth_radius * cos(lat) * cos(lng),
            localcons::earth_radius * sin(lat),
            localcons::earth_radius * cos(lat) * sin(lng)
            );
}

void interpolatePos(glm::vec3 tl_pos, glm::vec3 bl_pos, glm::vec3 tr_pos, glm::vec3 br_pos, int mid_pos_index, int unit_size) {
    glm::vec3 mt_pos = (tl_pos + tr_pos)/2.0f;
    glm::vec3 mb_pos = (bl_pos + br_pos)/2.0f;
    glm::vec3 ml_pos = (bl_pos + tl_pos)/2.0f;
    glm::vec3 mr_pos = (br_pos + tr_pos)/2.0f;
    glm::vec3 mid_pos = (ml_pos + mr_pos)/2.0f;
    result[mid_pos_index] = mid_pos;
    result[mid_pos_index + unit_size] = mr_pos;
    result[mid_pos_index + dinmension*unit_size] = mb_pos;
    if (unit_size > 1) {
        int sub_unit_size = unit_size/2;
        interpolatePos(tl_pos, ml_pos, mt_pos, mid_pos, mid_pos_index - dinmension*sub_unit_size - sub_unit_size, sub_unit_size);
        interpolatePos(mt_pos, mid_pos, tr_pos, mr_pos, mid_pos_index - dinmension*sub_unit_size + sub_unit_size, sub_unit_size);
        interpolatePos(ml_pos, bl_pos, mid_pos, mb_pos, mid_pos_index + dinmension*sub_unit_size - sub_unit_size, sub_unit_size);
        interpolatePos(mid_pos, mb_pos, mr_pos, br_pos, mid_pos_index + dinmension*sub_unit_size + sub_unit_size, sub_unit_size);
    }
}

void addNodeToResult(glm::vec2 bl_coord, glm::vec2 tr_coord) {
    if (nodeIndex >= maxNodes) {
        return;
    }
    int baseIndex = nodeIndex * dinmension * dinmension;
    glm::vec3 bl_pos = calcPosFromCoord(bl_coord.x, bl_coord.y);
    glm::vec3 br_pos = calcPosFromCoord(bl_coord.x, tr_coord.y);
    glm::vec3 tl_pos = calcPosFromCoord(tr_coord.x, bl_coord.y);
    glm::vec3 tr_pos = calcPosFromCoord(tr_coord.x, tr_coord.y);
    interpolatePos(tl_pos, bl_pos, tr_pos, br_pos, baseIndex + dinmension*dinmension/2, dinmension/2);
    float d_uv = (tr_coord.y - bl_coord.y)/coord_delta_context.y/(float)dinmension;
    result_uv[baseIndex] = glm::vec2(
            (bl_coord.y - bl_coord_context.y)/coord_delta_context.y,
            (tr_coord.x - bl_coord_context.x)/coord_delta_context.x
            );
    for (int i=0; i<dinmension; i++) {
        int iIndex = baseIndex + i * dinmension;
        for (int j=0; j<dinmension; j++) {
            int cur_index = iIndex + j;
            if (j == 0) {
                if (i == 0) {
                    continue;
                }
                result_uv[cur_index].x = result_uv[baseIndex].x;
                result_uv[cur_index].y = result_uv[cur_index - dinmension].y - d_uv;
                continue;
            }
            result_uv[cur_index].x = result_uv[cur_index - 1].x + d_uv;
            result_uv[cur_index].y = result_uv[iIndex].y;
        }
    }
    nodeIndex++;
}

void selectNode(glm::vec2 bl_coord, glm::vec2 tr_coord) {
    glm::vec3 bl_pos = calcPosFromCoord(bl_coord.x, bl_coord.y);
    glm::vec3 tr_pos = calcPosFromCoord(tr_coord.x, tr_coord.y);
    float nodeSize = glm::length(bl_pos - tr_pos);
    if (nodeSize < minNodeSize) {
        addNodeToResult(bl_coord, tr_coord);
        return;
    }
    if (nodeSize > glm::length((bl_pos + tr_pos)/2.0f - viewPos) || nodeSize > maxNodeSize) {
        glm::vec2 tl_coord = glm::vec2(tr_coord.x, bl_coord.y);
        glm::vec2 br_coord = glm::vec2(bl_coord.x, tr_coord.y);
        glm::vec2 mid_coord = (bl_coord + tr_coord)/2.0f;
        glm::vec2 mt_coord = (tl_coord + tr_coord)/2.0f;
        glm::vec2 mb_coord = (bl_coord + br_coord)/2.0f;
        glm::vec2 ml_coord = (tl_coord + bl_coord)/2.0f;
        glm::vec2 mr_coord = (tr_coord + br_coord)/2.0f;
        selectNode(ml_coord, mt_coord);
        selectNode(bl_coord, mid_coord);
        selectNode(mid_coord, tr_coord);
        selectNode(mb_coord, mr_coord);
    } else {
        addNodeToResult(bl_coord, tr_coord);
    }
}

void createQuardTree(glm::vec2 bl_coord, glm::vec2 tr_coord, int* index, glm::vec3* result_ret, glm::vec2* result_uv_ret) {
    nodeIndex = *index / dinmension / dinmension;
    bl_coord_context = bl_coord;
    coord_delta_context = tr_coord - bl_coord;
    result = result_ret;
    result_uv = result_uv_ret;
    selectNode(bl_coord, tr_coord);
    *index = nodeIndex * dinmension * dinmension;
}

