#version 410 core

in vec3 fragPosEye;
in vec3 normalEye;
in vec2 fragTexCoords;

out vec4 fColor;

// dirlight
uniform vec3 lightDir;
uniform vec3 lightColor;

// texturi
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

// spotlight
struct SpotLight
{
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 color;
};

uniform SpotLight bradSpotLight;

const float ambientStrength  = 0.15;
const float specularStrength = 0.6;
const float shininess        = 64.0;

// fog
float computeFog()
{
    float fogDensity = 0.011;             
    float distance = length(fragPosEye);  
    float fogFactor = exp(-pow(distance * fogDensity, 2.0));
    return clamp(fogFactor, 0.0, 1.0);
}

// directional
void computeDirLight(out vec3 ambient,
                     out vec3 diffuse,
                     out vec3 specular)
{
    vec3 N = normalize(normalEye);
    vec3 L = normalize(lightDir);
    vec3 V = normalize(-fragPosEye);

    ambient = ambientStrength * lightColor;

    float diff = max(dot(N, L), 0.0);
    diffuse = diff * lightColor;

    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess);
    specular = spec * lightColor;
}

// spotlight
vec3 computeSpotLight()
{
    vec3 N = normalize(normalEye);
    vec3 L = normalize(bradSpotLight.position - fragPosEye);

    float theta = dot(L, normalize(-bradSpotLight.direction));
    float epsilon = bradSpotLight.cutOff - bradSpotLight.outerCutOff;
    float intensity = clamp((theta - bradSpotLight.outerCutOff) / epsilon, 0.0, 1.0);

    float diff = max(dot(N, L), 0.0);
    return diff * bradSpotLight.color * intensity;
}

//pointlight for tavern
struct PointLight {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};
uniform PointLight tavernLight;

vec3 computePointLight(PointLight light)
{
    vec3 N = normalize(normalEye);
    vec3 L = normalize(light.position - fragPosEye);

    float diff = max(dot(N, L), 0.0);

    float d = length(light.position - fragPosEye);
    float att = 1.0 / (light.constant + light.linear * d + light.quadratic * d * d);

    return diff * light.color * att;
}

void main()
{
    // light
    vec3 ambient, diffuse, specular;
    computeDirLight(ambient, diffuse, specular);
    vec3 spot = computeSpotLight();
    vec3 tav = computePointLight(tavernLight);

    vec3 texDiff = texture(diffuseTexture, fragTexCoords).rgb;
    vec3 texSpec = texture(specularTexture, fragTexCoords).rgb;

    vec3 lightingColor =
        (ambient + diffuse + tav) * texDiff +
        (specular + spot * 2.5) * texSpec;

    vec4 baseColor = vec4(min(lightingColor, 1.0), 1.0);

    // fog
    float fogFactor = computeFog();
    vec4 fogColor = vec4(0.7, 0.7, 0.7, 1.0);

    fColor = mix(fogColor, baseColor, fogFactor);
}
