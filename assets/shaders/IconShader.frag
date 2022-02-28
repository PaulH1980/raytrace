
uniform sampler2D tex;
uniform vec3  iconColor = vec3( 1.0, 1.0, 1.0);

in  vec2 _uvOut;
out vec4 _fragColor;

void main()
{
	vec4 rgba  = texture( tex, _uvOut );
	if( rgba.a < 0.5 )
		discard;			
	vec3 curColor = iconColor;
	if( isSelected != 0 )
	{
		vec4 selColor = GetSelectionColor();
		curColor = mix( curColor, selColor.rgb, selColor.a * 1.5 );
	}
	_fragColor = vec4( curColor * rgba.rgb, 1.0);
}