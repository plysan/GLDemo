layout(location = 0) in vec3 vertex_pos_ms;
layout(location = 1) in vec2 vertex_uv_ms;
layout(location = 2) in vec3 vertex_normal_ms;
layout(location = 3) in vec3 vertex_water_ss;
layout(location = 4) in vec3 vertex_water_uv_ss; //reserve

out vec3 vertex_ws;
out vec3 vertex_normal_cs;
out vec3 vertex_origin_dir_cs;
out vec3 vertex_sun_dir_cs;
out vec2 uv;
out vec2 water_uv;
noperspective out vec4 scatter_rgba;

uniform mat4 MVP;
uniform mat4 M;
uniform mat4 V;
uniform mat4 inv_P;
uniform mat4 notrans_VP;
uniform mat4 inv_V;
uniform vec3 cam_ms;
uniform vec3 sun_ws;
uniform vec3 vertex_offset_ws;
uniform float scatter_height;
uniform int render_target;
uniform sampler3D scatter_texture_sampler;
uniform sampler2D water_texture_sampler;
uniform float water_ac;
uniform float cam_height;
uniform vec2 water_offset_uv;
uniform vec2 coord_length;
uniform vec2 water_origin_coord;

float atmosphere_radius_square = atmosphere_top_radius * atmosphere_top_radius;
float earth_radius_square = earth_radius * earth_radius;
float atmosphere_thickness = atmosphere_top_radius - earth_radius;
float scatter_texture_size = scatter_texture_3d_size * scatter_texture_4thd_in_3d_size;
float scatter_texture_scale_factor = float(scatter_texture_3d_size - 1) / (scatter_texture_size);
float view_angle_factor = float(scatter_texture_size - 1) / scatter_texture_size;
float scatter_texture_offset = 1.0 / (scatter_texture_size * 2);
int scatter_texture_4thd_size_sub_1 = int(pow(float(scatter_texture_4thd_in_3d_size), 2)) - 1;
float green_pow_coefficient = 1 / pow(green_avg_wlength/red_avg_wlength, 4);
float blue_pow_coefficient = 1 / pow(blue_avg_wlength/red_avg_wlength, 4);

vec4 getScatterAngles(vec3 up_n_cs, vec3 origin_vertex_dir_n_cs, vec3 sun_n_cs, float height_coord_linear, float height_sealevel){
    float view_angle_horizon_cos = -(sqrt(pow(earth_radius+height_sealevel, 2) - pow(earth_radius, 2)) / (earth_radius+height_sealevel));
    float scatter_view_angle_cos = dot(up_n_cs, origin_vertex_dir_n_cs);
    float scatter_view_angle_coord;
    if (scatter_view_angle_cos > view_angle_horizon_cos) {
        scatter_view_angle_coord = (0.5+pow((scatter_view_angle_cos-view_angle_horizon_cos)/(1-view_angle_horizon_cos), 0.2)*0.5) * view_angle_factor + scatter_texture_offset;
    } else {
        scatter_view_angle_coord = (0.5-pow((view_angle_horizon_cos-scatter_view_angle_cos)/(1+view_angle_horizon_cos), 0.2)*0.5) * view_angle_factor + scatter_texture_offset;
    }
    float scatter_sun_angle_vertical = (dot(up_n_cs, sun_n_cs)+1)/2 * scatter_texture_scale_factor + scatter_texture_offset;
    float scatter_sun_angle_horizontal = (dot(cross(up_n_cs, origin_vertex_dir_n_cs), cross(up_n_cs, sun_n_cs))+1)/2 * scatter_texture_scale_factor + scatter_texture_offset;
    return vec4(scatter_sun_angle_horizontal, scatter_sun_angle_vertical, scatter_view_angle_coord, pow(height_coord_linear, 0.5));
}

vec4 texture4D(vec4 scatter_params)
{
    int height_unormalize = int(scatter_params.w * scatter_texture_4thd_size_sub_1);
    float height_middle_offset = scatter_params.w * scatter_texture_4thd_size_sub_1 - height_unormalize;
    float y = float(height_unormalize / scatter_texture_4thd_in_3d_size) / scatter_texture_4thd_in_3d_size;
    float x = float(height_unormalize % scatter_texture_4thd_in_3d_size) / scatter_texture_4thd_in_3d_size;
    vec4 color_0 = texture(scatter_texture_sampler, vec3(scatter_params.x+x, scatter_params.y+y, scatter_params.z));
    height_unormalize++;
    y = float(height_unormalize / scatter_texture_4thd_in_3d_size) / scatter_texture_4thd_in_3d_size;
    x = float(height_unormalize % scatter_texture_4thd_in_3d_size) / scatter_texture_4thd_in_3d_size;
    vec4 color_1 = texture(scatter_texture_sampler, vec3(scatter_params.x+x, scatter_params.y+y, scatter_params.z));
    vec4 result = color_0 * (1.0 - height_middle_offset) + color_1 * height_middle_offset;
    result.rgb = (result.rgb - vec3(color_srgb_offset))/color_srgb_coefficient;
    return result;
}

void main() {

    if (render_target == 3) {
        vec3 cam_vertex_dir_ms = normalize((inv_V * vec4((inv_P * vec4(vertex_water_ss, 1)).xyz, 0)).xyz);
        vec3 cam_earthCenter_dir = -(cam_ms + vertex_offset_ws);
        float cam_earthCenter_length = length(cam_earthCenter_dir);
        vec3 cam_earthCenter_dir_n = normalize(cam_earthCenter_dir);
        float earthCenter_cam_vertex_cos = dot(cam_earthCenter_dir_n, cam_vertex_dir_ms);
        float b = 2 * cam_earthCenter_length * earthCenter_cam_vertex_cos;
        float bb_4ac = pow(b, 2) - 4 * water_ac;
        float cam_vertex_length;
        vec3 surface_pos_ms;
        if(bb_4ac < 0 || b < 0.0000001) {
            gl_Position = MVP * vec4(cam_earthCenter_dir, 1);
            return;
        } else {
            cam_vertex_length = (b - sqrt(bb_4ac)) / 2;
            if (cam_vertex_length < 0.2) {
                cam_vertex_length = cam_height / earthCenter_cam_vertex_cos;
                surface_pos_ms = cam_ms + cam_vertex_dir_ms * cam_vertex_length;
                vec3 north = vec3(0,1,0);
                vec3 up = -cam_earthCenter_dir_n;
                vec3 tangent_east_n = normalize(cross(north, up));
                vec3 tangent_north_n = normalize(cross(up, tangent_east_n));
                vec3 cam_surface_pos = cam_ms - cam_earthCenter_dir_n * cam_height;
                vec3 cam_surface_vertex_dir = surface_pos_ms - cam_surface_pos;
                float cam_surface_vertex_length = length(cam_surface_vertex_dir);
                vec3 cam_surface_vertex_dir_n = normalize(cam_surface_vertex_dir);
                float cam_surface_vertex_dir_east_cos = dot(tangent_east_n, cam_surface_vertex_dir_n);
                float delta_lng = cam_surface_vertex_length * cam_surface_vertex_dir_east_cos / coord_length.y;
                float cam_surface_vertex_dir_north_cos = dot(tangent_north_n, cam_surface_vertex_dir_n);
                float delta_lat = cam_surface_vertex_length * cam_surface_vertex_dir_north_cos / coord_length.x;
                water_uv = water_offset_uv + vec2(delta_lat, delta_lng);
                gl_Position = MVP * vec4((surface_pos_ms - cam_earthCenter_dir_n * 0.0001 * texture(water_texture_sampler, water_uv).w), 1);
                return;
            }
            surface_pos_ms = cam_ms + cam_vertex_dir_ms * cam_vertex_length;
        }
        vec3 surface_pos_earth = surface_pos_ms + vertex_offset_ws;
        float radius_xz = sqrt(pow(surface_pos_earth.x, 2) + pow(surface_pos_earth.z, 2));
        float lat = atan(surface_pos_earth.y/radius_xz);
        float lng = -atan(surface_pos_earth.z/surface_pos_earth.x);
        if (surface_pos_earth.x < 0.0f) {
            if (lng < 0.0f) {
                lng += pi;
            } else {
                lng -= pi;
            }
        }
        water_uv = vec2(lat - water_origin_coord.x, lng - water_origin_coord.y) * water_uv_scale;
        float water_height = texture(water_texture_sampler, water_uv).w;
        gl_Position = MVP * vec4((surface_pos_ms - cam_earthCenter_dir_n * 0.0001 * water_height), 1);
        return;
    }

    uv = vertex_uv_ms;

    if (render_target == 0) {
        gl_Position = notrans_VP * vec4(vertex_pos_ms, 1);
    } else {
        gl_Position = MVP * vec4(vertex_pos_ms, 1);
    }

    vertex_ws = (M * vec4(vertex_pos_ms,1)).xyz;

    vec3 vertex_pos_cs = ( V * M * vec4(vertex_pos_ms,1)).xyz;
    vertex_origin_dir_cs = vec3(0,0,0) - vertex_pos_cs;

    vec3 sun_cs = ( V * vec4(sun_ws,1)).xyz;
    vertex_sun_dir_cs = sun_cs + vertex_origin_dir_cs;

    vertex_normal_cs = ( V * M * vec4(vertex_normal_ms,0)).xyz; // Only correct if ModelMatrix does not scale the model ! Use its inverse transpose if not.

    vec3 up_n_cs;
    float height_coord_linear;
    vec3 origin_vertex_dir_n_cs = -normalize(vertex_origin_dir_cs);
    vec3 sun_n_cs = normalize(sun_cs);

    vec3 sphereCenter_pos_cs = (V * vec4(-vertex_offset_ws,1)).xyz;
    vec3 sphereCenter_vertex_cs = vertex_pos_cs - sphereCenter_pos_cs;
    float sphereCenter_vertex_length = length(sphereCenter_vertex_cs);
    vec3 sphereCenter_vertex_normal_cs = normalize(sphereCenter_vertex_cs);
    vec3 vertex_pos_normal_cs = normalize(vertex_pos_cs);
    if (scatter_height > 1.0) {
        height_coord_linear = 1.0;
        float viewer_vertex_sphereCenter_cos = dot(normalize(-sphereCenter_vertex_cs), -normalize(vertex_pos_cs));
        float viewer_vertex_sphereCenter_acos = acos(viewer_vertex_sphereCenter_cos);
        float vertex_pos_length = length(vertex_pos_cs);
        if (render_target == 1) {
            float vertex_sphereCenter_intersec_cos = cos(pi - 2 * viewer_vertex_sphereCenter_acos);
            float camera_offset = vertex_pos_length - sqrt(2 * atmosphere_radius_square * (1 - vertex_sphereCenter_intersec_cos));
            up_n_cs = normalize(vertex_pos_normal_cs * camera_offset - sphereCenter_pos_cs);
        } else if (render_target == 2) {
            float vertex_atmosTopIntersect_sphereCenter_angle = asin(sphereCenter_vertex_length*sin(viewer_vertex_sphereCenter_acos)/atmosphere_top_radius);
            float camera_offset = vertex_pos_length - sqrt(pow(sphereCenter_vertex_length, 2) + pow(atmosphere_top_radius, 2)
                - 2*atmosphere_top_radius*sphereCenter_vertex_length*cos(pi-vertex_atmosTopIntersect_sphereCenter_angle-viewer_vertex_sphereCenter_acos));
            up_n_cs = normalize(vertex_pos_normal_cs * camera_offset - sphereCenter_pos_cs);
        }
    } else {
        height_coord_linear = scatter_height;
        up_n_cs = normalize(-sphereCenter_pos_cs);
    }
    vec4 scatter_rgba_first = texture4D(getScatterAngles(up_n_cs, origin_vertex_dir_n_cs, sun_n_cs, height_coord_linear, height_coord_linear*atmosphere_thickness));
    scatter_rgba = scatter_rgba_first;
    scatter_rgba = clamp(scatter_rgba, vec4(0), vec4(1));
    scatter_rgba.a = max(scatter_rgba.r, max(scatter_rgba.g, scatter_rgba.b));
    if (scatter_rgba.a > 0) {
        scatter_rgba.rgb /= scatter_rgba.a;
    }
    if (render_target == 2) {
        up_n_cs = sphereCenter_vertex_normal_cs;
        float height_sealevel = clamp(sphereCenter_vertex_length - earth_radius, 0, atmosphere_thickness);
        height_coord_linear = height_sealevel/atmosphere_thickness;

        vec4 scatter_rgba_second = texture4D(getScatterAngles(up_n_cs, origin_vertex_dir_n_cs, sun_n_cs, height_coord_linear, height_sealevel));
        float first_second_optical_depth = scatter_rgba_first.a / scatter_rgba_second.a;
        vec3 scatter_rgba_offset = vec3(scatter_rgba_second.r * first_second_optical_depth,
                                        scatter_rgba_second.g * pow(first_second_optical_depth, green_pow_coefficient),
                                        scatter_rgba_second.b * pow(first_second_optical_depth, blue_pow_coefficient));
        scatter_rgba.rgb = clamp(scatter_rgba_first.rgb - scatter_rgba_offset, vec3(0), vec3(1));
        scatter_rgba.a = clamp(max(scatter_rgba.r, max(scatter_rgba.g, scatter_rgba.b)), 0, 1);
        if(scatter_rgba.a > 0) {
            scatter_rgba.rgb /= scatter_rgba.a;
        }
    }

}

