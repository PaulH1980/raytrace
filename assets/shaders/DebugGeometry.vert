
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;


out vec2 _texCoord;
out vec3 _normal;

void main()
{
    gl_Position = (g_perLayer.m_proj * g_perLayer.m_view) * vec4(position, 1.0);    
	_texCoord = texCoord;
	_normal  = normal;
}