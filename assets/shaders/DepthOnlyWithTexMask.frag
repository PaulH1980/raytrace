
uniform sampler2D texMask;

in vec2 _texCoord;

void main() {
	float alpha = texture( texMask, _texCoord ).a;
	if( alpha < 0.5 )
		discard;
	
}