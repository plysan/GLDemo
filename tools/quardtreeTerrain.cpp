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
#include <string.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <tiffio.h>
#include <glm/glm.hpp>
#include <glm/gtx/constants.hpp>
#include <GL/glew.h>
#include "quardtreeTerrain.hpp"
#include "vars.hpp"
#include "tools.hpp"

//TODO tools should not access directly
extern glm::vec3 dbuf_view_offset;

QTProfile::QTProfile(
        int dinmension,
        int maxNodes,
        float minNodeSize,
        float maxNodeSize,
        int texture_unit_size,
        int texture_unit_dinmension,
        int lod_max):
        dinmension(dinmension),
        maxNodes(maxNodes),
        minNodeSize(minNodeSize),
        maxNodeSize(maxNodeSize),
        texture_unit_size(texture_unit_size),
        texture_unit_dinmension(texture_unit_dinmension),
        lod_max(lod_max) {
    vertex_offset = glm::vec3();
    elevation_divisor = earth_radius * 10000.0f;
    // make sure malloc is enough for one strip
    buf = _TIFFmalloc(262144L);
    def_dem = TIFFOpen("assets/def_dem.tif", "r");
    def_img = TIFFOpen("assets/def_img.tif", "r");
    updateValues();
}

QTProfile::~QTProfile() {
    _TIFFfree(buf);
    TIFFClose(def_dem);
    TIFFClose(def_img);
}

void QTProfile::updateValues() {
    ele_index_node_size = 2 * dinmension * (dinmension - 1) + 2;
    vertexBufferSize = dinmension*dinmension*maxNodes;
    ele_index_size = maxNodes*ele_index_node_size;
    one_degree_lat_length = glm::length(calcFPosFromCoord(0.0f, 0.0f) - calcFPosFromCoord(1.0f, 0.0f));
    terrain_texture_size = texture_unit_size*texture_unit_dinmension;
    terrain_texture_units = texture_unit_dinmension * texture_unit_dinmension - 1;
    quardtree_pos_length = maxNodes * dinmension * dinmension;
    quardtree_uv_length = maxNodes * dinmension * dinmension;
    quardtree_element_index_length = maxNodes * ele_index_node_size;
    quardtree_normal_length = maxNodes * dinmension * dinmension;
    terrain_texture_length = terrain_texture_size*terrain_texture_size;
}

glm::dvec3 QTProfile::calcMDTerrainPosFromCoord(double lat, double lng) {
    return glm::dvec3(
        (double)earth_radius * std::cos(lat) * std::cos(lng) - vertex_offset.x,
        (double)earth_radius * std::sin(lat) - vertex_offset.y,
        (double)-earth_radius * std::cos(lat) * std::sin(lng) - vertex_offset.z
    );
}

void QTProfile::interpolatePos2D(glm::dvec2 bl_coord, glm::dvec2 tr_coord, glm::vec2 bl_uv, glm::vec2 tr_uv, int mid_pos_index, int unit_size) {
    glm::dvec2 mt_coord = glm::dvec2(tr_coord.x, (bl_coord.y+tr_coord.y)/2.0);
    glm::dvec2 mb_coord = glm::dvec2(bl_coord.x, (bl_coord.y+tr_coord.y)/2.0);
    glm::dvec2 ml_coord = glm::dvec2((bl_coord.x+tr_coord.x)/2.0, bl_coord.y);
    glm::dvec2 mr_coord = glm::dvec2((bl_coord.x+tr_coord.x)/2.0, tr_coord.y);
    glm::dvec2 mid_coord = (bl_coord + tr_coord)/2.0;
    result[mid_pos_index] = calcMDTerrainPosFromCoord(mid_coord.x, mid_coord.y);
    result[mid_pos_index + unit_size] = calcMDTerrainPosFromCoord(mr_coord.x, mr_coord.y);
    result[mid_pos_index + dinmension*unit_size] = calcMDTerrainPosFromCoord(mb_coord.x, mb_coord.y);

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
        interpolatePos2D(ml_coord, mt_coord, ml_uv, mt_uv, mid_pos_index - dinmension*sub_unit_size - sub_unit_size, sub_unit_size);
        interpolatePos2D(mid_coord, tr_coord, mid_uv, tr_uv, mid_pos_index - dinmension*sub_unit_size + sub_unit_size, sub_unit_size);
        interpolatePos2D(bl_coord, mid_coord, bl_uv, mid_uv, mid_pos_index + dinmension*sub_unit_size - sub_unit_size, sub_unit_size);
        interpolatePos2D(mb_coord, mr_coord, mb_uv, mr_uv, mid_pos_index + dinmension*sub_unit_size + sub_unit_size, sub_unit_size);
    }
}

void QTProfile::interpolatePos1D(glm::dvec2 frst_coord, glm::dvec2 lst_coord, glm::vec2 frst_uv, glm::vec2 lst_uv, int mid_pos_index, int interval, int unit_size) {
    glm::dvec2 mid_coord = (frst_coord + lst_coord)/2.0;
    result[mid_pos_index] = calcMDTerrainPosFromCoord(mid_coord.x, mid_coord.y);
    glm::vec2 mid_uv = (frst_uv + lst_uv)/2.0f;
    result_uv[mid_pos_index] = mid_uv;
    if (interval >= unit_size) {
        interpolatePos1D(frst_coord, mid_coord, frst_uv, mid_uv, mid_pos_index-interval, interval/2, unit_size);
        interpolatePos1D(mid_coord, lst_coord, mid_uv, lst_uv, mid_pos_index+interval, interval/2, unit_size);
    }
}

void QTProfile::elevationOffset(glm::vec3 *result, double elevation_factor) {
    result->x = ((double)result->x + (double)vertex_offset.x) * elevation_factor - (double)vertex_offset.x;
    result->y = ((double)result->y + (double)vertex_offset.y) * elevation_factor - (double)vertex_offset.y;
    result->z = ((double)result->z + (double)vertex_offset.z) * elevation_factor - (double)vertex_offset.z;
}

/*
 * Convention:
 *  only use two types of image file: global image, and 1 degree scale image.
 *  image file is in tiff format, with no compression, 1 row per strip, and can have any size.
 */
void QTProfile::readDATAFromIMG(glm::vec2 node_bl_coord, glm::vec2 node_span, int data_base_index, const char* img_suffix) {
    int node_dinmension;
    if (strcmp(img_suffix, "dem") == 0) {
        node_dinmension = dinmension;
    } else if (strcmp(img_suffix, "img") == 0) {
        node_dinmension = texture_unit_size;
    } else {
        return;
    }
    bool negative_lat = node_bl_coord.x < 0.0f;
    bool negative_lng = node_bl_coord.y < 0.0f;
    char ns = negative_lat ? 's' : 'n';
    char ew = negative_lng ? 'w' : 'e';
    float node_bl_coord_img_name_lat = (int)std::abs(node_bl_coord.x);
    float node_bl_coord_img_name_lng = (int)std::abs(node_bl_coord.y);
    float node_detail_offset_lat = node_bl_coord.x - (float)(int)node_bl_coord.x;
    if (node_detail_offset_lat < 0.0f) {
        node_detail_offset_lat += 1.0f;
        node_bl_coord_img_name_lat++;
    }
    float node_detail_offset_lng = node_bl_coord.y - (float)(int)node_bl_coord.y;
    if (node_detail_offset_lng < 0.0f) {
        node_detail_offset_lng += 1.0f;
        node_bl_coord_img_name_lng++;
    }
    int node_span_int_lat = std::max((int)node_span.x, 1);
    int node_span_int_lng = std::max((int)node_span.y, 1);
    std::stringstream ss;
    for (int lat_offset = 0; lat_offset<node_span_int_lat; lat_offset++) {
        for (int lng_offset = 0; lng_offset<node_span_int_lng; lng_offset++) {
            // node corner coordinates in the coordinate system which target image size is 1 unit and bl corner of target image is origin
            double begin_node_lat, end_node_lat, delta_node_lat, begin_node_lng, end_node_lng, delta_node_lng;
            // clamp range of discrete points in the node that target image is overlapping
            double min_lat, max_lat, min_lng, max_lng;
            ss.str("");
            ss << "assets/" << ns << (negative_lat?node_bl_coord_img_name_lat-lat_offset:node_bl_coord_img_name_lat+lat_offset)
                    << '_' << ew << (negative_lng?node_bl_coord_img_name_lng-lng_offset:node_bl_coord_img_name_lng+lng_offset)
                    << "_" << img_suffix << ".tif";
            TIFF *tif = TIFFOpen(ss.str().c_str(), "r");
            bool close_img = true;
            if (tif != NULL) {
                begin_node_lat = node_detail_offset_lat - lat_offset;
                end_node_lat = begin_node_lat + node_span.x;
                delta_node_lat = (end_node_lat - begin_node_lat) / (node_dinmension-1);
                begin_node_lng = node_detail_offset_lng - lng_offset;
                end_node_lng = begin_node_lng + node_span.y;
                delta_node_lng = (end_node_lng - begin_node_lng) / (node_dinmension-1);
                min_lat = begin_node_lat + lat_offset + delta_node_lat/2;
                max_lat = min_lat + std::min(node_span.x, 1.0f);
                if (lat_offset==0) min_lat -= delta_node_lat;
                min_lng = begin_node_lng + lng_offset + delta_node_lng/2;
                max_lng = min_lng + std::min(node_span.y, 1.0f);
                if (lng_offset==0) min_lng -= delta_node_lng;
                min_lat = begin_node_lat + std::ceil((min_lat-begin_node_lat)/delta_node_lat) * delta_node_lat;
                min_lng = begin_node_lng + std::ceil((min_lng-begin_node_lng)/delta_node_lng) * delta_node_lng;
            } else {
                if (strcmp(img_suffix, "dem") == 0) {
                    tif = def_dem;
                } else if (strcmp(img_suffix, "img") == 0) {
                    tif = def_img;
                }
                if (tif != NULL) {
                    begin_node_lat = (90.0 + node_bl_coord.x) / 180.0;
                    end_node_lat = begin_node_lat + node_span.x/180.0;
                    delta_node_lat = (end_node_lat - begin_node_lat) / (node_dinmension-1);
                    begin_node_lng = (180.0 + node_bl_coord.y) / 360.0;
                    end_node_lng = begin_node_lng + node_span.y/360.0;
                    delta_node_lng = (end_node_lng - begin_node_lng) / (node_dinmension-1);
                    min_lat = begin_node_lat + lat_offset/180.0 + delta_node_lat/2;
                    max_lat = min_lat + std::min(node_span.x, 1.0f)/180.0;
                    if (lat_offset==0) min_lat -= delta_node_lat;
                    min_lng = begin_node_lng + lng_offset/360.0 + delta_node_lng/2;
                    max_lng = min_lng + std::min(node_span.y, 1.0f)/360.0;
                    if (lng_offset==0) min_lng -= delta_node_lng;
                    min_lat = begin_node_lat + std::ceil((min_lat-begin_node_lat)/delta_node_lat) * delta_node_lat;
                    min_lng = begin_node_lng + std::ceil((min_lng-begin_node_lng)/delta_node_lng) * delta_node_lng;
                } else {
                    return;
                }
                close_img = false;
            }
            if (tif != NULL) {
                // dem file dinmension should have 1 more pixel
                uint32 image_w, image_h;
                TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &image_w);
                TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &image_h);
                image_w--;
                image_h--;
                if (strcmp(img_suffix, "dem") == 0) {
                    short* buf_dem = (short*)buf;
                    for(double i=min_lat; i<max_lat; i+=delta_node_lat) {
                        int write_index_lat = data_base_index + (int)std::round((end_node_lat-i)/delta_node_lat) * dinmension;
                        TIFFReadEncodedStrip(tif, std::max(0, (int)std::round((1.0-i) * image_h)), buf_dem, TIFFStripSize(tif));
                        for(double j=min_lng; j<max_lng; j+=delta_node_lng) {
                            int write_index_lng = write_index_lat + (int)std::round((j-begin_node_lng)/delta_node_lng);
                            double elevation_factor = ((double)(short)buf_dem[(int)std::round(j * image_w)])/elevation_divisor + 1.0f;
                            elevationOffset(&result[write_index_lng], elevation_factor);
                        }
                    }
                    if (close_img) TIFFClose(tif);
                } else if (strcmp(img_suffix, "img") == 0) {
                    uint32* buf_img = (uint32*)buf;
                    for(double i=min_lat; i<max_lat; i+=delta_node_lat) {
                        int write_index_lat = data_base_index + (int)std::round((i-begin_node_lat)/delta_node_lat) * texture_unit_size * texture_unit_dinmension;
                        TIFFReadRGBAStrip(tif, std::max(0, (int)std::round((1.0-i) * image_h)), buf_img);
                        for(double j=min_lng; j<max_lng; j+=delta_node_lng) {
                            int write_index_lng = write_index_lat + (int)std::round((j-begin_node_lng)/delta_node_lng);
                            uint32 color = buf_img[(int)std::round(j * image_w)];
                            texture[write_index_lng] = color<<24&0xff000000 | color<<8&0xff0000 | color>>8&0xff00 | color>>24&0xff;
                        }
                    }
                    if (close_img) TIFFClose(tif);
                }
            }
        }
    }
}

bool QTProfile::addNodeToResult(glm::vec2 bl_coord, glm::vec2 tr_coord, glm::vec2 bl_uv, glm::vec2 tr_uv) {
    if (nodeIndex >= maxNodes) {
        return false;
    }
    int baseIndex = vertex_index_offset + nodeIndex * dinmension * dinmension;
    glm::dvec2 bl_coord_arc = glm::dvec2((double)bl_coord.x/180*pi, (double)bl_coord.y/180*pi);
    glm::dvec2 tr_coord_arc = glm::dvec2((double)tr_coord.x/180*pi, (double)tr_coord.y/180*pi);
    glm::dvec3 bl_pos = calcMDTerrainPosFromCoord(bl_coord_arc.x, bl_coord_arc.y);
    glm::dvec3 br_pos = calcMDTerrainPosFromCoord(bl_coord_arc.x, tr_coord_arc.y);
    glm::dvec3 tl_pos = calcMDTerrainPosFromCoord(tr_coord_arc.x, bl_coord_arc.y);
    glm::dvec3 tr_pos = calcMDTerrainPosFromCoord(tr_coord_arc.x, tr_coord_arc.y);
    interpolatePos2D(bl_coord_arc, tr_coord_arc, bl_uv, tr_uv, baseIndex + dinmension*dinmension/2, dinmension/2);
    interpolatePos1D(glm::dvec2(tr_coord_arc.x, bl_coord_arc.y), tr_coord_arc, glm::vec2(bl_uv.x, tr_uv.y), tr_uv, baseIndex + dinmension/2, dinmension/4, 1);
    interpolatePos1D(glm::dvec2(tr_coord_arc.x, bl_coord_arc.y), bl_coord_arc, glm::vec2(bl_uv.x, tr_uv.y), bl_uv, baseIndex + dinmension/2*dinmension, dinmension/4*dinmension, dinmension);
    result[baseIndex] = tl_pos;
    result_uv[baseIndex] = glm::vec2(bl_uv.x, tr_uv.y);
    result[baseIndex + dinmension - 1] = tr_pos;
    result_uv[baseIndex + dinmension - 1] = tr_uv;
    result[baseIndex + dinmension * dinmension - 1] = br_pos;
    result_uv[baseIndex + dinmension * dinmension - 1] = glm::vec2(tr_uv.x, bl_uv.y);
    result[baseIndex + dinmension * (dinmension - 1)] = bl_pos;
    result_uv[baseIndex + dinmension * (dinmension - 1)] = bl_uv;

    readDATAFromIMG(bl_coord, glm::vec2(tr_coord.x-bl_coord.x, tr_coord.y-bl_coord.y), vertex_index_offset + nodeIndex * dinmension * dinmension, "dem");
    nodeIndex++;
    return true;
}

float QTProfile::calResultNormalwithRoughness(int ele_index, int offset_a, int offset_b, bool add_roughness) {
    int result_index_p = result_index[ele_index];
    result_normal[result_index_p] = glm::normalize(glm::cross(
        result[result_index[ele_index+offset_a]] - result[result_index_p],
        result[result_index[ele_index+offset_b]] - result[result_index_p]));
    if (add_roughness) {
        return glm::length(glm::cross(result_normal[result_index_p], result_normal[result_index[ele_index-1]]));
    } else {
        return 0.0f;
    }
}

float QTProfile::genNodeElementNormalwithRoughness(int idx) {
    float roughness = 0.0f;
    int base_ele_index = element_index_offset + idx * ele_index_node_size;
    int baseIndex_copy = vertex_index_offset + idx * dinmension * dinmension;
    result_index[base_ele_index++] = baseIndex_copy;
    for (int i=0; i<dinmension; i++) {
        if (i%2 == 0) {
            int normal_index_strip = base_ele_index;
            if (i == dinmension - 1) {
                // fill the last row of node with normal data
                normal_index_strip--;
                for (int i=0; i<dinmension-1; i++) {
                    roughness += calResultNormalwithRoughness(normal_index_strip, -2, -1, true);
                    normal_index_strip -= 2;
                }
                roughness += calResultNormalwithRoughness(normal_index_strip, -1, 1, true);
            } else {
                for (int j=0; j<dinmension; j++) {
                    result_index[base_ele_index++] = baseIndex_copy;
                    result_index[base_ele_index++] = baseIndex_copy++ + dinmension;
                }
                baseIndex_copy += (dinmension - 1);

                // skip roughness for this one
                calResultNormalwithRoughness(normal_index_strip, 1, 2, false);
                normal_index_strip++;
                for (int i=0; i<dinmension*2-2; i++) {
                    if (i%2 == 0) {
                        roughness += calResultNormalwithRoughness(normal_index_strip, 1, -1, true);
                        normal_index_strip++;
                    } else {
                        roughness += calResultNormalwithRoughness(normal_index_strip, -1, 1, true);
                        normal_index_strip++;
                    }
                }
                roughness += calResultNormalwithRoughness(normal_index_strip, -1, -2, true);
            }
        } else {
            for (int j=0; j<dinmension; j++) {
                result_index[base_ele_index++] = baseIndex_copy;
                result_index[base_ele_index++] = baseIndex_copy-- + dinmension;
            }
            baseIndex_copy += (dinmension + 1);
        }
    }
    result_index[base_ele_index] = baseIndex_copy;
    return roughness;
}

glm::vec2 QTProfile::getNewUv() {
    float delta = 1.0f/(float)texture_unit_dinmension;
    glm::vec2 new_texture_unit_uv_base = glm::vec2(texture_unit_index%texture_unit_dinmension*delta, texture_unit_index/texture_unit_dinmension*delta);
    texture_unit_index++;
    return new_texture_unit_uv_base;
}

int QTProfile::getTextureIndex() {
    return (texture_unit_index / texture_unit_dinmension) * texture_unit_size * texture_unit_size * texture_unit_dinmension +
        texture_unit_index % texture_unit_dinmension * texture_unit_size;
}

glm::vec2 QTProfile::new_texture_unit(glm::vec2 bl_coord, glm::vec2 tr_coord) {
    if (texture_unit_index >= terrain_texture_units) {
        texture_unit_index--;
    } else {
        readDATAFromIMG(bl_coord, glm::vec2(tr_coord.x-bl_coord.x, tr_coord.y-bl_coord.y), getTextureIndex(), "img");
    }
    return getNewUv();
}

void QTProfile::selectNode(glm::vec2 bl_coord, glm::vec2 tr_coord, glm::vec2 bl_uv, glm::vec2 tr_uv, int level, Node** node, glm::vec3* viewing_pos) {
    if (tr_coord.x-bl_coord.x==0 || tr_coord.y-bl_coord.y==0) {
        delete *node;
        *node = NULL;
        return;
    }
    (*node)->bl = NULL;
    (*node)->br = NULL;
    (*node)->tl = NULL;
    (*node)->tr = NULL;
    (*node)->start_index = vertex_index_offset + nodeIndex * dinmension * dinmension;
    (*node)->node_size_lat = tr_coord.x - bl_coord.x;
    (*node)->node_size_lng = tr_coord.y - bl_coord.y;
    glm::vec2 mid_coord;
    if (((int)(tr_coord.x-bl_coord.x))>1 || ((int)(tr_coord.y-bl_coord.y))>1) {
        mid_coord = glm::vec2(bl_coord.x + ((int)(tr_coord.x-bl_coord.x))/2, bl_coord.y + ((int)(tr_coord.y-bl_coord.y))/2);
    } else {
        mid_coord = (bl_coord + tr_coord)/2.0f;
    }
    (*node)->lat_mid = mid_coord.x;
    (*node)->lng_mid = mid_coord.y;
    (*node)->lat_bl = bl_coord.x;
    (*node)->lng_bl = bl_coord.y;

    glm::vec3 bl_pos = calcFPosFromCoord(bl_coord.x, bl_coord.y);
    glm::vec3 tr_pos = calcFPosFromCoord(tr_coord.x, tr_coord.y);
    float one_degree_lng_length = glm::length(calcFPosFromCoord(mid_coord.x, 0.0f) - calcFPosFromCoord(mid_coord.x, 1.0f));
    float node_size = sqrt(pow((*node)->node_size_lat*one_degree_lat_length, 2) + pow((*node)->node_size_lng*one_degree_lng_length, 2));
    if (!addNodeToResult(bl_coord, tr_coord, bl_uv, tr_uv)) {
        delete *node;
        *node = NULL;
        return;
    }
    // leave this var for future use
    float roughness = genNodeElementNormalwithRoughness(nodeIndex-1);
    if (node_size < minNodeSize) {
        return;
    }
    glm::vec3 viewer_pos = vertex_offset - dbuf_view_offset + *viewing_pos;
    float view_node_mid_distance = glm::length(calcFPosFromCoord(mid_coord.x, mid_coord.y) - viewer_pos);
    float view_height_sealevel = glm::abs(glm::length(viewer_pos) - earth_radius) + 0.5f; // less sensitive to lod change near sealevel
    float view_node_mid_distance_horizontal = sqrt(glm::max(pow(view_node_mid_distance, 2) - pow(view_height_sealevel, 2), 0.0));
    float node_view_size_arc = atan((view_node_mid_distance_horizontal+node_size*0.708f)/view_height_sealevel) // 0.708 ~ 2^0.5/2
        - atan((view_node_mid_distance_horizontal-node_size*0.708f)/view_height_sealevel);
    float node_area = (tr_coord.x-bl_coord.x)*(tr_coord.y-bl_coord.y);
    if (roughness>(pow(view_height_sealevel,2)*0.001f+0.1f)/node_area && node_view_size_arc > 0.15f //TODO param configurable
            || node_size > maxNodeSize) {
        nodeIndex--;
        glm::vec2 tl_coord = glm::vec2(tr_coord.x, bl_coord.y);
        glm::vec2 br_coord = glm::vec2(bl_coord.x, tr_coord.y);
        glm::vec2 mt_coord = glm::vec2(tr_coord.x, mid_coord.y);
        glm::vec2 mb_coord = glm::vec2(bl_coord.x, mid_coord.y);
        glm::vec2 ml_coord = glm::vec2(mid_coord.x, bl_coord.y);
        glm::vec2 mr_coord = glm::vec2(mid_coord.x, tr_coord.y);

        if (texture_unit_index < terrain_texture_units) {//TODO: Height map size def
            float delta_coord = tr_coord.x-bl_coord.x;
            glm::vec2 temp_bl_uv = new_texture_unit(bl_coord, tr_coord);
            if (temp_bl_uv.x != -10.0f) {
                bl_uv = temp_bl_uv;
                tr_uv = bl_uv + glm::vec2(1.0f/(float)texture_unit_dinmension, 1.0f/(float)texture_unit_dinmension);
            }
        }

        glm::vec2 mid_uv = glm::vec2();
        mid_uv.x = bl_uv.x + (tr_uv.x-bl_uv.x)*(mid_coord.y-bl_coord.y)/(tr_coord.y-bl_coord.y);
        mid_uv.y = bl_uv.y + (tr_uv.y-bl_uv.y)*(mid_coord.x-bl_coord.x)/(tr_coord.x-bl_coord.x);

        level++;

        (*node)->tl = new Node;
        selectNode(ml_coord, mt_coord, glm::vec2(bl_uv.x, mid_uv.y), glm::vec2(mid_uv.x, tr_uv.y), level, &((*node)->tl), viewing_pos);
        (*node)->bl = new Node;
        selectNode(bl_coord, mid_coord, bl_uv, mid_uv, level, &((*node)->bl), viewing_pos);
        (*node)->tr = new Node;
        selectNode(mid_coord, tr_coord, mid_uv, tr_uv, level, &((*node)->tr), viewing_pos);
        (*node)->br = new Node;
        selectNode(mb_coord, mr_coord, glm::vec2(mid_uv.x, bl_uv.y), glm::vec2(tr_uv.x, mid_uv.y), level, &((*node)->br), viewing_pos);
    }
}

void QTProfile::createQuardTree(
        glm::vec2 bl_coord,
        glm::vec2 tr_coord,
        int* vertex_index,
        glm::vec3* result_ret,
        glm::vec2* result_uv_ret,
        glm::vec3* result_normal_ret,
        int* ele_index,
        unsigned int* result_index_ret,
        uint32* texture_array,
        Node** new_node,
        glm::vec3* viewing_pos) {
    TIFFSetWarningHandler(NULL);
    TIFFSetErrorHandler(NULL);
    vertex_index_offset = *vertex_index;
    element_index_offset = *ele_index;
    nodeIndex = 0;
    texture_unit_index = 0;
    result = result_ret;
    result_uv = result_uv_ret;
    result_index = result_index_ret;
    result_normal = result_normal_ret;
    texture = texture_array;
    float delta = 1.0f/(float)texture_unit_dinmension;
    glm::vec2 delta_coord = glm::vec2(delta, delta);
    if(tr_coord.y > bl_coord.y) {
        readDATAFromIMG(bl_coord, glm::vec2(tr_coord.x-bl_coord.x, tr_coord.y-bl_coord.y), getTextureIndex(), "img");
        texture_unit_index++;
        selectNode(bl_coord, tr_coord, glm::vec2(0.0f, 0.0f), delta_coord, 0, new_node, viewing_pos);
    } else {
        (*new_node)->lat_mid = 90.0f;
        (*new_node)->lng_mid = 0.0f;
        (*new_node)->bl = new Node;
        (*new_node)->tl = NULL;
        (*new_node)->br = new Node;
        (*new_node)->tr = NULL;
        glm::vec2 tr_coord_mid = glm::vec2(tr_coord.x, 180.0f);
        glm::vec2 bl_coord_mid = glm::vec2(bl_coord.x, -180.0f);
        readDATAFromIMG(bl_coord, glm::vec2(tr_coord_mid.x-bl_coord.x, tr_coord_mid.y-bl_coord.y), getTextureIndex(), "img");
        glm::vec2 uv0 = getNewUv();
        selectNode(bl_coord, tr_coord_mid, uv0, uv0+delta_coord, 0, &((*new_node)->br), viewing_pos);
        readDATAFromIMG(bl_coord_mid, glm::vec2(tr_coord.x-bl_coord_mid.x, tr_coord.y-bl_coord_mid.y), getTextureIndex(), "img");
        glm::vec2 uv1 = getNewUv();
        selectNode(bl_coord_mid, tr_coord, uv1, uv1+delta_coord, 0, &((*new_node)->bl), viewing_pos);
    }
    *vertex_index += nodeIndex * dinmension * dinmension;
    *ele_index += nodeIndex * ele_index_node_size;
}

void QTProfile::cleanupNode(Node** node) {
    if (*node == NULL) {
        return;
    }
    Node* pointer = (*node)->tl;
    cleanupNode(&pointer);
    pointer = (*node)->tr;
    cleanupNode(&pointer);
    pointer = (*node)->bl;
    cleanupNode(&pointer);
    pointer = (*node)->br;
    cleanupNode(&pointer);
    delete *node;
}

glm::vec3 QTProfile::getResultNormalFromCoord(glm::vec2 coord, glm::vec3* using_buffer_data, int* rIndex) {
    Node* target_node = node;
    Node* target_node_suspect = node;
    while (target_node_suspect != NULL) {
        target_node = target_node_suspect;
        if (target_node->lat_mid > coord.x) {
            if (target_node->lng_mid > coord.y) {
                target_node_suspect = target_node->bl;
            } else {
                target_node_suspect = target_node->br;
            }
        } else {
            if (target_node->lng_mid > coord.y) {
                target_node_suspect = target_node->tl;
            } else {
                target_node_suspect = target_node->tr;
            }
        }
    }
    float lat_offset;
    if(target_node->lat_bl < coord.x) {
        lat_offset = target_node->lat_bl + target_node->node_size_lat - coord.x;
    } else {
        lat_offset = target_node->node_size_lat*0.99999f; // make it not reach the bondary
    }
    float lng_offset = coord.y - target_node->lng_bl;
    if(lng_offset == target_node->node_size_lng)lng_offset *= 0.99999f; // make it not reach the bondary
    float normal_index_row_offset = (lat_offset/target_node->node_size_lat * (float)(dinmension - 1));
    float normal_index_column_offset = (lng_offset/target_node->node_size_lng * (float)(dinmension - 1));
    int normal_index_offset = (int)normal_index_row_offset * dinmension + (int)normal_index_column_offset;
    float lat_deviation = normal_index_row_offset - (float)(int)normal_index_row_offset;
    float lng_deviation = normal_index_column_offset - (float)(int)normal_index_column_offset;
    int index = target_node->start_index + normal_index_offset;
    *rIndex = index;
    if ((int)normal_index_row_offset%2 == 0) {
        if (lat_deviation + lng_deviation > 1.0f) {
            index++;
            *rIndex = index;
            return glm::normalize(glm::cross(using_buffer_data[index+dinmension-1] - using_buffer_data[index], using_buffer_data[index+dinmension] - using_buffer_data[index]));
        } else {
            return glm::normalize(glm::cross(using_buffer_data[index+dinmension] - using_buffer_data[index], using_buffer_data[index+1] - using_buffer_data[index]));
        }
    } else {
        if (1.0f - lat_deviation + lng_deviation > 1.0f) {
            return glm::normalize(glm::cross(using_buffer_data[index+dinmension+1] - using_buffer_data[index], using_buffer_data[index+1] - using_buffer_data[index]));
        } else {
            return glm::normalize(glm::cross(using_buffer_data[index+dinmension] - using_buffer_data[index], using_buffer_data[index+dinmension+1] - using_buffer_data[index]));
        }
    }
}

