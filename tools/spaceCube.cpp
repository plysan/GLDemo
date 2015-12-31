#include <stdio.h>
#include <glm/glm.hpp>
#include <sstream>
//#include <iostream>
#include <tiffio.h>

int spacecube_texture_size = 2048;

int getSpaceCubePosLength() {
    return 36;
}

void createSpaceCube(glm::vec3* vertex, int* vertex_offset, glm::vec2* vertex_uv) {
    int index = *vertex_offset;
    float uv_offset = 1.0f/(spacecube_texture_size*2);
    vertex_uv[index] = glm::vec2(0.0f, 1.0f);
    vertex[index++] = glm::vec3(-1.0f,-1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(0.0f, 0.666667f);
    vertex[index++] = glm::vec3(-1.0f,-1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(0.333333f, 0.666667f);
    vertex[index++] = glm::vec3(-1.0f, 1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(1.0f, 1.0f);
    vertex[index++] = glm::vec3(1.0f, 1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(0.666667f, 0.666667f);
    vertex[index++] = glm::vec3(-1.0f,-1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(1.0f, 0.666667f);
    vertex[index++] = glm::vec3(-1.0f, 1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(0.666667f, 0.666667f);
    vertex[index++] = glm::vec3(1.0f,-1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(0.333333f, 0.333333f);
    vertex[index++] = glm::vec3(-1.0f,-1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(0.666667f, 0.333333f);
    vertex[index++] = glm::vec3(1.0f,-1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(1.0f, 1.0f);
    vertex[index++] = glm::vec3(1.0f, 1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(0.666667f, 1.0f);
    vertex[index++] = glm::vec3(1.0f,-1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(0.666667f, 0.666667f);
    vertex[index++] = glm::vec3(-1.0f,-1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(0.0f, 1.0f);
    vertex[index++] = glm::vec3(-1.0f,-1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(0.333333f, 0.666667f);
    vertex[index++] = glm::vec3(-1.0f, 1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(0.333333f, 1.0f);
    vertex[index++] = glm::vec3(-1.0f, 1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(0.666667f, 0.666667f);
    vertex[index++] = glm::vec3(1.0f,-1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(0.333333f, 0.666667f);
    vertex[index++] = glm::vec3(-1.0f,-1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(0.333333f, 0.333333f);
    vertex[index++] = glm::vec3(-1.0f,-1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(1.0f, 0.333333f);
    vertex[index++] = glm::vec3(-1.0f, 1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(1.0f, 0.666667f);
    vertex[index++] = glm::vec3(-1.0f,-1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(0.666667f, 0.666667f);
    vertex[index++] = glm::vec3(1.0f,-1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(0.666667f, 1.0f);
    vertex[index++] = glm::vec3(1.0f, 1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(0.333333f, 0.666667f);
    vertex[index++] = glm::vec3(1.0f,-1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(0.666667f, 0.666667f);
    vertex[index++] = glm::vec3(1.0f, 1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(0.333333f, 0.666667f);
    vertex[index++] = glm::vec3(1.0f,-1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(0.666667f, 1.0f);
    vertex[index++] = glm::vec3(1.0f, 1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(0.333333f, 1.0f);
    vertex[index++] = glm::vec3(1.0f,-1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(0.0f, 0.666667f);
    vertex[index++] = glm::vec3(1.0f, 1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(0.0f, 0.333333f);
    vertex[index++] = glm::vec3(1.0f, 1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(0.333333f, 0.333333f);
    vertex[index++] = glm::vec3(-1.0f, 1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(0.0f, 0.666667f);
    vertex[index++] = glm::vec3(1.0f, 1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(0.333333f, 0.333333f);
    vertex[index++] = glm::vec3(-1.0f, 1.0f,-1.0f);
    vertex_uv[index] = glm::vec2(0.333333f, 0.666667f);
    vertex[index++] = glm::vec3(-1.0f, 1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(0.666667f, 0.333333f);
    vertex[index++] = glm::vec3(1.0f, 1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(1.0f, 0.333333f);
    vertex[index++] = glm::vec3(-1.0f, 1.0f, 1.0f);
    vertex_uv[index] = glm::vec2(0.666667f, 0.666667f);
    vertex[index++] = glm::vec3(1.0f,-1.0f, 1.0f);
    *vertex_offset = index;
}

void fillSpacecubeTexture(glm::detail::uint32* texture_data) {
    std::stringstream ss;
    ss << "/home/ply/projects/opengl/test2/data/space.tif";
    TIFF *tif = TIFFOpen(ss.str().c_str(), "r");
    if (tif != NULL) {
        uint32 imageW, imageH;
        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imageW);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageH);
        float scale_width = (float)imageW / spacecube_texture_size;
        float scale_height = (float)imageH / spacecube_texture_size;
        uint32* buf = (uint32*)_TIFFmalloc(imageW * sizeof(uint32));
        for(int i=0; i<spacecube_texture_size; i++) {
            TIFFReadRGBAStrip(tif, (int)((spacecube_texture_size-1-i)*scale_height), buf);
            for(int j=0; j<spacecube_texture_size; j++) {
                uint32 color = buf[(int)(j*scale_width)];
                texture_data[i*spacecube_texture_size + j] = color<<24&0xff000000 | color<<8&0xff0000 | color>>8&0xff00 | color>>24&0xff;;
            }
        }
    }
}
