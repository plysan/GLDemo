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
#include <iomanip>
#include <sstream>
#include <tiffio.h>
#include <glm/glm.hpp>
#include <glm/gtx/constants.hpp>
#include <GL/glew.h>
#include "quardtreeTerrain.hpp"
#include "vars.hpp"
#include "tools.hpp"

extern glm::vec3 vertex_offset_diff;

QTProfile::QTProfile(
        int dinmension,
        int maxNodes,
        float minNodeSize,
        float maxNodeSize,
        int texture_unit_size,
        int texture_unit_size_dem,
        int texture_unit_dinmension,
        int lod_max):
        dinmension(dinmension),
        maxNodes(maxNodes),
        minNodeSize(minNodeSize),
        maxNodeSize(maxNodeSize),
        texture_unit_size(texture_unit_size),
        texture_unit_size_dem(texture_unit_size_dem),
        texture_unit_dinmension(texture_unit_dinmension),
        lod_max(lod_max) {
    vertex_offset = glm::vec3();
    elevation_divisor = earth_radius * 10000.0f;
    updateValues();
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

void QTProfile::addNodeToResult(glm::vec2 bl_coord, glm::vec2 tr_coord, glm::vec2 bl_uv, glm::vec2 tr_uv, Node** node) {
    if (nodeIndex >= maxNodes) {
        *node = NULL;
        return;
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

    int coords_spaned_x = (int)(tr_coord.x - bl_coord.x);
    int coords_spaned_y = (int)(tr_coord.y - bl_coord.y);
    if (coords_spaned_x == 0) {
        float bl_coord_lat_texture_offset = 1.0f - (bl_coord.x - (float)(int)bl_coord.x) - (tr_coord.x - bl_coord.x);
        float bl_coord_lng_texture_offset = bl_coord.y - (float)(int)bl_coord.y;
        if (bl_coord_lng_texture_offset < 0.0f) {
            bl_coord_lng_texture_offset += 1.0f;
        }
        char ns = bl_coord.x<0.0f ? 's' : 'n';
        char ew = bl_coord.y<0.0f ? 'w' : 'e';
        std::stringstream ss;
        ss << "assets/" << ns << (int)bl_coord.x << '_' << ew << (bl_coord.y < 0.0f && bl_coord_lng_texture_offset > 0.0001f ? -(int)bl_coord.y+1 : -(int)bl_coord.y) << "_1arc_v2.tif";
        TIFF *tif = TIFFOpen(ss.str().c_str(), "r");
        if (tif != NULL) {
            short* buf = (short*)_TIFFmalloc(TIFFStripSize(tif));
            int base_index = baseIndex;
            for (int i=0; i<dinmension; i++) {
                TIFFReadEncodedStrip(tif, (int)((bl_coord_lat_texture_offset + (float)i/((float)dinmension - 1.0f) * (tr_coord.x - bl_coord.x)) * (float)texture_unit_size_dem), buf, TIFFStripSize(tif));
                for (int j=0; j<dinmension; j++) {
                    double elevation_factor = (((double)(short)buf[(int)((bl_coord_lng_texture_offset + (double)j/((double)dinmension - 1.0f) * (tr_coord.y - bl_coord.y)) * (double)texture_unit_size_dem)])/elevation_divisor + 1.0f);
                    elevationOffset(&result[base_index++], elevation_factor);
                }
            }
            _TIFFfree(buf);
            TIFFClose(tif);
        }
    } else {
        // dinmension covers points on both edges of node, so we need to -1
        float scale_x = (float)texture_unit_size_dem/((float)(dinmension-1)/coords_spaned_x);
        float scale_y = (float)texture_unit_size_dem/((float)(dinmension-1)/coords_spaned_y);
        for (int i=(int)bl_coord.x; i<(int)tr_coord.x; i++) {
            for (int j=(int)bl_coord.y; j<(int)tr_coord.y; j++) {
                //TODO lat/lng organization in node array ?
                int base_index_unit = vertex_index_offset + nodeIndex * dinmension * dinmension + dinmension/coords_spaned_x*(coords_spaned_x-1-i+(int)bl_coord.x)*dinmension + dinmension/coords_spaned_y*(j-(int)bl_coord.y);
                int bl_coord_lat = i;
                int bl_coord_lng = j;
                std::stringstream ss;
                char ns = bl_coord_lat<0 ? 's' : 'n';
                char ew = bl_coord_lng<0 ? 'w' : 'e';
                if (bl_coord_lng < 0) {
                    bl_coord_lng = -bl_coord_lng;
                }
                ss << "assets/" << ns << bl_coord_lat << '_' << ew << bl_coord_lng << "_1arc_v2.tif";
                TIFF *tif = TIFFOpen(ss.str().c_str(), "r");
                if (tif != NULL) {
                    uint32 imageW, imageH;
                    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imageW);
                    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageH);
                    short* buf = (short*)_TIFFmalloc(TIFFStripSize(tif));
                    int last_index_strip = -1;
                    for (float strip=0.0f; strip<(float)TIFFNumberOfStrips(tif); strip+=scale_x) {
                        if ((int)(strip/scale_x) != last_index_strip) {
                            TIFFReadEncodedStrip(tif, (int)strip, buf, TIFFStripSize(tif));
                            last_index_strip = (int)(strip/scale_x);
                        } else {
                            continue;
                        }
                        elevationOffset(&result[base_index_unit], (((double)(short)buf[0])/elevation_divisor + 1.0f));
                        int last_index_row = base_index_unit;
                        for (float k=scale_y; k<((float)(TIFFStripSize(tif)))/(float)sizeof(short); k+=scale_y) {
                            if (base_index_unit + (int)(k/scale_y) != last_index_row) {
                                // The index of result cannot be plus one in every loop, but cast from float, why?
                                elevationOffset(&result[base_index_unit + (int)(k/scale_y)], (((double)(short)buf[(int)(k)])/elevation_divisor + 1.0f));
                                last_index_row = base_index_unit + (int)(k/scale_y);
                            }
                        }
                        base_index_unit += dinmension;
                    }
                    //end of row
                    _TIFFfree(buf);
                    TIFFClose(tif);
                }
            }
        }
    }

    nodeIndex++;
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

bool QTProfile::getImageFromCoords(TIFF** tif, glm::vec2* image_bl_coord, float* span_image_coord, glm::vec2 bl_coord, glm::vec2 tr_coord) {
    std::stringstream ss;
    float coord_span = tr_coord.x - bl_coord.x;
    float coord_span_lod = 1.0f/lod_max;
    while(coord_span > coord_span_lod) {
        coord_span_lod *= 2.0f;
    }
    glm::vec2 bl_coord_int = glm::vec2((float)(int)bl_coord.x, (float)(int)bl_coord.y);
    if(bl_coord_int.x < 0.0f)bl_coord_int.x -= 1.0f;
    if(bl_coord_int.y < 0.0f)bl_coord_int.y -= 1.0f;
    *tif = NULL;
    glm::vec2 bl_coord_image;
    while(*tif == NULL && coord_span_lod <= 1.0f) {
        bl_coord_image = bl_coord_int;
        while(bl_coord_image.x>bl_coord.x || bl_coord_image.x+coord_span_lod<tr_coord.x) {
            bl_coord_image.x += coord_span_lod;
        }
        while(bl_coord_image.y>bl_coord.y || bl_coord_image.y+coord_span_lod<tr_coord.y) {
            bl_coord_image.y += coord_span_lod;
        }
        ss.str("");
        if(coord_span_lod == 1.0f) {
            ss << "assets/"
                    << (bl_coord_image.x<0.0f ? 's' : 'n') << glm::abs(bl_coord_image.x) << '_'
                    << (bl_coord_image.y<0.0f ? 'w' : 'e') << glm::abs(bl_coord_image.y) << "_img.tif";;
        } else {
            glm::vec2 tr_coord_index = glm::vec2(bl_coord_image.x+coord_span_lod, bl_coord_image.y+coord_span_lod);
            ss << "assets/"
                    << (bl_coord_image.x<0.0f ? 's' : 'n') << glm::abs(bl_coord_image.x) << '_'
                    << (bl_coord_image.y<0.0f ? 'w' : 'e') << glm::abs(bl_coord_image.y) << '~'
                    << (tr_coord_index.x<0.0f ? 's' : 'n') << glm::abs(tr_coord_index.x) << '_'
                    << (tr_coord_index.y<0.0f ? 'w' : 'e') << glm::abs(tr_coord_index.y) << "_img.tif";
        }
        *tif = TIFFOpen(ss.str().c_str(), "r");
        if(*tif == NULL)coord_span_lod *= 2.0f;
    }
    if(*tif != NULL)*image_bl_coord = bl_coord_image;
    *span_image_coord = coord_span_lod;
    return *tif != NULL;
}

bool QTProfile::readImageToTexture(glm::vec2 bl_coord, glm::vec2 tr_coord, int scale_x, int scale_y, int base_index_unit) {
    bool created = false;
    std::stringstream ss;
    ss << std::setprecision(std::numeric_limits<float>::digits10+1);
    char ns = bl_coord.x<0.0f ? 's' : 'n';
    char ew = bl_coord.y<0.0f ? 'w' : 'e';
    bool detailed = tr_coord.x - bl_coord.x < 1.0f;
    TIFF *tif;
    if (!detailed) {
        ss << "assets/"
                << ns << glm::abs(bl_coord.x) << '_' << ew << glm::abs(bl_coord.y) << "_img.tif";
        tif = TIFFOpen(ss.str().c_str(), "r");
        if (tif != NULL) {
            created = true;
            uint32 image_w, image_h;
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &image_w);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &image_h);
            float scale_lat = image_h/(texture_unit_size/(float)scale_y);
            float scale_lng = image_w/(texture_unit_size/(float)scale_x);
            uint32* buf = (uint32*)_TIFFmalloc(image_w * sizeof(uint32));
            for (float strip=image_h-1; strip>=0.0f; strip-=scale_lat) {
                TIFFReadRGBAStrip(tif, (int)strip, buf);
                int row_index = 0;
                for (float i=0; i<image_w; i+=scale_lng) {
                    uint32 color = buf[(int)i];
                    texture[base_index_unit + row_index] = color<<24&0xff000000 | color<<8&0xff0000 | color>>8&0xff00 | color>>24&0xff;
                    row_index++;
                }
                base_index_unit += texture_unit_size * texture_unit_dinmension;
            }
            _TIFFfree(buf);
            TIFFClose(tif);
        }
    } else {
        glm::vec2 mid_coord = (bl_coord+tr_coord)/2.0f;
        int bl_image_lat = (int)mid_coord.x;
        int bl_image_lng = glm::abs(((int)mid_coord.y)-1);
        ss.str("");
        glm::vec2 image_bl_coord;
        float span_image_coord;
        if(getImageFromCoords(&tif, &image_bl_coord, &span_image_coord, bl_coord, tr_coord)) {
            uint32 image_w, image_h;
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &image_w);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &image_h);
            float image_usage_percent = (tr_coord.x-bl_coord.x)/span_image_coord;
            if(image_usage_percent*image_h*1.5f > texture_unit_size || image_usage_percent*image_w*1.5f > texture_unit_size) {
                created = true;
                uint32* buf = (uint32*)_TIFFmalloc(image_w * sizeof(uint32));
                float scale_lat = image_h*image_usage_percent/texture_unit_size;
                float scale_lng = image_w*image_usage_percent/texture_unit_size;
                float offset_lat = (span_image_coord-(bl_coord.x-image_bl_coord.x))/span_image_coord*image_h;
                float offset_lng = (bl_coord.y-image_bl_coord.y)/span_image_coord*image_w;
                for (int i=0; i<texture_unit_size; i++) {
                    TIFFReadRGBAStrip(tif, (int)offset_lat, buf);
                    offset_lat -= scale_lat;
                    float offset = offset_lng;
                    for (int j=0; j<texture_unit_size; j++) {
                        uint32 color = buf[(int)offset];
                        offset += scale_lng;
                        texture[base_index_unit + j] = color<<24&0xff000000 | color<<8&0xff0000 | color>>8&0xff00 | color>>24&0xff;
                    }
                    base_index_unit += texture_unit_size * texture_unit_dinmension;
                }
                _TIFFfree(buf);
            }
            TIFFClose(tif);
        }
    }
    return created;
}

bool QTProfile::readGlobalImageToTexture(glm::vec2 bl_coord, glm::vec2 tr_coord) {
    std::stringstream ss;
    ss << "assets/def_img.tif";
    TIFF *tif = TIFFOpen(ss.str().c_str(), "r");
    if (tif != NULL) {
        int base_index_unit = (texture_unit_index / texture_unit_dinmension) * texture_unit_size * texture_unit_size * texture_unit_dinmension
                + texture_unit_index % texture_unit_dinmension * texture_unit_size;
        uint32 imageW, imageH;
        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imageW);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageH);
        float scale_lat = (tr_coord.x-bl_coord.x)/180.0f*imageH/texture_unit_size;
        float scale_lng = (tr_coord.y-bl_coord.y)/360.0f*imageW/texture_unit_size;
        int begin_index_img_lat = (int)((90.0f-bl_coord.x)/180.0f*imageH);
        int begin_index_img_lng = (int)((bl_coord.y+180.0f)/360.0f*imageW);
        uint32* buf = (uint32*)_TIFFmalloc(imageW * sizeof(uint32));
        float strip = begin_index_img_lat;
        for (int i=0; i<texture_unit_size; i++) {
            TIFFReadRGBAStrip(tif, (int)strip, buf);
            strip -= scale_lat;
            float strip_index = begin_index_img_lng;
            for (int j=0; j<texture_unit_size; j++) {
                uint32 color = buf[(int)strip_index];
                strip_index += scale_lng;
                texture[base_index_unit + j] = color<<24&0xff000000 | color<<8&0xff0000 | color>>8&0xff00 | color>>24&0xff;
            }
            base_index_unit += texture_unit_size * texture_unit_dinmension;
        }
        _TIFFfree(buf);
        TIFFClose(tif);
    }
}

glm::vec2 QTProfile::getNewUv() {
    float delta = 1.0f/(float)texture_unit_dinmension;
    glm::vec2 new_texture_unit_uv_base = glm::vec2(texture_unit_index%texture_unit_dinmension*delta, texture_unit_index/texture_unit_dinmension*delta);
    texture_unit_index++;
    return new_texture_unit_uv_base;
}

glm::vec2 QTProfile::new_texture_unit(glm::vec2 bl_coord, glm::vec2 tr_coord, bool detailed) {
    if (texture_unit_index >= terrain_texture_units) {
        texture_unit_index--;
    } else if (!detailed) {
        readGlobalImageToTexture(bl_coord, tr_coord);
        int bl_coord_x_int = (int)bl_coord.x;
        int tr_coord_x_int = (int)tr_coord.x;
        int bl_coord_y_int = (int)bl_coord.y;
        int tr_coord_y_int = (int)tr_coord.y;
        int scale_y = tr_coord_x_int - bl_coord_x_int;
        int scale_x = tr_coord_y_int - bl_coord_y_int;
        for (int i=bl_coord_y_int; i<tr_coord_y_int; i++) {
            for (int j=bl_coord_x_int; j<tr_coord_x_int; j++) {
                int base_index_unit =
                        (texture_unit_index / texture_unit_dinmension) * texture_unit_size * texture_unit_size * texture_unit_dinmension +
                        texture_unit_index % texture_unit_dinmension * texture_unit_size +
                        (int)((j - bl_coord_x_int) * ((float)texture_unit_size/scale_y)) * texture_unit_size * texture_unit_dinmension +
                        (int)((i - bl_coord_y_int) * ((float)texture_unit_size/scale_x));
                readImageToTexture(glm::vec2(j, i), glm::vec2(j+1, i+1), scale_x, scale_y, base_index_unit);
            }
        }
    } else {
        int base_index_unit = (texture_unit_index / texture_unit_dinmension) * texture_unit_size * texture_unit_size * texture_unit_dinmension +
                texture_unit_index % texture_unit_dinmension * texture_unit_size;
        if (!readImageToTexture(bl_coord, tr_coord, 1, 1, base_index_unit)) {
            return glm::vec2(-10.0f, -10.0f);
        }
    }
    return getNewUv();
}

void QTProfile::selectNode(glm::vec2 bl_coord, glm::vec2 tr_coord, glm::vec2 bl_uv, glm::vec2 tr_uv, int level, Node** node, glm::vec3* viewing_pos) {
    if (tr_coord.x-bl_coord.x==0 || tr_coord.y-bl_coord.y==0) {
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
    addNodeToResult(bl_coord, tr_coord, bl_uv, tr_uv, node);
    // leave this var for future use
    float roughness = genNodeElementNormalwithRoughness(nodeIndex-1);
    if (node_size < minNodeSize) {
        return;
    }
    glm::vec3 viewer_pos = vertex_offset - vertex_offset_diff + *viewing_pos;
    float view_node_mid_distance = glm::length(calcFPosFromCoord(mid_coord.x, mid_coord.y) - viewer_pos);
    float view_height_sealevel = glm::abs(glm::length(viewer_pos) - earth_radius) + 0.01f;
    float view_node_mid_distance_horizontal = sqrt(pow(view_node_mid_distance, 2) - pow(view_height_sealevel, 2));
    float node_view_size_arc = atan((view_node_mid_distance_horizontal+node_size/2)/view_height_sealevel)
            - atan((view_node_mid_distance_horizontal-node_size/2)/view_height_sealevel);
    if (view_node_mid_distance < node_size
            || node_view_size_arc > 0.174532925f // ~ 10 degrees
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
            bool detailed = delta_coord < 1.0f;
            glm::vec2 temp_bl_uv = new_texture_unit(bl_coord, tr_coord, detailed);
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
        readGlobalImageToTexture(bl_coord, tr_coord);
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
        readGlobalImageToTexture(bl_coord, tr_coord_mid);
        glm::vec2 uv0 = getNewUv();
        selectNode(bl_coord, tr_coord_mid, uv0, uv0+delta_coord, 0, &((*new_node)->br), viewing_pos);
        readGlobalImageToTexture(bl_coord_mid, tr_coord);
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

