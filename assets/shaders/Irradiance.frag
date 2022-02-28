
 


uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];


//input from vertex shader
in VS_OUT
{
	vec3 m_pos;
	vec3 m_normal;
	vec3 m_tangent;
	vec3 m_biTangent;
	vec2 m_uv;
}fs_input;

out vec4 _colorOut;


mat3 cotangent_frame( vec3 N, vec3 p, vec2 uv )
    {
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );
 
    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
    // construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, N );
    }

vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord )
{
    // assume N, the interpolated vertex normal and 
    // V, the view vector (vertex to eye)
	
	mat3 TBN = cotangent_frame( N, -V, texcoord );
	vec3 tangentNormal = texture(tex_normal,texcoord).xyz * 2.0 - 1.0;
    return normalize( TBN * tangentNormal );
}
	
vec3 GetNormalFromMap( vec2 uv, vec3 worldPos )
{
    vec3 tangentNormal = texture(tex_normal,uv).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(worldPos);
    vec3 Q2  = dFdy(worldPos);
    vec2 st1 = dFdx(uv);
    vec2 st2 = dFdy(uv);

    vec3 N   = normalize(fs_input.m_normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

vec4 GetAlbedo( vec2 uv )
{
	if( (hasTextureFlag & albedoFlags) != 0 )
		return texture( tex_albedo, uv );
	
	return const_albedo;
}

vec3 GetNormal(vec2 uv)
{
	if( (hasTextureFlag & normalFlags) != 0 )
		return texture( tex_normal, uv).rgb;
	
	return const_normal;
}

vec3 GetAO_Metal_Rough(vec2 uv)
{
	if( (hasTextureFlag & ao_rough_metalFlags) != 0 )
		return texture( tex_ao_rough_metal, uv).rgb;
	
	return const_ao_rough_metal;
}

vec3 GetEmission(vec2 uv)
{
	if((hasTextureFlag & emissionFlags) != 0)
		return texture( tex_emission, uv).rgb;
	
	return const_emission;
}



vec3 PointLight( vec3 N, vec3 V, vec3 F0, float roughness, float metallic )
{
    return vec3(1.0);
}

vec3 SpotLight()
{
    return vec3(1.0);
}

vec3 DirLight()
{
    return vec3(1.0);
}


void main()
{	
	vec2 uv 			= fs_input.m_uv;	
	mat3 TBN 			= transpose(mat3(fs_input.m_tangent, fs_input.m_biTangent, fs_input.m_normal));
	
	vec4 color 		= GetAlbedo( uv );
    vec3 albedo     = pow( color.rgb, vec3(2.2));
    float alpha     = color.a;
	if(alpha < 0.25 ) //todo do in differnt pass
		discard;
	
	vec3 V = normalize( g_perLayer.m_cameraPos.xyz - fs_input.m_pos);	
	vec3 N 		        = GetNormalFromMap( uv, V); 	
	vec3 ao_metal_rough = GetAO_Metal_Rough( uv );
	vec3 emission 		= GetEmission( uv );

    float ao            = ao_metal_rough.r;   
    float roughness     = ao_metal_rough.g;	
    float metallic      = ao_metal_rough.b;

    //ao = 1.0;
	
	
    
	
	vec3 F0 = vec3(0.04); 
    F0 = mix( F0, albedo, metallic );
	
	vec3 Lo = vec3(0.0);
    for(int i = 0; i < 0; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - g_perLayer.m_cameraPos.xyz);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - g_perLayer.m_cameraPos.xyz);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(dot( N, H ), roughness);   
        float G   = GeometrySmith(N, V, L, roughness);    
        vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);        
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
         // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	                
            
        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }   
    Lo = vec3(0.0);

    vec3 R = reflect(-V, N);

      // ambient lighting (we now use IBL as the ambient term)
    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    
    vec3 irradiance   = texture(tex_irradiance, N).rgb;
    vec3 diffuse      = irradiance * albedo;
	
	
    
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(tex_prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf     = texture(tex_brdfLUT, vec2( max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao; 
    vec3 finalColor = ambient + Lo + emission;	
    _colorOut = vec4( Gamma( finalColor ), 1.0 ); 
}