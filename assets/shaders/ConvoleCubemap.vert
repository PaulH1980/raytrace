layout(location = 0) in vec3 position;

uniform mat4 view;
uniform mat4 proj;

out vec3 _pos;

void main()
{
 	gl_Position   = (proj * view ) * vec4(position, 1.0);
	_pos 		  = position.xyz;
}