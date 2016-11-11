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

#define CHECK_SIZE 0.01
#define DARK_COLOR vec3(0.1)
#define LIGHT_COLOR vec3(1.0)

vec3 calcAmbColor() {
    return ambient.color * ambient.intensity;
}

vec3 calcDirColor() {
    return max(dot(-directional.dir, Normal), 0) * directional.intensity *
                                   vec3(Color) * directional.color;
}

vec3 calcPointColor() {
    float dist = length(WorldPosition.xyz - point.position.xyz);
    vec3 dir = (point.position.xyz - WorldPosition.xyz) / dist;

    vec3 clr = max(dot(dir, Normal), 0) * point.intensity * vec3(Color) * point.color;

    return clr / (1 + point.att.a * dist + point.att.b * dist * dist);
}

void main() {
    float x = step(CHECK_SIZE / 2.0, mod(TexCoord.x, CHECK_SIZE));
    float y = step(CHECK_SIZE / 2.0, mod(TexCoord.y, CHECK_SIZE));

    vec3 checkerColor = DARK_COLOR;
    if ((x > 0.5 && y < 0.5) || (x < 0.5 && y > 0.5)) {
        checkerColor = LIGHT_COLOR;
    }

    vec3 lighting = calcAmbColor() + checkerColor * (calcDirColor() + calcPointColor());
    //vec3 lighting = calcPointColor();

    float distanceFromOrigin = length(WorldPosition);

    if (distanceFromOrigin > 10.0) discard;

    color = vec4(lighting * exp(-0.05*distanceFromOrigin), 1.0);
    //color = vec4(ambient.color, 1.0);
}
