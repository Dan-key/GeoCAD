#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main()
{
    int a = int(gl_FragCoord.xy[1]) % 60;
    if (a > 30 ){
        discard;
    }
    outColor = vec4(fragColor, 1.0);
}

