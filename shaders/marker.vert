#version 330

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 normal;
layout (location = 2) in vec2 texCoord;
layout (location = 3) in vec4 color;

uniform mat4 M;
uniform mat4 MVP;

out vec2 TexCoord;

void main() {
    TexCoord = texCoord;
    gl_Position = MVP * vec4(position.x, position.y, position.z, 1.0);
}
