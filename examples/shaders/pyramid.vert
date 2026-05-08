#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushData {
    mat4 viewProj;
} pc;

void main() {
    gl_Position = pc.viewProj * vec4(inPosition, 1.0);
    fragColor = inColor.rgb;
}
