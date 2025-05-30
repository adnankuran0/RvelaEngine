#version 400 core
out float FragColor;
in vec2 TexCoords;

uniform sampler2D gNormal;
uniform sampler2D gDepth;
uniform sampler2D texNoise;
uniform mat4 projection;
uniform mat4 invProjection; 
uniform vec3 samples[64];

const float radius = 1.0;      
const float bias = 0.1;        
const float intensity = 1.0;
const vec2 noiseScale = vec2(1280.0/4.0, 720.0/4.0);
uniform float near;
uniform float far;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

vec3 getViewPos(vec2 uv)
{
    // DÜZELTME: Ham derinlik değerini kullan
    float depthValue = texture(gDepth, uv).r;
    vec4 ndc = vec4(
        uv.x * 2.0 - 1.0,
        uv.y * 2.0 - 1.0,
        depthValue * 2.0 - 1.0, // Lineerleştirme YOK
        1.0
    );
    vec4 viewPos = invProjection * ndc; // CPU'dan gelen ters matris
    return viewPos.xyz / viewPos.w;
}

void main()
{
    vec3 fragPos = getViewPos(TexCoords); // Görüş uzayında (z negatif)
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
    
    // TBN matrisi oluştur (aynı)
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    float occlusion = 0.0;
    for(int i = 0; i < 64; ++i)
    {
        vec3 sampleDir = TBN * samples[i]; // Tangent -> Görüş uzayı
        vec3 samplePos = fragPos + sampleDir * radius;
        
        // Örnek noktayı ekran koordinatına dönüştür
        vec4 clipPos = projection * vec4(samplePos, 1.0);
        clipPos.xyz /= clipPos.w; // Perspektif bölme
        vec2 sampleUV = clipPos.xy * 0.5 + 0.5;
        
        // Örneklenen geometrinin gerçek derinliğini al
        float sampledDepth = texture(gDepth, sampleUV).r;
        float linearSampledDepth = LinearizeDepth(sampledDepth);
        float sampleViewZ = -linearSampledDepth; // Görüş uzayı Z'si (negatif)
        
        // Mesim kontrolü (artifacts önlemek için)
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleViewZ));
        
        // DÜZELTME: Negatif Z değerleriyle karşılaştır
        if (sampleViewZ >= samplePos.z + bias) {
            occlusion += 1.0 * rangeCheck;
        }
    }
    
    occlusion = 1.0 - (occlusion / 64.0);
    occlusion = pow(occlusion, intensity);  // <-- Güç eğrisi uygula
    FragColor = occlusion;
}