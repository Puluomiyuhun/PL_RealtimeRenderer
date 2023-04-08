#version 330 core

in vec3 normal;
in vec2 texcoord;
in vec3 FragPos;  
out vec4 FragColor;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 cameraPos;

struct Material {
    vec3 ambient;

    vec4 diffuse;
    bool diffuse_texture_use;
    sampler2D diffuse_texture;
    vec3 specular;
    bool specular_texture_use;
    sampler2D specular_texture;

    float shininess_n;
}; 
uniform Material material;

struct DirectionLight {
    vec3 dir;
    vec3 color;
};

struct PointLight{
    vec3 pos;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};

uniform DirectionLight dl[6];
uniform PointLight pl[6];

void main()
{
    vec3 norm = normalize(normal);
    FragColor = vec4(0,0,0,1);
    
    vec4 rho_d;
    if(material.diffuse_texture_use == true) rho_d = texture(material.diffuse_texture, texcoord);
    else rho_d = material.diffuse;

    vec3 rho_s;
    if(material.specular_texture_use == true) rho_s = vec3(texture(material.specular_texture, texcoord));
    else rho_s = material.specular;

    vec3 viewDir = normalize(cameraPos - FragPos);
    for(int i = 0; i < 6; i++){
        if(dl[i].color == vec3(0,0,0)) continue;
        vec3 lightDir = normalize(-dl[i].dir);
        float cosine = max(dot(norm, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, norm);
        float cosine2 = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess_n);
        FragColor += vec4(dl[i].color * (vec3(rho_d) * cosine + rho_s * cosine2), 0.0);
    }
    for(int i = 0; i < 6; i++){
        if(pl[i].color == vec3(0,0,0)) continue;
        vec3 lightDir = normalize(pl[i].pos - FragPos);
        float distance = length(pl[i].pos - FragPos);
        float cosine = max(dot(norm, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, norm);
        float cosine2 = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess_n);
        vec3 pl_color = pl[i].color / (pl[i].constant + pl[i].linear * distance + pl[i].quadratic * distance * distance);
        FragColor += vec4(pl_color * (vec3(rho_d) * cosine + rho_s * cosine2), 0.0);
    }
    FragColor += vec4(material.ambient * vec3(rho_d), 0);
    FragColor.a = rho_d.a;
}