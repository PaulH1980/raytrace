/**
* This shader is originally from https://learnopengl.com
**/


in vec3 _pos;


uniform samplerCube tex;
uniform float roughness;
uniform float resolution; 

out vec4 _colorOut;


void main()
{
    vec3 N = normalize(_pos);
    vec3 R = N;
    vec3 V = R;
    const uint SAMPLE_COUNT = 8192u;
    float totalWeight = 0.0;
    vec3 prefilteredColor = vec3(0.0);
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);
        float NdotH = max(dot(N, H), 0.0);
        float HdotV = max(dot(H, V), 0.0);
        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            float D   = DistributionGGX(NdotH, roughness);
            float pdf = (D * NdotH / (4.0 * HdotV)) + 0.0001;
            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
            float mipLevel = 0.0;
            if(roughness != 0.0){
                mipLevel = 0.5 * log2(saSample / saTexel);
            }

            prefilteredColor += texture(tex, L, mipLevel).rgb * NdotL;
            totalWeight      += NdotL;
        }

    }
    prefilteredColor = prefilteredColor / totalWeight;
    _colorOut = vec4(prefilteredColor, 1.0);
}