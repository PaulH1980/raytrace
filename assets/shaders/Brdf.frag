
in vec2 _texCoord;
out vec4 _colorOut;

void main() {
	vec2 integratedBRDF = IntegrateBRDF(_texCoord.x, _texCoord.y);
    _colorOut = vec4(integratedBRDF, 0.0, 1.0 );
}