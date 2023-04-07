#version 330 core

in vec3 normal;
in vec2 texcoord;
in vec3 FragPos;  
out vec4 FragColor;
uniform sampler2D ourTexture;
uniform vec3 lightPos;
uniform vec3 lightColor;

void main()
{
   vec3 ambient = lightColor * 0.15;
   vec3 norm = normalize(normal);
   vec3 lightDir = normalize(lightPos - FragPos);
   vec3 diffuse = vec3(texture(ourTexture, texcoord));
   float cosine = max(dot(norm, lightDir), 0.0);
   FragColor = vec4(ambient + lightColor * diffuse * cosine, 1.0);
}