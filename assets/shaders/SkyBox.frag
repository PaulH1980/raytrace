
uniform samplerCube albedo;

in vec3 _texCoord;
out vec4 _colorOut;

void main()
{
    vec3 rgb =Gamma( texture(albedo, _texCoord).xyz);
	
	_colorOut = vec4( rgb, 1.0 );
}