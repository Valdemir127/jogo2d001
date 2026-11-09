shader_type canvas_item;

uniform sampler2D tex_paleta : filter_nearest, repeat_disable;
uniform vec2  tam_grid;
uniform bool  usa_r16 = true;

void fragment() {
    float raw = texture(TEXTURE, UV).r;
    int id = usa_r16 ? int(round(raw * 65535.0)) : int(round(raw * 255.0));

    vec4 cor = texelFetch(tex_paleta, ivec2(id, 0), 0);

    ivec2 cell = ivec2(UV * tam_grid);
    int   h    = (cell.x * 73856093) ^ (cell.y * 19349663);
    float v    = float((h >> 8) & 0xFF) / 255.0;
    cor.rgb *= 0.94 + 0.12 * v;

    COLOR = cor;
}