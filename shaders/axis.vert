#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

uniform mat4 VP;

out vec3 Color;

#define WINDOW_AXIS_X 0.2
#define WINDOW_AXIS_Y 0.2

void main() {
    vec3 flippedposition = position;
    flippedposition.y = -flippedposition.y;
    vec3 vposition = vec3(VP * vec4(0.2 * flippedposition, 1));

    vec2 screenpos = vposition.xy;

    // make in range [0, 1]
    screenpos = 0.5 * screenpos + 0.5;

    // make in range
    screenpos.x = WINDOW_AXIS_X * screenpos.x;
    screenpos.y = WINDOW_AXIS_Y * screenpos.y;

    // map back to [-1, 1]
    screenpos = 2 * (screenpos - 0.5);

    vposition.xy = screenpos;

    Color = color;
	gl_Position = vec4(vposition, 1.0);
}
