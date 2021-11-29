#version 430 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 model;
uniform mat3 intrinsic;
uniform vec2 screenSize;

out vec3 vPosition;
out vec3 vNormal;

void main() {
}
