#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec3 aNormal;

out VS_OUT {
   vec3 normal;
   vec2 texcoord;
   vec3 FragPos;  
}vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
   gl_Position = projection * view * model * vec4(aPos, 1.0);
   vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
   vs_out.normal = mat3(transpose(inverse(model))) * aNormal;;
   vs_out.texcoord = aUV;
}