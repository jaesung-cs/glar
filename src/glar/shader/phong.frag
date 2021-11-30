#version 430 core

in vec3 vPosition;
in vec3 vNormal;
in vec3 vColor;

out vec4 outColor;

void main() {
  const vec3 N = normalize(vNormal);

  // Eye is at (0, 0, 0)
  const vec3 V = normalize(-vPosition);

  // Light source is also (0, 0, 0)
  const vec3 L = normalize(-vPosition);
  const vec3 R = reflect(-L, N);
  
  const float diffuse = max(dot(N, L), 0.f);
  const float shininess = 128.f;
  const float specular = pow(max(dot(V, R), 0.f), shininess);

  vec3 color = diffuse * vColor + specular * vec3(1.f, 1.f, 1.f);

  outColor = vec4(color, 1.f);
}
