#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 outColor;

struct InstanceData {
    mat4 transform;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    uint64_t instanceAddress;
    uint64_t userAddress;
} pc;

void main() {
    InstanceBuffer instanceData = InstanceBuffer(pc.instanceAddress);
    InstanceData data = instanceData.instances[gl_InstanceIndex];
    
    outColor = inColor * data.color.rgb;
    gl_Position = pc.viewProj * data.transform * vec4(inPos, 1.0);
}
