
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;


out vec2 _texCoord;

void main()
{
    gl_Position = (g_perLayer.m_ortho2dProj * g_perLayer.m_ortho2dView ) * vec4(position, 1.0);    
	_texCoord = texCoord;
}