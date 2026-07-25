#version 330
uniform sampler2D myTexture;
uniform float time;
uniform vec4 colorTint;
in vec2 vsoTexCoord;
out vec4 fragColor;
void main(void)
{
    vec2 uv = vsoTexCoord;
    vec4 tex = texture(myTexture, uv);
    /* fumée légère */
    float smoke =
        sin(uv.y * 20.0 + time * 0.7) *
        sin(uv.x * 14.0 + time * 0.5);
    smoke = smoke * 0.08 + 0.08;
    /* glow */
    float glow = 1.0 + 0.25 * sin(time * 3.0);
    vec3 col = tex.rgb * colorTint.rgb * glow;
    col += smoke;
    fragColor = vec4(col, tex.a * colorTint.a);
}
