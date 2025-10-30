#version 450

layout(location = 0) out vec2 v_uv;

void main() {
    vec2 pos = vec2((gl_VertexIndex==0)?-1.0:3.0, (gl_VertexIndex==1)?-1.0:3.0);
    gl_Position = vec4(pos, 0.0, 1.0);

    v_uv = gl_Position.xy * 0.5 + 0.5;
}
