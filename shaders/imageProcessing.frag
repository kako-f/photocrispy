#version 430 core
in vec2 textureCoordinates;
out vec4 fragmentColor;

uniform sampler2D sourceImage;
uniform float exposure;
uniform vec3 whiteBalance;

void main(){
    vec3 source = texture(sourceImage, textureCoordinates).rgb;
    vec3 adjusted = source * exp2(exposure) * whiteBalance;
    fragmentColor = vec4(clamp(adjusted, 0.0, 1.0), 1.0);
}