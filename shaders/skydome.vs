#version 330

/* position 3D du sommet (vertex) dans l'espace */
layout(location = 0) in vec3 pos;
/* vecteur normal du sommet dans l'espace */
layout(location = 1) in vec3 normal;
/* coordonnée de texture du sommet (liée à la surface de l'objet) */
layout(location = 2) in vec2 texCoord;
uniform mat4 proj;
uniform mat4 model;
uniform mat4 view;
uniform vec2 tex_mult;
uniform vec2 tex_translate;

out vec2 vsoTC;

void main() {
  gl_Position = proj * view * model * vec4(pos, 1.0);
  /* on transfère les corrdonnées de texture au fragment shader */
  vsoTC = tex_mult * texCoord + tex_translate;
}
