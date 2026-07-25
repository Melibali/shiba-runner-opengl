#version 330

uniform sampler2D tex;
uniform vec4 lumpos;
uniform vec4 diffuse_color;
uniform vec4 ambient_color;
uniform vec4 specular_color;
uniform float shininess;
uniform int hasTexture;

in vec2 vsoTexCoord;
in vec3 vsoNormal;
in vec4 vsoModPosition;

out vec4 fragColor;
void main(void) {
  vec3 N = normalize(vsoNormal);
  /* Lumière fixe + lumière orbitale combinées */
  vec3 L1 = normalize(vec3(1.0, 2.0, 1.0));              /* fixe */
  vec3 L2 = normalize(lumpos.xyz - vsoModPosition.xyz);  /* orbitale */
  float diff = max(dot(N, L1), 0.0) * 0.5
             + max(dot(N, L2), 0.0) * 0.6;

  vec3 V = normalize(-vsoModPosition.xyz);
  vec3 R = reflect(-L2, N);
  float spec = (shininess > 0.0)
               ? pow(max(dot(R, V), 0.0), shininess) : 0.0;

  vec4 base = (hasTexture == 1)
              ? texture(tex, vsoTexCoord)
              : vec4(0.85, 0.75, 0.65, 1.0);

  vec4 ambientLight = vec4(0.35, 0.30, 0.25, 1.0);
  fragColor = base * (ambientLight + diff * diffuse_color)
            + specular_color * spec;
  fragColor.a = base.a;
}