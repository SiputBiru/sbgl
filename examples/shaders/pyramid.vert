#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushData {
    mat4 viewProj;
} pc;

void main() {
    gl_Position = pc.viewProj * inPosition;
    fragColor = inColor.rgb;
}
