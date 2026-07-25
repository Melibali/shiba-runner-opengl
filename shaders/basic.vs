#version 330

layout(location=0) in vec3 vsiPosition;
layout(location=2) in vec2 vsiTexCoord;
uniform int inv;
uniform float zoom;
out vec2 vsoTexCoord;
void main(void)
{
    vec3 pos = vsiPosition * zoom;
    gl_Position = vec4(pos,1.0);
    if(inv != 0)
        vsoTexCoord = vec2(vsiTexCoord.x,1.0-vsiTexCoord.y);
    else
        vsoTexCoord = vsiTexCoord;
}