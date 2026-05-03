#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;
layout(push_constant) uniform Push {
    vec2 mousePos;
    float time;
} push;
void main() {
    vec3 rainbow = 0.5 + 0.5 * cos(push.time + fragColor.xyx + vec3(0, 2, 4));
    vec3 finalColor = mix(rainbow, vec3(push.mousePos, 1.0), 0.5);
    outColor = vec4(finalColor, 1.0);
}
