#version 430 core

in vec2 fragTexCoord;

uniform sampler2D tex;

out vec4 outColor;

void main() {
  outColor = vec4(texture(tex, fragTexCoord).rgb, 1.f);
}
