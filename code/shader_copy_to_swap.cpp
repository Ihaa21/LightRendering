#version 450

#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 InUv;

layout(location = 0) out vec4 OutColor;

layout(set = 0, binding = 0) uniform sampler2D ColorTexture;

void main()
{
    OutColor = texture(ColorTexture, InUv);
}
