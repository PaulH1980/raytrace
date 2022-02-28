
layout(location = 0) in vec3 position;
layout(location = 1) in uint objectId;

out uint _objectId;

void main()
{
    gl_Position = (g_perLayer.m_proj * g_perLayer.m_view ) * vec4(position, 1.0); 
	_objectId = objectId;
		
}