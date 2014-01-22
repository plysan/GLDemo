/** 
 * Level Of Detail implementation for terrain rendering
 *
 * This impl includes:
 *  1. vertex(currently position) generation using quardtree selection algorithm.
 *  2. One texture with different LOD texture sub units combined
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <tiffio.h>
#include <glm/glm.hpp>
#include <glm/gtx/constants.hpp>
#include <GL/glew.h>
#include "constants.hpp"
#include "tools.hpp"

using namespace std;

extern glm::vec3 viewPos;

static glm::vec3* result;
static glm::vec2* result_uv;
static unsigned int* result_index;

// dinmension must be 2^n+1 where n is int
static int dinmension = 9;
static int ele_index_node_size = 2 * dinmension * (dinmension - 1) + 2;
static int maxNodes = 1000;
static int nodeIndex = 0;
static float minNodeSize = 1.0f;
static float maxNodeSize = 20.0f;

static int texture_unit_size = 3600;
static int texture_unit_dinmension = 2;
static int texture_units = texture_unit_dinmension * texture_unit_dinmension;
static float* texture;
static int texture_unit_index = 0;

glm::vec3* createQuardTreePos() {
    result = new glm::vec3[maxNodes * dinmension * dinmension];
    return result;
}

glm::vec2* createQuardTreeUV() {
    result_uv = new glm::vec2[maxNodes * dinmension * dinmension];
    return result_uv;
}

unsigned int* createQuardTreeElementIndex() {
    result_index = new unsigned int[maxNodes * ele_index_node_size];
    return result_index;
}

void interpolatePos2D(glm::vec3 tl_pos, glm::vec3 bl_pos, glm::vec3 tr_pos, glm::vec3 br_pos, glm::vec2 bl_uv, glm::vec2 tr_uv, int mid_pos_index, int unit_size) {
    glm::vec3 mt_pos = (tl_pos + tr_pos)/2.0f;
    glm::vec3 mb_pos = (bl_pos + br_pos)/2.0f;
    glm::vec3 ml_pos = (bl_pos + tl_pos)/2.0f;
    glm::vec3 mr_pos = (br_pos + tr_pos)/2.0f;
    glm::vec3 mid_pos = (ml_pos + mr_pos)/2.0f;
    result[mid_pos_index] = mid_pos;
    result[mid_pos_index + unit_size] = mr_pos;
    result[mid_pos_index + dinmension*unit_size] = mb_pos;

    glm::vec2 mid_uv = (bl_uv + tr_uv)/2.0f;
    glm::vec2 mr_uv = glm::vec2(tr_uv.x, mid_uv.y);
    glm::vec2 mb_uv = glm::vec2(mid_uv.x, bl_uv.y);
    glm::vec2 ml_uv = glm::vec2(bl_uv.x, mid_uv.y);
    glm::vec2 mt_uv = glm::vec2(mid_uv.x, tr_uv.y);
    result_uv[mid_pos_index] = mid_uv;
    result_uv[mid_pos_index + unit_size] = mr_uv;
    result_uv[mid_pos_index + dinmension*unit_size] = mb_uv;
    if (unit_size > 1) {
        int sub_unit_size = unit_size/2;
        interpolatePos2D(tl_pos, ml_pos, mt_pos, mid_pos, ml_uv, mt_uv, mid_pos_index - dinmension*sub_unit_size - sub_unit_size, sub_unit_size);
        interpolatePos2D(mt_pos, mid_pos, tr_pos, mr_pos, mid_uv, tr_uv, mid_pos_index - dinmension*sub_unit_size + sub_unit_size, sub_unit_size);
        interpolatePos2D(ml_pos, bl_pos, mid_pos, mb_pos, bl_uv, mid_uv, mid_pos_index + dinmension*sub_unit_size - sub_unit_size, sub_unit_size);
        interpolatePos2D(mid_pos, mb_pos, mr_pos, br_pos, mb_uv, mr_uv, mid_pos_index + dinmension*sub_unit_size + sub_unit_size, sub_unit_size);
    }
}

void interpolatePos1D(glm::vec3 frst_pos, glm::vec3 lst_pos, glm::vec2 frst_uv, glm::vec2 lst_uv, int mid_pos_index, int interval, int unit_size) {
    glm::vec3 mid_pos = (frst_pos + lst_pos)/2.0f;
    result[mid_pos_index] = mid_pos;
    glm::vec2 mid_uv = (frst_uv + lst_uv)/2.0f;
    result_uv[mid_pos_index] = mid_uv;
    if (interval >= unit_size) {
        interpolatePos1D(frst_pos, mid_pos, frst_uv, mid_uv, mid_pos_index-interval, interval/2, unit_size);
        interpolatePos1D(mid_pos, lst_pos, mid_uv, lst_uv, mid_pos_index+interval, interval/2, unit_size);
    }
}

void addNodeToResult(glm::vec2 bl_coord, glm::vec2 tr_coord, glm::vec2 bl_uv, glm::vec2 tr_uv) {
    if (nodeIndex >= maxNodes) {
        return;
    }
    int baseIndex = nodeIndex * dinmension * dinmension;
    glm::vec3 bl_pos = calcPosFromCoord(bl_coord.x, bl_coord.y);
    glm::vec3 br_pos = calcPosFromCoord(bl_coord.x, tr_coord.y);
    glm::vec3 tl_pos = calcPosFromCoord(tr_coord.x, bl_coord.y);
    glm::vec3 tr_pos = calcPosFromCoord(tr_coord.x, tr_coord.y);
    interpolatePos2D(tl_pos, bl_pos, tr_pos, br_pos, bl_uv, tr_uv, baseIndex + dinmension*dinmension/2, dinmension/2);
    interpolatePos1D(tl_pos, tr_pos, glm::vec2(bl_uv.x, tr_uv.y), tr_uv, baseIndex + dinmension/2, dinmension/4, 1);
    interpolatePos1D(tl_pos, bl_pos, glm::vec2(bl_uv.x, tr_uv.y), bl_uv, baseIndex + dinmension/2*dinmension, dinmension/4*dinmension, dinmension);
    result[baseIndex] = tl_pos;
    result[baseIndex + dinmension - 1] = tr_pos;
    result[baseIndex + dinmension * dinmension - 1] = br_pos;
    result[baseIndex + dinmension * (dinmension - 1)] = bl_pos;

    nodeIndex++;
}

void genElementIndex() {
    for (int idx=0; idx<nodeIndex; idx++) {
        int base_ele_index = idx * ele_index_node_size;
        int baseIndex_copy = idx * dinmension * dinmension;
        //printf("ele: %d, idx: %d ~\n", base_ele_index, baseIndex_copy);
        result_index[base_ele_index++] = baseIndex_copy;
        for (int i=0; i<dinmension-1; i++) {
            if (i%2 == 0) {
                for (int j=0; j<dinmension; j++) {
                    result_index[base_ele_index++] = baseIndex_copy;
                    result_index[base_ele_index++] = baseIndex_copy++ + dinmension;
                }
                baseIndex_copy += (dinmension - 1);
            } else {
                for (int j=0; j<dinmension; j++) {
                    result_index[base_ele_index++] = baseIndex_copy;
                    result_index[base_ele_index++] = baseIndex_copy-- + dinmension;
                }
                baseIndex_copy += (dinmension + 1);
            }
            //printf("mid: ele: %d, idx: %d\n", base_ele_index, baseIndex_copy);
        }
        result_index[base_ele_index] = baseIndex_copy;
        //printf("~ ele: %d, idx: %d\n", base_ele_index, baseIndex_copy);
    }
}

float* createLODDEM(int bl_coord_lat, int bl_coord_lng, int scale, int base_index_unit) {
    stringstream ss;
    char ns = bl_coord_lat<0 ? 's' : 'n';
    char ew = bl_coord_lng<0 ? 'w' : 'e';
    if (bl_coord_lng < 0) {
        bl_coord_lng = -bl_coord_lng;
    }
    ss << "/home/ply/projects/opengl/test2/data/Hawaii/" << ns << bl_coord_lat << '_' << ew << bl_coord_lng << "_1arc_v2.tif";
    TIFF *tif = TIFFOpen(ss.str().c_str(), "r");
    if (tif != NULL) {
        cout << "reading: " << ss.str() << " scale: " << scale << " baseIndex: " << base_index_unit << " NumOfStrips: " << TIFFNumberOfStrips(tif) << " stripSize: " << TIFFStripSize(tif)/sizeof(short) << endl;
        uint32 imageW, imageH;
        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imageW);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageH);
        short* buf = (short*)_TIFFmalloc(TIFFStripSize(tif));
        int index = 0;
        for (int strip=TIFFNumberOfStrips(tif)-1; strip>-1; strip-=scale) {
            base_index_unit += texture_unit_size * texture_unit_dinmension;
            TIFFReadEncodedStrip(tif, strip, buf, TIFFStripSize(tif));
            for (int i=0; i<TIFFStripSize(tif)/sizeof(short); i+=scale) {
                texture[base_index_unit + i/scale] = ((float)(short)buf[i])/10000.0f;
            }
        }
        _TIFFfree(buf);
        TIFFClose(tif);
    }
}

glm::vec2 new_texture_unit(glm::vec2 bl_coord, glm::vec2 tr_coord) {
    printf("new texture_unit: %d\n", texture_unit_index);
    if (texture_unit_index >= texture_unit_dinmension * texture_unit_dinmension) {
        texture_unit_index--;
    } else {
        int bl_coord_x_int = (int)bl_coord.x;
        int tr_coord_x_int = (int)tr_coord.x;
        int bl_coord_y_int = (int)bl_coord.y;
        int tr_coord_y_int = (int)tr_coord.y;
        int scale = tr_coord_x_int - bl_coord_x_int;
        for (int i=bl_coord_y_int; i<tr_coord_y_int; i++) {
            for (int j=bl_coord_x_int; j<tr_coord_x_int; j++) {
                int base_index_unit =
                        (texture_unit_index / texture_unit_dinmension) * texture_unit_size * texture_unit_size * texture_unit_dinmension +
                        texture_unit_index % texture_unit_dinmension * texture_unit_size +
                        (j - bl_coord_x_int) * (texture_unit_size/scale) * texture_unit_size * texture_unit_dinmension +
                        (i - bl_coord_y_int) * (texture_unit_size/scale);
                printf("j: %d, i: %d, base_index_unit: %d\n", j, i, base_index_unit);
                createLODDEM(j, i, scale, base_index_unit);
            }
        }
    }
    float delta = 1.0f/(float)texture_unit_dinmension;
    glm::vec2 new_texture_unit_uv_base = glm::vec2(texture_unit_index%texture_unit_dinmension*delta, texture_unit_index/texture_unit_dinmension*delta);
    texture_unit_index++;
    return new_texture_unit_uv_base;
}

void selectNode(glm::vec2 bl_coord, glm::vec2 tr_coord, glm::vec2 bl_uv, glm::vec2 tr_uv, int level) {
    glm::vec3 bl_pos = calcPosFromCoord(bl_coord.x, bl_coord.y);
    glm::vec3 tr_pos = calcPosFromCoord(tr_coord.x, tr_coord.y);
    float nodeSize = glm::length(bl_pos - tr_pos);
    if (nodeSize < minNodeSize) {
        addNodeToResult(bl_coord, tr_coord, bl_uv, tr_uv);
        return;
    }
    if (nodeSize > glm::length((bl_pos + tr_pos)/2.0f - viewPos) || nodeSize > maxNodeSize) {
        //TODO: need optimizing like uv...
        glm::vec2 tl_coord = glm::vec2(tr_coord.x, bl_coord.y);
        glm::vec2 br_coord = glm::vec2(bl_coord.x, tr_coord.y);
        glm::vec2 mid_coord = (bl_coord + tr_coord)/2.0f;
        glm::vec2 mt_coord = (tl_coord + tr_coord)/2.0f;
        glm::vec2 mb_coord = (bl_coord + br_coord)/2.0f;
        glm::vec2 ml_coord = (tl_coord + bl_coord)/2.0f;
        glm::vec2 mr_coord = (tr_coord + br_coord)/2.0f;

        if (level%2 == 0 && tr_coord.x-bl_coord.x>=1.0f && texture_unit_index < texture_units) {//TODO: Height map size def
            bl_uv = new_texture_unit(bl_coord, tr_coord);
            tr_uv = bl_uv + glm::vec2(1.0f/(float)texture_unit_dinmension, 1.0f/(float)texture_unit_dinmension);
            printf("new uv map: (%f, %f) (%f, %f)\n", bl_uv.x, bl_uv.y, tr_uv.x, tr_uv.y);
        }

        glm::vec2 mid_uv = (bl_uv + tr_uv)/2.0f;

        level++;
        selectNode(ml_coord, mt_coord, glm::vec2(bl_uv.x, mid_uv.y), glm::vec2(mid_uv.x, tr_uv.y), level);
        selectNode(bl_coord, mid_coord, bl_uv, mid_uv, level);
        selectNode(mid_coord, tr_coord, mid_uv, tr_uv, level);
        selectNode(mb_coord, mr_coord, glm::vec2(mid_uv.x, bl_uv.y), glm::vec2(tr_uv.x, mid_uv.y), level);
    } else {
        addNodeToResult(bl_coord, tr_coord, bl_uv, tr_uv);
    }
}

void createQuardTree(glm::vec2 bl_coord, glm::vec2 tr_coord, int* index, glm::vec3* result_ret, glm::vec2* result_uv_ret, int* ele_index, unsigned int* result_index_ret, float* texture_array) {
    nodeIndex = *index / dinmension / dinmension;
    result = result_ret;
    result_uv = result_uv_ret;
    result_index = result_index_ret;
    texture = texture_array;
    selectNode(bl_coord, tr_coord, glm::vec2(-1.0f/(float)texture_unit_dinmension, 0.0f), glm::vec2(0.0f, 1.0f/(float)texture_unit_dinmension), 0);
    genElementIndex();
    *index = nodeIndex * dinmension * dinmension;
    *ele_index = nodeIndex * ele_index_node_size;
}

