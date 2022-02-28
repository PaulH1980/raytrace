uniform sampler2D tex;
in vec3  _pos;
out vec4 _colorOut;

void main() {

	vec2 uv    = SampleSphericalMap(normalize(_pos) * -1.0);
    vec3 color = texture(tex, uv).rgb;
	_colorOut  = vec4( color, 1.0 );
}