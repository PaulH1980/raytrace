uniform mat4 model;
uniform vec4 color;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 uv;

out vec4 _color;

void main()
{
    gl_Position = (g_perLayer.m_proj * g_perLayer.m_view * model) * vec4(position, 1.0); 
    _color =  vec4((normalize(normal) * 0.5f ) + 0.5, 1.0);    
}