#version 330

struct DirectionalLight {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 dir;
};

struct PointLight {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec4 position;
    struct {
        float linear;
        float quadratic;
    } att;
};

uniform DirectionalLight directional;
uniform PointLight point;
uniform vec3 WorldEye;
uniform float shininess;

in vec4 WorldPosition;
in vec3 Normal;
in vec2 TexCoord;
in vec4 Color;

out vec4 color;

#define CHECK_SIZE 0.01
#define DARK_COLOR vec3(0.2, 0.1, 0.1)
#define LIGHT_COLOR vec3(1.0)

float calcChecker() {
    float x = step(CHECK_SIZE / 2.0, mod(TexCoord.x, CHECK_SIZE));
    float y = step(CHECK_SIZE / 2.0, mod(TexCoord.y, CHECK_SIZE));

    return (x > 0.5 && y < 0.5) || (x < 0.5 && y > 0.5) ? 1.0f : 0.0;
}

vec3 getMaterialColor() {
    float checker = calcChecker();
    return checker * LIGHT_COLOR + (1 - checker) * DARK_COLOR;
}

vec3 calcDirColor(vec3 normal) {
    vec3 R = reflect(-directional.dir, normal);

    vec3 eye = WorldEye;
    eye.y = - eye.y;
    vec3 E = normalize(eye - WorldPosition.xyz);

    float NL = max(dot(-directional.dir, normal), 0);
    float RE = max(dot(E, R), 0);

    vec3 ambient = directional.ambient * getMaterialColor();
    vec3 diffuse = directional.diffuse * getMaterialColor() * NL;
    vec3 specular = directional.specular * getMaterialColor() * pow(RE, shininess);

    return 0.25 * (ambient + diffuse + specular);
}

vec3 calcPointColor(vec3 normal) {
    float dist = length(WorldPosition.xyz - point.position.xyz);
    vec3 dir = (WorldPosition.xyz - point.position.xyz) / dist;

    vec3 R = reflect(dir, normal);
    vec3 eye = WorldEye;
    eye.y = - eye.y;
    vec3 E = normalize(eye - WorldPosition.xyz);

    float NL = max(dot(-dir, normal), 0);
    float RE = max(dot(E, R), 0);

    vec3 ambient = point.ambient * getMaterialColor();
    vec3 diffuse = point.diffuse * getMaterialColor() * NL;
    vec3 specular = 2 * point.specular * getMaterialColor() * pow(RE, 100*shininess);

    vec3 c = ambient + diffuse + specular;

    return c / (1 + point.att.linear * dist + point.att.quadratic * dist * dist);
}

void main() {
    float distanceFromOrigin = length(WorldPosition);
    if (distanceFromOrigin > 10.0) discard;

    vec3 normal = normalize(Normal);
    vec3 total = /*calcDirColor(normal) +*/ calcPointColor(normal);

    color = vec4(total, 1.0);
}
