#version 330

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 texCoord;
layout (location = 3) in vec4 color;

uniform mat4 M;
uniform mat4 MVP;

out vec4 WorldPosition;
out vec3 Normal;
out vec2 TexCoord;
out vec4 Color;

void main() {
    Normal = vec3(normal);
    TexCoord = texCoord;
    Color = color;

    vec4 Position = vec4(position.x, position.y, position.z, 1.0);

    WorldPosition   = M * Position;
	gl_Position     = MVP * Position;
}
