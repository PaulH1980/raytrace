uniform mat4 model;
layout(location = 0) in vec3 position;

void main()
{
    gl_Position = (g_perLayer.m_proj * g_perLayer.m_view * model) * vec4(position, 1.0);  
}