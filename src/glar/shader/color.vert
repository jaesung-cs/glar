#version 430 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

uniform mat4 model;
uniform mat3 intrinsic;
uniform vec4 screen; // [width, height, near, far]

out vec3 vColor;

void main() {
  vec3 p = intrinsic * vec3(model * vec4(position, 1.f));

  // Unlike usual perspective projection matrix, linearize depth between near and far
  // TODO: connection between perspective
  const float near = screen.z;
  const float far = screen.w;
  vec3 ndc = vec3(
    p.x / p.z / screen.x * 2.f - 1.f,
    -p.y / p.z / screen.y * 2.f + 1.f,
    (p.z - near) / (far - near));

  gl_Position = vec4(ndc, 1.f);

  vColor = color;
}
