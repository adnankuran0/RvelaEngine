#version 460 core
in vec3 worldDir;

uniform vec3 lightDirection = vec3(-60, -90, 0); 

out vec4 FragColor;

vec3 calculateSkyColor(vec3 dir, vec3 lightDir)
{
    vec3 nDir = normalize(dir);
    vec3 nLightDir = normalize(-lightDir); // Negate light direction to match sun position
    
    // Gün veya geceyi light'ın y bileşenine göre belirle
    float isDay = smoothstep(-0.1, 0.1, nLightDir.y);
    float isNight = 1.0 - isDay;
    
    // Gün ve gece için gökyüzü renkleri
    vec3 zenithColorDay = vec3(0.2, 0.4, 0.9);
    vec3 horizonColorDay = vec3(0.9, 0.9, 1.0);
    vec3 zenithColorNight = vec3(0.02, 0.02, 0.05);
    vec3 horizonColorNight = vec3(0.05, 0.05, 0.1);
    
    // Gün/gece geçişine göre renkleri interpolate et
    vec3 zenithColor = mix(zenithColorNight, zenithColorDay, isDay);
    vec3 horizonColor = mix(horizonColorNight, horizonColorDay, isDay);
    
    // Yükseklik faktörünü hesapla
    float height = (nDir.y + 1.0) / 2.0;
    
    // Gökyüzü renk gradyanı
    vec3 skyColor = mix(horizonColor, zenithColor, pow(smoothstep(0.0, 1.0, height),1/10));
    
    // Işık ufka yakınken kırmızılaşma efekti
    float reddeningFactor = 1.0 - smoothstep(0.0, 0.2, abs(nLightDir.y));
    vec3 sunsetColor = vec3(1.0, 0.5, 0.2);
    float sunsetAmount = pow(reddeningFactor, 1.5);
    skyColor = mix(skyColor, sunsetColor, sunsetAmount * (1.0 - height) * 0.5);
    
    // Güneş efekti
    float sun = max(dot(nDir, nLightDir), 0.0);
    vec3 sunColor = vec3(1.0, 0.9, 0.7) * pow(sun, 800.0) * isDay; // Dar güneş noktası
    vec3 glowColor = vec3(1.0, 0.8, 0.6) * pow(sun, 50.0) * 0.5 * isDay; // Güneş parlaması
    
    // Gökyüzü rengini güneş efektleriyle birleştir
    vec3 finalColor = skyColor + sunColor + glowColor;
    
    return finalColor;
}

void main()
{
    vec3 dir = normalize(worldDir);
    vec3 color = calculateSkyColor(dir, lightDirection);
    FragColor = vec4(color, 1.0);
}