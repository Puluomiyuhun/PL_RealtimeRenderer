#version 430 core
 
layout(binding = 3) uniform sampler2D u_normalMap;
layout(binding = 4) uniform samplerCube ReflectionText;

uniform vec3 u_lightDirection;
uniform vec3 u_color;
uniform vec3 camPos;
 
in vec2 v_texCoord;
in vec3 WorldPos;
in vec4 vReflectCoordinates;
 
out vec4 FragColor;
 
		
void main (void)
{
	/*vec3 normal = texture2D( u_normalMap, v_texCoord ).rgb;
 
	vec3 view = normalize( camPos - WorldPos );
			
	vec3 R = normalize( reflect( -view, normal ) );
 
	vec3 reflection = normalize( reflect( -u_lightDirection, normal ) );
	
	float specularFactor = pow( max( 0.0, dot( view, reflection ) ), 32.0 ) * 0.1f;
		
	vec3 distortion = normal * vec3( 0.1, 0.0, 0.1 ) ;
    vec3 reflectionColor = vec3(textureLod( ReflectionText, R, 0.0 )) * 1.0f;
 
	float distanceRatio = min( 1.0, log( 1.0 / length( camPos - WorldPos ) * 3000.0 + 1.0 ) );
	distanceRatio *= distanceRatio;
	distanceRatio = distanceRatio * 0.7 + 0.3;
			
	normal = ( distanceRatio * normal + vec3( 0.0, 1.0 - distanceRatio, 0.0 ) ) * 0.5;
	normal /= length( normal );
			
	float fresnel = pow( 1.0 - max(dot( normal, view ),0.1), 5.0 );
			
	float skyFactor = ( fresnel + 0.1 ) * 0.1;
	vec3 waterColor = ( 1.0 - fresnel ) * vec3(0.0627, 0.145, 0.3);

	vec3 color = ( skyFactor + specularFactor) * reflectionColor + waterColor * u_color ;
	FragColor = vec4( color, 1.0 );*/


    vec3 view = normalize( camPos - WorldPos );
  	vec3 normal = texture2D( u_normalMap, v_texCoord ).rgb;
	vec3 R = normalize( reflect( -view, normal ) );
    vec3 ambient = u_color * 0.6;
    vec3 reflectionColor = vec3(textureLod( ReflectionText, R, 0.0 )) * 1.0f;
    //漫反射
    vec3 norm = normalize(normal);
    vec3 lightDir = u_lightDirection;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_color;  
    
    //镜面光
    vec3 viewDir = normalize(camPos - WorldPos);
    vec3 reflectDir = normalize(reflect(-lightDir, norm));  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = 0.5 * spec * u_color;

    //菲涅尔
    float R0 = 0.02;
    float cosTheta = dot(viewDir, norm);
    float R1 = R0 + (1 - R0) * pow(1 - cosTheta, 5.0);
    vec3 fresnel = vec3(R1);
        
    vec3 re = (diffuse + specular) * vec4(0.0627, 0.145, 0.25, 1.0).xyz;
    re += ambient * fresnel * reflectionColor;
    FragColor = vec4(re, 1.0);
}