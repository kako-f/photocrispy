#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D uTexture;
uniform float uHighlightFactor; // 0.0 = no change, >1.0 = brighten, <1.0 = recover
uniform float fs;
uniform float ls;

float srgb_l(float channel){
    if (channel <= 0.04045)
    {
        return channel / 12.92;
    }

    return pow((( channel + 0.055)/1.055),2.4);
}

float ytoL(float y){
    if(y <= (216.0/24389.0)){  // The CIE standard states 0.008856 but 216/24389 is the intent for 0.008856451679036
        return y * (24389.0 / 27.0);  // The CIE standard states 903.3, but 24389/27 is the intent, making 903.296296296296296
    }else{
        return pow(y,(1.0/3.0))*116.0-16.0;
    }    
}

void main() {

    vec4 color = texture(uTexture, TexCoords);

    // Compute simple luminance
    //float luminance = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    
    float r1 = srgb_l(color.r/255.0);
    float g1 = srgb_l(color.g/255.0);
    float b1 = srgb_l(color.b/255.0);

    float luminance = (0.2126 * ytoL(r1) + 0.7152 * ytoL(g1) + 0.0722 * ytoL(b1));

    // Make a mask: 0.0 for shadows/midtones, 1.0 for highlights
    float highlightMask = smoothstep(fs, ls, luminance);

    // Blend original + boosted version
    vec3 highlightAdjusted = mix(color.rgb, color.rgb * uHighlightFactor, highlightMask);

    FragColor = vec4(highlightAdjusted, color.a);
}


/* #version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D uTexture;
uniform float uBrightness;

void main() {
    
    vec4 color = texture(uTexture, TexCoords);

    FragColor = vec4(color.rgb * uBrightness, color.a);


}

 */