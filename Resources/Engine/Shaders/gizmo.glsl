#shader vertex
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in vec2 aTexCoord;

uniform mat4 u_ViewProj;
uniform mat4 u_Model;

out vec2 v_TexCoord;

void main()
{
    v_TexCoord = aTexCoord;
    gl_Position = u_ViewProj * u_Model * vec4(aPos, 1.0);
}

#shader fragment
#version 460 core
layout (location = 0) out vec4 FragColor;

in vec2 v_TexCoord;

uniform sampler2D u_IconTexture;
uniform vec4 u_TintColor;

void main()
{
    vec4 texColor = texture(u_IconTexture, v_TexCoord) * u_TintColor;
    
    if (texColor.a < 0.1)
        discard;
        
    FragColor = texColor;
}