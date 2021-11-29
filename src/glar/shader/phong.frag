#version 430 core

in vec3 vPosition;
in vec3 vNormal;

out vec4 outColor;

void main() {
  outColor = vec4(vNormal, 1.f);
}
