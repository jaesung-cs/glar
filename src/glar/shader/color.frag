#version 430 core

in vec3 vColor;

out vec4 outColor;

void main() {
  outColor = vec4(vColor, 1.f);
}
