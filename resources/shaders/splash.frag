#version 430

in vec2 timeInfo;  // x-total lifetime; y-elapsed lifetime
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D spriteTexture;
uniform int totalFrames = 5;
uniform vec2 frameSize = vec2(0.2, 1.0);
uniform float alphaCutoff = 0.5;
uniform float speedFactor = 3.0;

void main() {
    float elapsedTime = timeInfo.x;
    float totalLifetime = timeInfo.y;

    float lifetimeProgress = elapsedTime / totalLifetime * speedFactor;
    float alpha = 1 - (elapsedTime / totalLifetime);
    if (alpha <= 0.0 || lifetimeProgress >= 1.0) {
        discard;
    }

    int currentFrame = int(lifetimeProgress * float(totalFrames)) % totalFrames;

    float frameOffset = float(currentFrame) / float(totalFrames);
    vec2 frameUV = vec2(frameOffset + TexCoords.x * frameSize.x, TexCoords.y * frameSize.y);

    vec4 texColor = texture(spriteTexture, frameUV);
    if (texColor.a < alphaCutoff) {
        discard;
    }

    FragColor = texColor;//vec4(texColor.rgb, texColor.a * alpha);
}