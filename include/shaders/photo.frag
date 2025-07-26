#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D uTexture;

uniform float uHighlightFactor; 
uniform float uShadowFactor;  
uniform float uExposure;  
uniform float uSaturation; 
uniform float uContrast;  

// cutOff limits for shadows and highlights
uniform float uShadowCutoffLum;
uniform float uShadowFullLum;

uniform float uHighlightLowLum;
uniform float uHighlightHighLum;

// Send this function a decimal sRGB gamma encoded color value
// between 0.0 and 1.0, and it returns a linearized value.
// https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color/37624009#37624009
float srgbToLin(float channel){

    if (channel <= 0.04045)
    {
        return channel / 12.92;
    }

    return pow((( channel + 0.055)/1.055),2.4);
}

// Send this function a luminance value between 0.0 and 1.0,
// and it returns L* which is "perceptual lightness"
// https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color/37624009#37624009
float ytoL(float y){

    if(y <= (216.0/24389.0)){  // The CIE standard states 0.008856 but 216/24389 is the intent for 0.008856451679036
        return y * (24389.0 / 27.0);  // The CIE standard states 903.3, but 24389/27 is the intent, making 903.296296296296296
    }else{
        return pow(y,(1.0/3.0))*116.0-16.0;
    }    
}
// Perceptual Lightness to luminance.
float Ltoy(float L_star){
    const float L_star_cutoff = 8.0; // Corresponds to linear luminance of ~0.008856
    if (L_star <= (216.0/24389.0)){
        return L_star * (27.0 / 24389.0);
    } else {
        return pow(((L_star + 16.0) / 116.0), 3.0);
    }
}
// From Linear values (0.0 - 1.0) to SRG
vec4 fromLinear(vec4 linearRGB)
{
    bvec3 cutoff = lessThan(linearRGB.rgb, vec3(0.0031308));
    vec3 higher = vec3(1.055)*pow(linearRGB.rgb, vec3(1.0/2.4)) - vec3(0.055);
    vec3 lower = linearRGB.rgb * vec3(12.92);

    return vec4(mix(higher, lower, cutoff), linearRGB.a);
}
// basic tonemap for srgb data
vec3 tonemapReinhard(vec3 v) { 
    return v / (1.0 + dot(v, vec3(0.21250175, 0.71537574, 0.07212251))); 
}

vec3 tonemapFilmic(vec3 v) {
    v = max(vec3(0.0), v - 0.004);
    v = (v * (6.2 * v + 0.5)) / (v * (6.2 * v + 1.7) + 0.06);
    return v;
}
// Shadows low - high threshold.
// Perceptual low threshold (on L* scale 0-100) = 0.1 * 100 = 10
// Perceptual high threshold (on L* scale 0-100) = 0.4 * 100 = 40
vec3 adjustShadows(vec3 linearColor, float shadowFactor, float shadowLowThreshold, float shadowLightThreshold)
{

    float luminance = (0.2126 * ytoL(linearColor.r) + 0.71529 * ytoL(linearColor.g) + 0.0722 * ytoL(linearColor.b));

    // low - high / threshold
    float shadowMask = smoothstep(shadowLowThreshold*100.0, shadowLightThreshold*100.0, luminance);
    // Note: smoothstep(edge0, edge1, x)
    // If x <= edge0, returns 0. If x >= edge1, returns 1.
    // So, if shadowLightthreshold (e.g., 40) is edge0, and shadowLowThreshold (e.g., 10) is edge1,
    // then if luminance = 50, it's > 40, returns 0.
    // If luminance = 5, it's < 10, returns 1.
    // This is the correct way to get a mask that is 1 for dark values and 0 for bright values.  
    vec3 shadowAdjusted = mix(linearColor, linearColor * shadowFactor, shadowMask);
    
    return shadowAdjusted;
    
}
    
// Perceptual low threshold (on L* scale 0-100) = 0.1 * 100 = 10
// Perceptual high threshold (on L* scale 0-100) = 0.9 * 100 = 90
vec3 adjustHighlights(vec3 linearColor, float highlightFactor, float highLowThres, float highHighThres){

    // Luminance calculation from linear_color_input to L*
    float luminance = (0.2126 * ytoL(linearColor.r) + 0.71529 * ytoL(linearColor.g) + 0.0722 * ytoL(linearColor.b));

    float highlightMask = smoothstep(highLowThres*100.0, highHighThres*100.0, luminance);  
    // Blend original + boosted version
    vec3 highlightAdjusted = mix(linearColor, linearColor * highlightFactor, highlightMask);

    return highlightAdjusted;
}

vec3 adjustExposure(vec3 color, float exposure) {
    return color * pow(2.0, exposure);
}
vec3 adjustSaturation(vec3 color, float saturation) {
     // Luminance - standard Rec. 709/sRGB 
    const vec3 luminanceWeighting = vec3(0.2126, 0.71529, 0.0722);
    vec3 grayscaleColor = vec3(dot(color, luminanceWeighting));
    return mix(grayscaleColor, color, 1.0 + saturation);
}

// Saturation with perceptual lightness . To verify
/* vec3 adjustSaturation(vec3 color, float saturation) {
    // Luminance - standard Rec. 709/sRGB 
    const vec3 luminanceWeighting = vec3(0.2126, 0.71529, 0.0722);
    float linearLuminance = dot(color, luminanceWeighting); 
    float L_star = ytoL(linearLuminance); // L* is in 0-100 range
    float grayscaleLinearLuminance = Ltoy(L_star);
    vec3 grayscaleColor = vec3(grayscaleLinearLuminance); // All channels equal to the linear luminance derived from L*
    return mix(grayscaleColor, color, 1.0 + saturation);
}
*/
// TODO - A perceptual lightness contrast could be better.
vec3 adjustContrast(vec3 color, float contrast) {
    return 0.5 + (contrast+1.0) * (color - 0.5);
}

void main() {

    vec4 color = texture(uTexture, TexCoords);

    vec3 colorLinear = vec3(
        srgbToLin(color.r),
        srgbToLin(color.g),
        srgbToLin(color.b)
    );
    // --- Exposure Adjustment ---
    colorLinear = adjustExposure(colorLinear, uExposure);
    // --- Saturation Adjustment ---
    colorLinear = adjustSaturation(colorLinear, uSaturation);
    // --- Contrast Adjustment ---
    colorLinear = adjustContrast(colorLinear, uContrast);
    // --- Shadow Adjustment ---
    colorLinear = adjustShadows(colorLinear, uShadowFactor, uShadowCutoffLum, uShadowFullLum);
    // --- Highlight Adjustment ---
    colorLinear = adjustHighlights(colorLinear, uHighlightFactor, uHighlightLowLum, uHighlightHighLum);
    // --- Tonemapping ---
    vec3 tonemappedLinear = tonemapReinhard(colorLinear);
    // --- Gamma Correction for Display ---
    vec4 finalSrgbGamma = fromLinear(vec4(tonemappedLinear, 1.0));
    
    FragColor = finalSrgbGamma;
}
