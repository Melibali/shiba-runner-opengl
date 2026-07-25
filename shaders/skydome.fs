#version 330
out vec4 fragColor;
uniform vec4 couleur;
uniform sampler2D tex;
uniform int sky, fog;
uniform float night_k;
in vec2 vsoTC;

void main() {
  vec4 t = texture(tex, vsoTC) * couleur;

  if(fog == 1 && sky == 0) {
    float depth     = gl_FragCoord.z / gl_FragCoord.w;
    float fogFactor = clamp(1.0 - depth / 80.0, 0.0, 1.0);
    vec4  fogColor  = vec4(night_k * 0.6, night_k * 0.75, night_k * 0.9, 1.0);
    t = mix(fogColor, t, fogFactor);
  }

  fragColor = t;
}