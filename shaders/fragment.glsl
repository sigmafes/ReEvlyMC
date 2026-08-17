#version 330 core
in vec3 vColor;
in float vLight;
in float vDistance;

uniform vec3 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;

out vec4 FragColor;

void main() {
    vec3 litColor = vColor * max(vLight, 0.0);

    float fog = clamp((vDistance - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);
    FragColor = vec4(mix(litColor, uFogColor, fog), 1.0);
}
