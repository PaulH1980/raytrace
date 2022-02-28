in VS_OUT
{
	vec3 m_near;
	vec3 m_far;
}fs_input;


out vec4 fragColor;

vec4 grid(vec3 fragPos3D, float scale) 
{
    vec2 coord 		= fragPos3D.xz * scale; // use the scale variable to set the distance between the lines
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);
    vec4 color = vec4(0.2, 0.2, 0.2, 1.0 - min(line, 1.0));
    // z axis
    if(fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
        color.z = 1.0;
    // x axis
    if(fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
        color.x = 1.0;
    return color;
}

float computeDepth(vec3 pos) {
    vec4 clip_space_pos = (g_perLayer.m_view  *g_perLayer.m_projInv) * vec4(pos.xyz, 1.0);
    return (clip_space_pos.z / clip_space_pos.w);
}

float computeLinearDepth(vec3 pos) {
    float clip_space_depth  = computeDepth(pos) * 2.0 - 1.0 ;
    float linearDepth = (2.0 * GetNear() * GetFar()) / (GetFar() + GetNear() - clip_space_depth * (GetFar() - GetNear())); // get linear value between 0.01 and 100
    return linearDepth / GetFar(); // normalize
}


void main()
{
	vec3 nearPoint = fs_input.m_near;
	vec3 farPoint  = fs_input.m_far;
	float t 	   = -nearPoint.y / (farPoint.y - nearPoint.y);	
	if( t < 0.0 )
		discard;
		
	
	vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);
	
	gl_FragDepth   = computeDepth(fragPos3D);
	
	vec4 grid3D    = grid(fragPos3D, 10.0);
	fragColor 	   = vec4(computeLinearDepth(fragPos3D)); 
}
