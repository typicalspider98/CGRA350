#version 430

struct Splash {
    vec4 position;
    vec4 lifetime;  // x-total lifetime; y-left lifetime
};

layout(std430, binding = 1) buffer SplashBuffer {
    Splash splashes[];
};

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform sampler2D spriteTexture;

layout(location = 0) in vec2 inQuad;
out vec2 timeInfo;  // x-total lifetime; y-elapsed lifetime
out vec2 TexCoords;

void main() {
    float totalLifetime = splashes[gl_InstanceID].lifetime.x;
    float remainingLifetime = splashes[gl_InstanceID].lifetime.y;
    float elapsedTime = totalLifetime - max(remainingLifetime, 0.0);
    timeInfo = vec2(elapsedTime, totalLifetime);
    TexCoords = inQuad * 0.5 + 0.5;

    if (remainingLifetime > 0.0) {
        float sizeFactor = remainingLifetime;
        float size = 0.5; //mix(1.0, 0.1, clamp(1.0 - sizeFactor, 0.0, 1.0));

        vec3 billboardPosition = splashes[gl_InstanceID].position.xyz + 
                                 (-cameraRight) * inQuad.x * size + 
                                 cameraUp * inQuad.y * size;

        gl_Position = projection * view * vec4(billboardPosition, 1.0);
    }
    else {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0); 
    }    
}