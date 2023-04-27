#version 430 core
layout(binding = 0) uniform sampler2D u_displacementMapx;
layout(binding = 1) uniform sampler2D u_displacementMapy;
layout(binding = 2) uniform sampler2D u_displacementMapz;
layout(binding = 3) uniform sampler2D u_normalMap;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
 
layout (location = 0) in vec3 a_vertex;
layout (location = 1) in vec2 a_texCoord;
 
out vec2 v_texCoord;
out vec3 WorldPos;
out vec4 vReflectCoordinates;
out vec3 f_normal;
 
void main(void)
{
    v_texCoord = a_texCoord;
    float displacementx = texture(u_displacementMapx, a_texCoord).r;
    float displacementy = texture(u_displacementMapy, a_texCoord).r;
    float displacementz = texture(u_displacementMapz, a_texCoord).r;
	vec4 displacement = vec4(displacementx,displacementy,displacementz,0);
    WorldPos = vec3(model * (vec4(a_vertex,1.0) + displacement));
    gl_Position = projection * view * vec4(WorldPos, 1.0f);
    vReflectCoordinates = gl_Position;
	f_normal = texture2D( u_normalMap, v_texCoord ).rgb;
    //vReflectCoordinates = u_Projection * u_View * u_Model * u_ReflectMatrix * (a_vertex + displacement);
}