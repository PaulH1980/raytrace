uniform mat4 model;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 uv;


out VS_OUT
{
	vec3 m_pos;
	vec3 m_normal;
	vec3 m_tangent;
	vec3 m_biTangent;
	vec2 m_uv;
}vs_output;



void main()
{
	vec4 pos = model * vec4( position, 1.0); //worldspace
	
	mat3 normalMat = mat3( model );	
	vec3 N 	= normalize( normalMat * normal );
	vec3 T 	= normalize( normalMat * tangent);
	vec3 B 	= cross( N, T );
	
	vs_output.m_pos    	  = pos.xyz;
	vs_output.m_normal    = N;
	vs_output.m_tangent   = T;
	vs_output.m_biTangent = B; 
	vs_output.m_uv        = uv;	   
	
	gl_Position = (g_perLayer.m_proj * g_perLayer.m_view) * pos; //clipspace
}