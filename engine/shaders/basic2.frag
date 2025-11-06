#version 450 core

in vec3 vertexColor;
in vec2 TexCoord;

out vec4 FragColor;

layout(binding = 0) uniform sampler2D uTexture;

void main()
{
    //FragColor = texture(uTexture, TexCoord);  // just the image

    vec4 texColor = texture(uTexture, TexCoord);
    
    // Discard transparent pixels
    if (texColor.a == 0.0)
       discard;
    
    FragColor = texColor;  // No tinting, just the texture
}