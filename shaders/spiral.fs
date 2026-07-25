#version 330
in vec2 uv;
out vec4 fragColor;
uniform float time;

void main() {
  vec2 p = uv - 0.5;
  float a = atan(p.y, p.x);
  float r = length(p);
  float v = sin(25.0 * r - a * 8.0 - time * 3.0);

  /* Vignettage : bords sombres */
  float vign = 1.0 - smoothstep(0.3, 0.7, r);

  /* Couleur dorée quand v>0, noir sinon */
  vec3 col = (v > 0.0)
    ? vec3(1.0, 0.85, 0.1) * vign   /* doré */
    : vec3(0.0, 0.0, 0.0);          /* noir */

  fragColor = vec4(col, 1.0);
}