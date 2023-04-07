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
    sampler2D rho_d_tex;
    vec3 rho_s;
    float shininess_n;
}; 

uniform Material material;

void main()
{
   vec3 norm = normalize(normal);
   vec3 lightDir = normalize(lightPos - FragPos);
   vec3 rho_d = vec3(texture(material.rho_d_tex, texcoord));
   float cosine = max(dot(norm, lightDir), 0.0);

   vec3 viewDir = normalize(cameraPos - FragPos);
   vec3 reflectDir = reflect(-lightDir, norm);
   float cosine2 = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess_n);
   FragColor = vec4(material.ambient * rho_d + lightColor * (rho_d * cosine + material.rho_s * cosine2), 1.0);
}