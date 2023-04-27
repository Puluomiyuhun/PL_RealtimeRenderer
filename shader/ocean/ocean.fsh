#version 430 core
 
layout(binding = 4) uniform samplerCube ReflectionText;

uniform vec3 u_lightDirection;
uniform vec3 u_color;
uniform vec3 camPos;
uniform vec3 diffuse;
uniform float specular;
uniform float transparency;
 
in vec2 v_texCoord;
in vec3 WorldPos;
in vec4 vReflectCoordinates;
in vec3 f_normal;
 
out vec4 FragColor;
 
		
void main (void)
{
	vec3 normal = f_normal;
 
	vec3 view = normalize( camPos - WorldPos );
			
	vec3 R = normalize( reflect( -view, normal ) );
 
	vec3 reflection = normalize( reflect(u_lightDirection, normal ) );
	
	float specularFactor = pow( max( 0.0, dot( view, reflection ) ), 16.0 ) * 0.2;
		
	vec3 distortion = normal * vec3( 0.1, 0.0, 0.1 ) ;
    vec3 reflectionColor = vec3(textureLod( ReflectionText, R, 0.8));
 
	float distanceRatio = min( 1.0, log( 1.0 / length( camPos - WorldPos ) * 300.0 + 1.0 ) );
	distanceRatio *= distanceRatio;
	distanceRatio = distanceRatio * 0.7 + 0.3;
			
	normal = ( distanceRatio * normal + vec3( 0.0, 1.0 - distanceRatio, 0.0 ) ) * 0.5;
	normal /= length( normal );
			
	float fresnel = pow( 1.0 - max(dot( normal, view ),0.1), 5.0 );
			
	float skyFactor = 0.1 + fresnel * fresnel;
	vec3 waterColor = ( 1.0 - skyFactor ) * diffuse;

	vec3 color = ( skyFactor + specularFactor) * reflectionColor * specular + waterColor * (u_color * 0.1 + 1) ;
    FragColor.rgb = pow(color,vec3(2.2));
    FragColor.a = transparency;
}