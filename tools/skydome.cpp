#include <stdio.h>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <math.h>
#include <GL/glew.h>
#include "vars.hpp"
#include "tools.hpp"
#include "quardtreeTerrain.hpp"

int g_circular_count = 0;
int g_radius_count = 0;

int getSkydomePosLength() {
   return g_circular_count * g_radius_count;
}

int getSkydomeElementLength() {
    return (g_circular_count+1) * 2 * (g_radius_count-1) + 2;
}

void updateSkydomeConf(int circular_count, int radius_count) {
    g_circular_count = circular_count;
    g_radius_count = radius_count;
}

glm::vec3 calcMDSkyPosFromCoord(float lat_offset, float lng_offset, float lat_origin, float lng_origin) {
    glm::vec3 pos = glm::vec3(
        (double)atmosphere_top_radius * std::cos(lat_offset) * std::cos(lng_offset),
        (double)atmosphere_top_radius * std::sin(lat_offset),
        (double)-atmosphere_top_radius * std::cos(lat_offset) * std::sin(lng_offset)
    );
    pos = glm::rotate(pos, 90.0f-lat_origin, glm::vec3(0.0f, 0.0f, -1.0f));
    pos = glm::rotate(pos, lng_origin, glm::vec3(0.0f, 1.0f, 0.0f));
    pos -= vertex_offset;
    return pos;
}

void createSkydome(glm::vec3* vertex, int* vertex_offset, unsigned int* element_index, int* element_index_offset, glm::vec2* vertex_uv, glm::vec2 view_coord, int circular_count, float radius_range, int radius_count) {
    int vertex_offset_init = *vertex_offset;
    int pointer = vertex_offset_init;
    float circle_interval = 2*pi/circular_count;
    float radius_interval = radius_range/radius_count;
    glm::vec2 zero_color_uv = glm::vec2(0.99f, 0.99f);
    for (int i=0; i<radius_count; i++) {
        for (int j=0; j<circular_count; j++) {
            vertex[pointer] = calcMDSkyPosFromCoord(pi/2 - radius_interval*i, circle_interval*j, view_coord.x, view_coord.y);
            vertex_uv[pointer++] = zero_color_uv;
        }
    }
    *vertex_offset = pointer;

    pointer = vertex_offset_init;
    int element_ptr = *element_index_offset;
    element_index[element_ptr++] = pointer;
    for (int i=0; i<radius_count-1; i++) {
        for (int j=0; j<circular_count; j++) {
            element_index[element_ptr++] = pointer + j + circular_count*i;
            element_index[element_ptr] = element_index[element_ptr-1] + circular_count;
            element_ptr++;
        }
        element_index[element_ptr++] = pointer + circular_count*i;
        element_index[element_ptr] = element_index[element_ptr-1] + circular_count;
        element_ptr++;
    }
    element_index[element_ptr] = element_index[element_ptr-1];
    *element_index_offset = element_ptr;
}

/*
 * CIE 1931 color matching functions data
 * obtained from http://www.cvrl.org/database/data/cmfs/ciexyzjv.csv
 */
int cie_matrix_length = 45;
float cie_matrix[][4] = {
    {0.38f, 0.0026899000f, 0.00020000000f, 0.01226000000000f},
    //{0.385f, 0.0053105000f, 0.00039556000f, 0.02422200000000f},
    {0.39f, 0.0107810000f, 0.00080000000f, 0.04925000000000f},
    //{0.395f, 0.0207920000f, 0.00154570000f, 0.09513500000000f},
    {0.40f, 0.0379810000f, 0.00280000000f, 0.17409000000000f},
    //{0.405f, 0.0631570000f, 0.00465620000f, 0.29013000000000f},
    {0.41f, 0.0999410000f, 0.00740000000f, 0.46053000000000f},
    //{0.415f, 0.1582400000f, 0.01177900000f, 0.73166000000000f},
    {0.42f, 0.2294800000f, 0.01750000000f, 1.06580000000000f},
    //{0.425f, 0.2810800000f, 0.02267800000f, 1.31460000000000f},
    {0.43f, 0.3109500000f, 0.02730000000f, 1.46720000000000f},
    //{0.435f, 0.3307200000f, 0.03258400000f, 1.57960000000000f},
    {0.44f, 0.3333600000f, 0.03790000000f, 1.61660000000000f},
    //{0.445f, 0.3167200000f, 0.04239100000f, 1.56820000000000f},
    {0.45f, 0.2888200000f, 0.04680000000f, 1.47170000000000f},
    //{0.455f, 0.2596900000f, 0.05212200000f, 1.37400000000000f},
    {0.46f, 0.2327600000f, 0.06000000000f, 1.29170000000000f},
    //{0.465f, 0.2099900000f, 0.07294200000f, 1.23560000000000f},
    {0.47f, 0.1747600000f, 0.09098000000f, 1.11380000000000f},
    //{0.475f, 0.1328700000f, 0.11284000000f, 0.94220000000000f},
    {0.48f, 0.0919440000f, 0.13902000000f, 0.75596000000000f},
    //{0.485f, 0.0569850000f, 0.16987000000f, 0.58640000000000f},
    {0.49f, 0.0317310000f, 0.20802000000f, 0.44669000000000f},
    //{0.495f, 0.0146130000f, 0.25808000000f, 0.34116000000000f},
    {0.50f, 0.0048491000f, 0.32300000000f, 0.26437000000000f},
    //{0.505f, 0.0023215000f, 0.40540000000f, 0.20594000000000f},
    {0.51f, 0.0092899000f, 0.50300000000f, 0.15445000000000f},
    //{0.515f, 0.0292780000f, 0.60811000000f, 0.10918000000000f},
    {0.52f, 0.0637910000f, 0.71000000000f, 0.07658500000000f},
    //{0.525f, 0.1108100000f, 0.79510000000f, 0.05622700000000f},
    {0.53f, 0.1669200000f, 0.86200000000f, 0.04136600000000f},
    //{0.535f, 0.2276800000f, 0.91505000000f, 0.02935300000000f},
    {0.54f, 0.2926900000f, 0.95400000000f, 0.02004200000000f},
    //{0.545f, 0.3622500000f, 0.98004000000f, 0.01331200000000f},
    {0.55f, 0.4363500000f, 0.99495000000f, 0.00878230000000f},
    //{0.555f, 0.5151300000f, 1.00010000000f, 0.00585730000000f},
    {0.56f, 0.5974800000f, 0.99500000000f, 0.00404930000000f},
    //{0.565f, 0.6812100000f, 0.97875000000f, 0.00292170000000f},
    {0.57f, 0.7642500000f, 0.95200000000f, 0.00227710000000f},
    //{0.575f, 0.8439400000f, 0.91558000000f, 0.00197060000000f},
    {0.58f, 0.9163500000f, 0.87000000000f, 0.00180660000000f},
    //{0.585f, 0.9770300000f, 0.81623000000f, 0.00154490000000f},
    {0.59f, 1.0230000000f, 0.75700000000f, 0.00123480000000f},
    //{0.595f, 1.0513000000f, 0.69483000000f, 0.00111770000000f},
    {0.60f, 1.0550000000f, 0.63100000000f, 0.00090564000000f},
    //{0.605f, 1.0362000000f, 0.56654000000f, 0.00069467000000f},
    {0.61f, 0.9923900000f, 0.50300000000f, 0.00042885000000f},
    //{0.615f, 0.9286100000f, 0.44172000000f, 0.00031817000000f},
    {0.62f, 0.8434600000f, 0.38100000000f, 0.00025598000000f},
    //{0.625f, 0.7398300000f, 0.32052000000f, 0.00015679000000f},
    {0.63f, 0.6328900000f, 0.26500000000f, 0.00009769400000f},
    //{0.635f, 0.5335100000f, 0.21702000000f, 0.00006894400000f},
    {0.64f, 0.4406200000f, 0.17500000000f, 0.00005116500000f},
    //{0.645f, 0.3545300000f, 0.13812000000f, 0.00003601600000f},
    {0.65f, 0.2786200000f, 0.10700000000f, 0.00002423800000f},
    //{0.655f, 0.2148500000f, 0.08165200000f, 0.00001691500000f},
    {0.66f, 0.1616100000f, 0.06100000000f, 0.00001190600000f},
    //{0.665f, 0.1182000000f, 0.04432700000f, 0.00000814890000f},
    {0.67f, 0.0857530000f, 0.03200000000f, 0.00000560060000f},
    //{0.675f, 0.0630770000f, 0.02345400000f, 0.00000395440000f},
    {0.68f, 0.0458340000f, 0.01700000000f, 0.00000279120000f},
    //{0.685f, 0.0320570000f, 0.01187200000f, 0.00000191760000f},
    {0.69f, 0.0221870000f, 0.00821000000f, 0.00000131350000f},
    //{0.695f, 0.0156120000f, 0.00577230000f, 0.00000091519000f},
    {0.70f, 0.0110980000f, 0.00410200000f, 0.00000064767000f},
    //{0.705f, 0.0079233000f, 0.00292910000f, 0.00000046352000f},
    {0.71f, 0.0056531000f, 0.00209100000f, 0.00000033304000f},
    //{0.715f, 0.0040039000f, 0.00148220000f, 0.00000023823000f},
    {0.72f, 0.0028253000f, 0.00104700000f, 0.00000017026000f},
    //{0.725f, 0.0019947000f, 0.00074015000f, 0.00000012207000f},
    {0.73f, 0.0013994000f, 0.00052000000f, 0.00000008710700f},
    //{0.735f, 0.0009698000f, 0.00036093000f, 0.00000006145500f},
    {0.74f, 0.0006684700f, 0.00024920000f, 0.00000004316200f},
    //{0.745f, 0.0004614100f, 0.00017231000f, 0.00000003037900f},
    {0.75f, 0.0003207300f, 0.00012000000f, 0.00000002155400f},
    //{0.755f, 0.0002257300f, 0.00008462000f, 0.00000001549300f},
    {0.76f, 0.0001597300f, 0.00006000000f, 0.00000001120400f},
    //{0.765f, 0.0001127500f, 0.00004244600f, 0.00000000808730f},
    {0.77f, 0.0000795130f, 0.00003000000f, 0.00000000583400f},
    //{0.775f, 0.0000560870f, 0.00002121000f, 0.00000000421100f},
    {0.78f, 0.0000395410f, 0.00001498900f, 0.00000000303830f},
    //{0.785f, 0.0000278520f, 0.00001058400f, 0.00000000219070f},
    {0.79f, 0.0000195970f, 0.00000746560f, 0.00000000157780f},
    //{0.795f, 0.0000137700f, 0.00000525920f, 0.00000000113480f},
    {0.80f, 0.0000096700f, 0.00000370280f, 0.00000000081565f},
    //{0.805f, 0.0000067918f, 0.00000260760f, 0.00000000058626f},
    {0.81f, 0.0000047706f, 0.00000183650f, 0.00000000042138f},
    //{0.815f, 0.0000033550f, 0.00000129500f, 0.00000000030319f},
    {0.82f, 0.0000023534f, 0.00000091092f, 0.00000000021753f},
    //{0.825f, 0.0000016377f, 0.00000063564f, 0.00000000015476f}
};

/*
 * cieXYZ to sRGB space matrix with reference white of D50.
 * data obtained from http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
 */
float ciexyz2srgb_data[9] = {3.1338561f, -0.9787684f, 0.0719453f, -1.6168667f, 1.9161415f, -0.2289914f, -0.4906146f, 0.0334540f, 1.4052427f};
glm::mat3 ciexyz2srgb_matrix = glm::make_mat3(ciexyz2srgb_data);

float intensity_integral_unit = 0.4f;
float attenuation_integral_unit = 0.4f;
float height_0_rayleigh = 0.8f;
// decrease/increase to be more/less blue
float intensity_integral_precision_maintain_coefficient = 0.03f;
float integral_ciexyz_clamp_coefficient = 1.0f/91.0f;

float height_coefficient(glm::vec3 height_pos) {
    float height = glm::length(height_pos)-earth_radius;
    if(height<0.0f)height = 0.0f;
    return pow(euler, -height/height_0_rayleigh);
}

float view_to_scatter_attenuation_integrating_cache = 0.0f;

int count = 0;

float integrantOneScatterIntensity(float lambda, glm::vec3 view_pos, glm::vec3 scatter_pos, glm::vec3 sun_dir_normal, glm::vec3 view_dir) {
    view_to_scatter_attenuation_integrating_cache += height_coefficient(scatter_pos);
    float integral_attenuation = view_to_scatter_attenuation_integrating_cache;
    glm::vec3 sun_dir_unit = sun_dir_normal * attenuation_integral_unit;
    glm::vec3 scatter_to_sun_integrant_pos = scatter_pos + sun_dir_unit;
    float scatter_to_sun_integrant_pos_height = glm::length(scatter_to_sun_integrant_pos);
    while(scatter_to_sun_integrant_pos_height < atmosphere_top_radius){
        if(scatter_to_sun_integrant_pos_height < earth_radius){
            return 0.0f;
        }
        integral_attenuation += height_coefficient(scatter_to_sun_integrant_pos);
        scatter_to_sun_integrant_pos += sun_dir_unit;
        scatter_to_sun_integrant_pos_height = glm::length(scatter_to_sun_integrant_pos);
    }
    float scatter_angle_cos = glm::dot(view_dir, sun_dir_normal);
    /*
     * intensity of one light beam from the sun to the viewer with one-time scatter inside atmosphere.
     * intensity = air refraction factor * optical depth * Rayleigh phase function / light's wave length^4 * air molecular density
     */
    return pow(euler, -integral_attenuation * attenuation_integral_unit * intensity_integral_precision_maintain_coefficient / pow(lambda, 4))
            * (1 + pow(scatter_angle_cos, 2))/pow(lambda, 4) * height_coefficient(scatter_pos);
}

float calculateColorLambda(float lambda, float height, float view_angle, float sun_angle_vertical, float sun_angle_horizontal) {
    view_to_scatter_attenuation_integrating_cache = 0.0f;
    float integral_intensity = 0.0f;
    glm::vec3 view_pos = calcFPosFromCoord(90.0f, 0.0f) + glm::vec3(0.0f, height, 0.0f);
    glm::vec3 integrating_dir_unit = glm::normalize(calcFPosFromCoord(90.0f-view_angle, 0.0f)) * intensity_integral_unit;
    glm::vec3 integrant_intensity_pos = view_pos + integrating_dir_unit;
    glm::vec3 sun_dir_normal = glm::normalize(calcFPosFromCoord(90.0f-sun_angle_vertical, sun_angle_horizontal));
    float integrant_intensity_pos_height = glm::length(integrant_intensity_pos);
    while(integrant_intensity_pos_height < atmosphere_top_radius
            && integrant_intensity_pos_height > earth_radius){
        integral_intensity += integrantOneScatterIntensity(lambda, view_pos, integrant_intensity_pos, sun_dir_normal, integrating_dir_unit);
        integrant_intensity_pos += integrating_dir_unit;
        integrant_intensity_pos_height = glm::length(integrant_intensity_pos);
    }
    // air refraction formula referenced from http://refractiveindex.info
    float air_refraction = 0.05792105f/(238.0185f-pow(lambda, -2)) + 0.00167917f/(57.362f-pow(lambda, -2)) + 1.0f;
    return integral_intensity * (pow(pow(air_refraction, 2)-1, 2) * 1000000.0f) * intensity_integral_unit;
}

// adjust the power value according to your monitor
float gamma_correction(float linear) {
    if (linear > 0.0f) {
        return pow(linear, (1.0f/1.2f));
    }
    return linear;
}

float max_intensity = 0.0f;

glm::detail::uint32 calculateColor(float height, float view_angle, float sun_angle_vertical, float sun_angle_horizontal) {
    glm::vec3 integral_ciexyz = glm::vec3(0.0f);
    for (int i=0; i<cie_matrix_length; i++) {
        integral_ciexyz.x += calculateColorLambda(cie_matrix[i][0], height, view_angle, sun_angle_vertical, sun_angle_horizontal) * cie_matrix[i][1];
        integral_ciexyz.y += calculateColorLambda(cie_matrix[i][0], height, view_angle, sun_angle_vertical, sun_angle_horizontal) * cie_matrix[i][2];
        integral_ciexyz.z += calculateColorLambda(cie_matrix[i][0], height, view_angle, sun_angle_vertical, sun_angle_horizontal) * cie_matrix[i][3];
    }

    if(integral_ciexyz.x > max_intensity)max_intensity = integral_ciexyz.x;
    if(integral_ciexyz.y > max_intensity)max_intensity = integral_ciexyz.y;
    if(integral_ciexyz.z > max_intensity)max_intensity = integral_ciexyz.z;

    glm::vec3 color_srgb_linear = ciexyz2srgb_matrix * (integral_ciexyz * integral_ciexyz_clamp_coefficient);
    glm::vec3 color_srgb = glm::vec3(gamma_correction(color_srgb_linear.x), gamma_correction(color_srgb_linear.y), gamma_correction(color_srgb_linear.z));
    int red = (int)(color_srgb.x * 255);
    int green = (int)(color_srgb.y * 255);
    int blue = (int)(color_srgb.z * 255);

    // clip the srgb range that out of 0~255
    if(red>255)red=255;
    if(red<0)red=0;
    if(green>255)green=255;
    if(green<0)green=0;
    if(blue>255)blue=255;
    if(blue<0)blue=0;

    printf("%3d%3d%3d|", red, green, blue);
    return red<<24&0xff000000 | green<<16&0x00ff0000 | blue<<8&0x0000ff00 | 255&0x000000ff;
}

void fillScatterTexture(glm::detail::uint32* scatter_texture_array_data, int scatter_texture_3d_size, int scatter_texture_4thd_in_3d_size) {
    float atmosphere_top_height = atmosphere_top_radius - earth_radius - 0.1f;
    int texture_size_exp_2 = scatter_texture_3d_size * scatter_texture_4thd_in_3d_size * scatter_texture_3d_size * scatter_texture_4thd_in_3d_size;
    int texture_size = scatter_texture_3d_size * scatter_texture_4thd_in_3d_size;
    int scatter_texture_4thd_size = pow(scatter_texture_4thd_in_3d_size, 3);
    // height: 0 -> atmosphere_top_height
    for (int i=0; i<scatter_texture_4thd_size; i++) {
        //if(i > 2)continue;
        int xy = i % (scatter_texture_4thd_in_3d_size * scatter_texture_4thd_in_3d_size);
        int x = xy%scatter_texture_4thd_in_3d_size * scatter_texture_3d_size;
        int y = xy/scatter_texture_4thd_in_3d_size * scatter_texture_3d_size;
        int z = i/(scatter_texture_4thd_in_3d_size*scatter_texture_4thd_in_3d_size) * scatter_texture_3d_size;
        float height = atmosphere_top_height * (float)i/(scatter_texture_4thd_size-1);
        // view angle: 0 -> 180
        for(int j=0; j<scatter_texture_3d_size; j++) {
            float view_angle = 180.0f * (float)j/(scatter_texture_3d_size-1);
            // sun angle vertical: 0 -> 180
            for(int k=0; k<scatter_texture_3d_size; k++) {
                float sun_angle_vertical = 180.0f * (float)k/(scatter_texture_3d_size-1);
                // sun angle horizontal: 0 -> 180
                for(int l=0; l<scatter_texture_3d_size; l++) {
                    float sun_angle_horizontal = 180.0f * (float)l/(scatter_texture_3d_size-1);
                    scatter_texture_array_data[(j+z)*texture_size_exp_2 + (k+y)*texture_size + l+x] = calculateColor(height, view_angle, sun_angle_vertical, sun_angle_horizontal);
                }
                printf("\n");
            }
            printf(".....................\n");
        }
        printf("-----------------------------------------^%d\n", i);
    }
    printf("max_intensity: %f\n", max_intensity);
}
