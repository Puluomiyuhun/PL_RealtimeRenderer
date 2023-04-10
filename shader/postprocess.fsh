#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{ 
    vec3 colors = vec3(texture(screenTexture, TexCoords));
    //这里对colors进行后处理
    FragColor.rgb = pow(colors,vec3(1/2.2));
    FragColor.a = 1.0;
}