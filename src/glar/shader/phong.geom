#version 430 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
  vec4 glPosition;
  vec3 position;
  vec3 color;
} vertex[];

out vec3 vPosition;
out vec3 vNormal;
out vec3 vColor;

void main() {
  const vec3 v1 = vertex[1].position - vertex[0].position;
  const vec3 v2 = vertex[2].position - vertex[0].position;
  vNormal = normalize(cross(v1, v2));

  gl_Position = vertex[0].glPosition;
  vPosition = vertex[0].position;
  vColor = vertex[0].color;
  EmitVertex();

  gl_Position = vertex[1].glPosition;
  vPosition = vertex[1].position;
  vColor = vertex[1].color;
  EmitVertex();

  gl_Position = vertex[2].glPosition;
  vPosition = vertex[2].position;
  vColor = vertex[2].color;
  EmitVertex();
}
