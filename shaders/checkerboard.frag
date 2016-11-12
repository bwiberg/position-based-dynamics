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
#define DARK_COLOR vec3(0.1)
#define LIGHT_COLOR vec3(1.0)

vec3 calcDirColor(vec3 normal) {
    vec3 R = reflect(-directional.dir, normal);
    vec3 E = normalize(WorldEye - WorldPosition.xyz);

    float NL = max(dot(-directional.dir, normal), 0);
    float RE = max(dot(E, R), 0);

    vec3 ambient = directional.ambient * Color.rgb;
    vec3 diffuse = directional.diffuse * Color.rgb * NL;
    vec3 specular = directional.specular * Color.rgb * pow(RE, shininess);

    return 0.25 * (ambient + diffuse + specular);
}

vec3 calcPointColor(vec3 normal) {
    float dist = length(WorldPosition.xyz - point.position.xyz);
    vec3 dir = (point.position.xyz - WorldPosition.xyz) / dist;

    vec3 R = reflect(dir, normal);
    vec3 E = normalize(WorldEye - WorldPosition.xyz);

    float NL = max(dot(dir, normal), 0);
    float RE = max(dot(E, R), 0);

    vec3 ambient = point.ambient * Color.rgb;
    vec3 diffuse = point.diffuse * Color.rgb * NL;
    vec3 specular = point.specular * Color.rgb * pow(RE, 100*shininess);

    vec3 c = ambient + diffuse + specular;

    return specular;
    //return 0.25 * c / (1 + point.att.linear * dist + point.att.quadratic * dist * dist);
}

void main() {
    float distanceFromOrigin = length(WorldPosition);
    if (distanceFromOrigin > 10.0) discard;

    float x = step(CHECK_SIZE / 2.0, mod(TexCoord.x, CHECK_SIZE));
    float y = step(CHECK_SIZE / 2.0, mod(TexCoord.y, CHECK_SIZE));

    vec3 checkerColor = DARK_COLOR;
    if ((x > 0.5 && y < 0.5) || (x < 0.5 && y > 0.5)) {
        checkerColor = LIGHT_COLOR;
    }

    vec3 normal = normalize(Normal);
    vec3 total = calcDirColor(normal) + calcPointColor(normal);

    color = vec4(calcPointColor(normal), 1.0);
    //color = vec4(checkerColor * total, 1.0);
}
