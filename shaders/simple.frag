#version 330

struct Attenuation {
    float a;
    float b;
};

struct DirectionalLight {
    vec3 color;
    float intensity;
    vec3 dir;
};

struct AmbientLight {
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 color;
    float intensity;
    vec4 position;
    Attenuation att;
};

uniform DirectionalLight directional;
uniform AmbientLight ambient;
uniform PointLight point;

in vec4 WorldPosition;
in vec3 Normal;
in vec2 TexCoord;
in vec4 Color;

out vec4 color;

vec3 calcAmbColor() {
    return Color.rgb * ambient.color * ambient.intensity;
}

vec3 calcDirColor() {
    return max(dot(-directional.dir, Normal), 0) * directional.intensity *
                                   Color.rgb * directional.color;
}

vec3 calcPointColor() {
    float dist = length(WorldPosition.xyz - point.position.xyz);
    vec3 dir = (point.position.xyz - WorldPosition.xyz) / dist;

    vec3 clr = max(dot(dir, Normal), 0) * point.intensity * color.rgb * point.color;

    return clr / (1 + point.att.a * dist + point.att.b * dist * dist);
}

void main() {
    vec3 total = calcAmbColor() + calcDirColor() + calcPointColor();

    color = vec4(total, 1);
}
