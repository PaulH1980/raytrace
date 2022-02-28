uniform sampler2D depthTex;
in vec2 _texCoord;


layout(location = 0) out vec4 _depthOut;
void main() {
	_depthOut = texture(depthTex, _texCoord);
}