#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in float aLight;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vColor;
out float vLight;
out float vDistance;

void main() {
    vec4 viewPos = uView * vec4(aPos, 1.0);
    vDistance = length(viewPos.xyz);
    vColor = aColor;
    vLight = aLight;
    gl_Position = uProjection * viewPos;
}
