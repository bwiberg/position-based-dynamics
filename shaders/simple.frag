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

vec3 calcDirColor(vec3 normal) {
    vec3 R = reflect(directional.dir, normal);

    float NL = max(dot(-directional.dir, normal), 0);
    float RE = max(dot(WorldEye, R), 0);

    vec3 ambient = directional.ambient * Color.rgb;
    vec3 diffuse = directional.diffuse * Color.rgb * NL;
    vec3 specular = directional.specular * Color.rgb * pow(RE, shininess);

    return ambient + diffuse + specular;
}

vec3 calcPointColor(vec3 normal) {
    float dist = length(WorldPosition.xyz - point.position.xyz);
    vec3 dir = (point.position.xyz - WorldPosition.xyz) / dist;

    vec3 R = reflect(-dir, normal);

    float NL = max(dot(dir, normal), 0);
    float RE = max(dot(WorldEye, R), 0);

    vec3 ambient = point.ambient * Color.rgb;
    vec3 diffuse = point.diffuse * Color.rgb * NL;
    vec3 specular = point.specular * Color.rgb * pow(RE, shininess);

    vec3 c = ambient + diffuse + specular;

    return c / (1 + point.att.linear * dist + point.att.quadratic * dist * dist);
}

void main() {
    vec3 normal = normalize(Normal);
    vec3 total = calcDirColor(normal) + calcPointColor(normal);
    color = vec4(total, 1);
    //color = vec4(normal, 1);
}
