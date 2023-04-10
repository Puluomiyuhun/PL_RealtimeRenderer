#version 330 core

in vec3 f_normal;
in vec2 f_texcoord;
in vec3 f_FragPos;  
out vec4 FragColor;
uniform vec3 cameraPos;
uniform mat4 lightSpaceMatrix;
uniform samplerCube cubeTexture;
uniform sampler2D dir_shadowMap;
uniform samplerCube point_shadowMap;
uniform float far_plane;

struct Material {
    vec3 ambient;

    vec4 diffuse;
    bool diffuse_texture_use;
    sampler2D diffuse_texture;
    vec3 specular;
    bool specular_texture_use;
    sampler2D specular_texture;
    float reflects;
    bool reflect_texture_use;
    sampler2D reflect_texture;

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

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float cal_dir_shadow()
{
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(f_FragPos,1.0);
    // 执行透视除法
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // 变换到[0,1]的范围
    projCoords = projCoords * 0.5 + 0.5;
    // 取得当前片段在光源视角下的深度
    float currentDepth = projCoords.z;
    // 检查当前片段是否在阴影中
    float bias = 0.005;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(dir_shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(dir_shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    return shadow;
}

float cal_point_shadow()
{
    vec3 fragToLight = f_FragPos - pl[0].pos; 
    float currentDepth = length(fragToLight);
    float bias = 11.0; 
    float shadow = 0.0;
    int samples = 20;
    float viewDistance = length(cameraPos - f_FragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 2.0;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(point_shadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_plane;   // Undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);

    return shadow;
}

void main()
{
    vec3 norm = normalize(f_normal);
    FragColor = vec4(0,0,0,1);
    
    vec4 rho_d;
    if(material.diffuse_texture_use == true) {
        rho_d = texture(material.diffuse_texture, f_texcoord);
        rho_d.rgb = pow(rho_d.rgb,vec3(2.2));
    }
    else rho_d = material.diffuse;

    vec3 rho_s;
    if(material.specular_texture_use == true) rho_s = vec3(texture(material.specular_texture, f_texcoord));
    else rho_s = material.specular;

    float rho_r;
    if(material.reflect_texture_use == true) rho_r = float(texture(material.reflect_texture, f_texcoord));
    else rho_r = material.reflects;

    vec3 viewDir = normalize(cameraPos - f_FragPos);
    for(int i = 0; i < 1; i++){
        if(dl[i].color == vec3(0,0,0)) continue;
        vec3 lightDir = normalize(-dl[i].dir);
        vec3 h = (lightDir + viewDir) / length(lightDir + viewDir);
        float cosine = max(dot(norm, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, norm);
        float cosine2 = pow(max(dot(norm, h), 0.0), material.shininess_n);
        float shadow = cal_dir_shadow();
        FragColor += vec4(dl[i].color * (1 - shadow) * (vec3(rho_d) * cosine + rho_s * cosine2), 0.0);
    }
    for(int i = 0; i < 1; i++){
        if(pl[i].color == vec3(0,0,0)) continue;
        vec3 lightDir = normalize(pl[i].pos - f_FragPos);
        vec3 h = (lightDir + viewDir) / length(lightDir + viewDir);
        float distance = length(pl[i].pos - f_FragPos);
        float cosine = max(dot(norm, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, norm);
        float cosine2 = pow(max(dot(norm, h), 0.0), material.shininess_n);
        vec3 pl_color = pl[i].color / (pl[i].constant + pl[i].linear * distance + pl[i].quadratic * distance * distance);
        float shadow = cal_point_shadow();
        FragColor += vec4(pl_color * (1 - shadow) * (vec3(rho_d) * cosine + rho_s * cosine2), 0.0);
    }
    FragColor += vec4(material.ambient * vec3(rho_d), 0);

    vec3 reflectDir2 = reflect(-viewDir, norm);
    vec3 environment = texture(cubeTexture, reflectDir2).rgb;
    FragColor.rgb = environment * rho_r + FragColor.rgb * (1 - rho_r);
    //FragColor.a = rho_d.a;
}