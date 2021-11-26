#version 430 core

layout (location = 0) in vec2 position;

out vec2 fragTexCoord;

void main() {
  gl_Position = vec4(position * 2.f - 1.f, 0.0f, 1.f);
  fragTexCoord = vec2(position.x, 1.f - position.y); // Flip v coordinate
}
