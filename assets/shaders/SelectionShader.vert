uniform mat4 model;


layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 uv;


void main()
{
    vec4 pos = model * vec4(position, 1.0);
	gl_Position = (g_perLayer.m_proj * g_perLayer.m_view ) * pos;   
}