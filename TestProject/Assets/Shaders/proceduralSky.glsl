#shader vertex
#version 460 core
layout (location = 0) in vec2 aPos;

uniform mat4 invProjection;
uniform mat4 invView;

out vec3 worldDir;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vec4 viewPos = invProjection * vec4(aPos, 1.0, 1.0);
    viewPos /= viewPos.w;
    vec4 worldPos = invView * vec4(viewPos.xyz, 0.0);
    worldDir = normalize(worldPos.xyz);
}

#shader fragment
#version 460 core
in vec3 worldDir;

uniform vec3 lightDirection = vec3(-0.5, -1.0, 0.0); // Normalize edilmiş yön
uniform float time; // Uygulama tarafından güncellenen zaman

out vec4 FragColor;

// Gerekli yardımcı fonksiyonlar
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 permute(vec4 x) { return mod289(((x*34.0)+1.0)*x); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

// Optimize edilmiş 3D Simplex Gürültü
float snoise(vec3 v) {
    const vec2 C = vec2(1.0/6.0, 1.0/3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);

    // İlk köşe
    vec3 i  = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);

    // Diğer köşeler
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);

    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;

    // Permütasyonlar
    i = mod289(i);
    vec4 p = permute(permute(permute(
             i.z + vec4(0.0, i1.z, i2.z, 1.0))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0))
           + i.x + vec4(0.0, i1.x, i2.x, 1.0));

    // Gradyan hesaplama
    float n_ = 0.142857142857;
    vec3  ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);

    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_);

    vec4 x = x_ * ns.x + ns.yyyy;
    vec4 y = y_ * ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);

    vec4 s0 = floor(b0) * 2.0 + 1.0;
    vec4 s1 = floor(b1) * 2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);

    // Normalizasyon
    vec4 norm = taylorInvSqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    // Nokta çarpımları
    vec4 m = max(0.6 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0);
    m = m * m;
    return 42.0 * dot(m * m, vec4(dot(p0, x0), dot(p1, x1), 
                                dot(p2, x2), dot(p3, x3)));
}

// Fraktal Brownian Motion (FBM) - Bulut dokusu için
float fbm(vec3 p, int octaves) {
    float total = 0.0;
    float frequency = 1.0;
    float amplitude = 1.0;
    float maxAmplitude = 0.0;
    
    for(int i = 0; i < octaves; i++) {
        total += snoise(p * frequency) * amplitude;
        maxAmplitude += amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return total / maxAmplitude;
}



vec3 calculateSkyColor(vec3 dir, vec3 lightDir) {
    vec3 nDir = normalize(dir);
    vec3 nLightDir = normalize(-lightDir);
    
    float isDay = smoothstep(-0.1, 0.1, nLightDir.y);
    float isNight = 1.0 - isDay;
    
    vec3 zenithColorDay = vec3(0.2, 0.4, 0.9);
    vec3 horizonColorDay = vec3(0.9, 0.9, 1.0);
    vec3 zenithColorNight = vec3(0.02, 0.02, 0.05);
    vec3 horizonColorNight = vec3(0.05, 0.05, 0.1);
    
    vec3 zenithColor = mix(zenithColorNight, zenithColorDay, isDay);
    vec3 horizonColor = mix(horizonColorNight, horizonColorDay, isDay);
    
    float height = (nDir.y + 1.0) / 2.0;
    float horizonBlend = pow(smoothstep(0.0, 1.0, height), 0.1);
    vec3 skyColor = mix(horizonColor, zenithColor, horizonBlend);

    // Günbatımı/gündoğumu efektleri
    float reddening = 1.0 - smoothstep(0.0, 0.5, abs(nLightDir.y));
    vec3 sunsetColor = mix(vec3(1.0, 0.6, 0.6), vec3(1.0, 0.4, 0.2), step(0.0, -nLightDir.y));
    skyColor = mix(skyColor, sunsetColor, pow(reddening, 1.2) * (1.0 - height) * 0.7);
    
    // Güneş efekti
    float sun = max(dot(nDir, nLightDir), 0.0);
    vec3 sunColor = vec3(1.0, 0.9, 0.7) * pow(sun, 800.0) * isDay;
    vec3 glowColor = vec3(1.0, 0.8, 0.6) * pow(sun, 50.0) * 0.5 * isDay;
    
    // Ufuk parlaması
    float horizonGlow = exp(-pow((nDir.y) * 20.0, 2.0)) * 0.2;
    vec3 horizonHighlight = vec3(1.0, 0.7, 0.4) * horizonGlow * isDay;

    // Gece ufku
    vec3 darkHorizonColor = vec3(0.02, 0.02, 0.05);
    float fadeToDark = smoothstep(-0.05, 0.0, nDir.y);
    vec3 finalColor = mix(darkHorizonColor, skyColor + sunColor + glowColor + horizonHighlight, fadeToDark);

    // ================= DÜZELTİLMİŞ BULUT HESAPLAMASI =================
    const float cloudScale = 0.3;
    const float cloudSpeed = 0.01;
    const float cloudThreshold = 0.5; // Daha düşük eşik
    const float cloudSoftness = 0.2;
    
    // Animasyonlu bulut koordinatları
    vec3 cloudCoord = dir * (1.0 / cloudScale) + vec3(time * cloudSpeed, time * cloudSpeed * 0.5, 0.0);
    
    // Bulut yoğunluğu (3 oktav)
    float cloudDensity = fbm(cloudCoord, 3);
    cloudDensity = smoothstep(cloudThreshold, cloudThreshold + cloudSoftness, cloudDensity * 0.5 + 0.5);
    
    // Ufukta yoğunluk düşürme
    float horizonFade = smoothstep(0.0, 0.3, dir.y);
    cloudDensity *= horizonFade;
    
    // Işıklandırma efekti
    vec3 lightOffset = normalize(lightDir) * 0.25;
    float lightFactor = fbm(cloudCoord + lightOffset, 2);
    lightFactor = smoothstep(0.4, 0.8, lightFactor) * isDay * 0.8; // Gece tamamen kapat
    
    // Gece için ek karanlık faktör
    float nightDarkness = 0.1 + 0.9 * isDay; // Gece %10 parlaklık
    
    // Bulut renkleri (Gece daha koyu)
    vec3 cloudBase = mix(
        mix(vec3(0.15, 0.15, 0.2), vec3(0.7, 0.75, 0.8), nightDarkness), // Gece/Gündüz taban
        vec3(1.0, 0.97, 0.9), 
        lightFactor
    );
    
    // Günbatımı efekti (gece etkisiz)
    vec3 cloudColor = mix(
        cloudBase, 
        sunsetColor, 
        pow(lightFactor, 2.0) * reddening * 0.5 * isDay
    );
    
    // Son harmanlama (gece daha şeffaf)
    float blendFactor = cloudDensity  * nightDarkness;
    finalColor = mix(finalColor, cloudColor, blendFactor);

    return finalColor;
}

void main() {
    vec3 dir = normalize(worldDir);
    vec3 color = calculateSkyColor(dir, lightDirection);
    FragColor = vec4(color, 1.0);
}