in vec3 vertex_ws;
in vec3 vertex_normal_cs;
in vec3 vertex_origin_dir_cs;
in vec3 vertex_sun_dir_cs;
in vec2 uv;
in vec2 water_uv;
noperspective in vec4 scatter_rgba;

out vec4 color;

uniform vec3 sun_ws;
uniform sampler2D terrain_texture_sampler;
uniform sampler2D spacecube_texture_sampler;
uniform sampler2D water_texture_sampler;
uniform int render_target;

void main()
{

    if(render_target == 3) {
        float coe = texture(water_texture_sampler, water_uv).a;
        color.rgb = vec3(1,1,1)*coe;
    }

    vec3 sun_color = vec3(1,1,1);
    float sun_power = 200000000.0f;

    vec4 terrain_rgba = texture(terrain_texture_sampler, uv);
    vec3 terrain_diffuse = vec3(0.01, 0.01, 0.01) + terrain_rgba.rgb;
    vec3 terrain_ambient = vec3(0.4, 0.4, 0.4) * terrain_diffuse;
    vec3 terrain_specular = vec3(1.0-terrain_rgba.a, 1.0-terrain_rgba.a, 1.0-terrain_rgba.a);

    float sun_vertex_distance = length( sun_ws - vertex_ws );

    vec3 vertex_normal_normal_cs = normalize( vertex_normal_cs );
    vec3 vertex_sun_dir_normal_cs = normalize( vertex_sun_dir_cs );
    // Cosine of the angle between the normal and the light direction,
    // clamped above 0
    //  - light is at the vertical of the triangle -> 1
    //  - light is perpendicular to the triangle -> 0
    //  - light is behind the triangle -> 0
    float cos_sun_vertex_normal = clamp( dot( vertex_normal_normal_cs,vertex_sun_dir_normal_cs ), 0,1 );

    vec3 vertex_origin_dir_normal_cs = normalize(vertex_origin_dir_cs);
    vec3 reflect_sun_vertex_cs = reflect(-vertex_sun_dir_normal_cs, vertex_normal_normal_cs);
    // Cosine of the angle between the Eye vector and the Reflect vector,
    // clamped to 0
    //  - Looking into the reflection -> 1
    //  - Looking elsewhere -> < 1
    float cos_origin_vertex_reflect = clamp( dot( vertex_origin_dir_normal_cs,reflect_sun_vertex_cs ), 0,1 );

    // TODO: branch optimizations
    if (render_target == 0) {
        color.rgb = texture(spacecube_texture_sampler, uv).rgb;
    } else {
        float factor = scatter_rgba.a;
        if (render_target == 1) {
            color = scatter_rgba;
        } else if (render_target == 2) {
            color.rgb =
                scatter_rgba.rgb*factor + (1-factor) * (terrain_ambient +
                terrain_diffuse * sun_color * sun_power * cos_sun_vertex_normal / (sun_vertex_distance) +
                terrain_specular * sun_color * sun_power * pow(cos_origin_vertex_reflect,7) / (sun_vertex_distance));
        }
    }
}
