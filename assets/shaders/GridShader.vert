layout(location = 0) in vec3 position;

vec3 gridPlane[4] = vec3[] (
    vec3(  -1,  1, 0), 
	vec3(  -1, -1, 0), 
	vec3(   1,  1, 0),	
    vec3(   1, -1, 0)	
);

out VS_OUT
{
	vec3 m_near;
	vec3 m_far;
}vs_output;



vec3 Unproject( float _x, float _y, float _z )
{
	vec4 unprojectedPoint =  (g_perLayer.m_viewInv * g_perLayer.m_projInv ) * vec4(_x, _y, _z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}


void main()
{
	 vec3 p = gridPlane[gl_VertexID].xyz;
	 
	 vs_output.m_near = Unproject( p.x, p.y, 0.0 );
	 vs_output.m_far  = Unproject( p.x, p.y, 1.0 );	 
	 
	 gl_Position = vec4(p, 1.0);	  
}
