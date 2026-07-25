#version 330
uniform float dt;
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
in  vec2 vsoTexCoord;
out vec4 fragColor;
void main(void) {
  vec2 tc = vec2(vsoTexCoord.x, 1.0 - vsoTexCoord.y);
  float tv = texture(tex2, tc).r;
  float f = smoothstep(tv - 0.05, tv + 0.05, dt);
  fragColor = mix(texture(tex0, vsoTexCoord),
                  texture(tex1, vsoTexCoord), f);
}