uniform sampler2D overlay;
in vec2 _texCoord;

out vec4 _colorOut;

void main() {
	_colorOut = texture(overlay, _texCoord);
}