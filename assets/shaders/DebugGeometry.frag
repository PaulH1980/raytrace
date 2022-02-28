uniform sampler2D overlay;
uniform sampler2D albedo;
in vec2 _texCoord;
in vec3 _normal;
out vec4 _colorOut;


 float random (vec2 uv)
 {
    return fract(sin(dot(uv,vec2(12.9898,78.233)))*43758.5453123);
 }

vec4 ShowDebugTriangles()
{
	vec2 seed = vec2( gl_PrimitiveID, gl_PrimitiveID );
    return vec4( random( seed + vec2(333) ), random(seed + vec2(111)), random(seed + + vec2(222)), 1.0 );
}

vec4 ShowNormals()
{
	return vec4((normalize(_normal) * 0.5f ) + 0.5 , 1.0);
}

vec4 ShowTexCoords()
{
	return vec4( _texCoord, 0, 1 );
}

vec4 ShowTexture()
{
	return texture( albedo, _texCoord );
}

void main() {

	_colorOut = ShowTexCoords();
}