#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 outColor;
layout(push_constant) uniform PC { vec2 center; float radius; vec4 color; } pc;

void main() {
    vec2 p = v_uv;
    float dist = distance(p, pc.center);
    float edge = abs(fwidth(dist));
    float alpha = 1.0 - smoothstep(pc.radius - edge, pc.radius + edge, dist);
    if (alpha <= 0.0) discard;
    outColor = vec4(pc.color.rgb, pc.color.a * alpha);
}
