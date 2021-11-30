#version 430 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

uniform mat4 model;
uniform mat3 intrinsic;
uniform vec4 screen; // [width, height, near, far]

out VS_OUT {
  vec4 glPosition;
  vec3 position;
  vec3 color;
} vertex;

void main() {
  vec3 p = intrinsic * vec3(model * vec4(position, 1.f));

  // Perspective transform, with gl_Position.w = p.z
  const float near = screen.z;
  const float far = screen.w;
  vertex.glPosition = vec4(
    p.x / screen.x * 2.f - p.z,
    -p.y / screen.y * 2.f + p.z,
    (p.z * (far + near) - 2.f * far * near) / (far - near),
    p.z
  );

  vertex.position = vec3(model * vec4(position, 1.f));
  vertex.color = color;
}
