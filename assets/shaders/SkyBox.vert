
layout (location = 0) in vec3 position;

out vec3 _texCoord;

void main()
{
    
    vec4 pos = g_perLayer.m_proj * g_perLayer.m_skyView * vec4(position, 1.0);
    gl_Position = pos.xyww; //https://learnopengl.com/Advanced-OpenGL/Cubemaps
    _texCoord = position;
};