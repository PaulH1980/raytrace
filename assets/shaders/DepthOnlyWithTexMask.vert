layout(location = 0) in vec3 position;
layout(location = 3) in vec2 texCoord;
uniform mat4 model;
out vec2 _texCoord;

void main()
{
    gl_Position = (g_perLayer.m_proj * g_perLayer.m_view * model) * vec4(position, 1.0);   
	_texCoord = texCoord;
}