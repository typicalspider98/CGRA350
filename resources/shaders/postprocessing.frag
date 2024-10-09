#version 430

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D scene;         // The texture of the rendered scene

// Color Grading
uniform bool enableColorGrading; // Enable or disable color grading
uniform vec3 colorFilter;        // Color grading filter

// Dynamic Filter
uniform bool enableDynamicFilter; // Enable or disable dynamic filter
uniform float contrast;           // Contrast adjustment
uniform float brightness;         // Brightness adjustment

// Gaussian Blur
uniform bool enableGaussianBlur;  // Enable or disable Gaussian blur
uniform vec2 blurDirection;       // Blur direction (horizontal or vertical)
uniform float blurWeight[5];      // Gaussian weights
uniform vec2 blurOffset[5];       // Blur offsets

// Bloom Effect
uniform bool enableBloom;         // Enable or disable Bloom effect
uniform float bloomThreshold;     // Bloom threshold
uniform float bloomIntensity;     // Bloom intensity

void main()
{
    vec3 oringinColor = texture(scene, TexCoords).rgb;
    vec3 baseColor = oringinColor;

    // 1. Dynamic Filter
    if (enableDynamicFilter)
    {
        baseColor = baseColor + brightness;  // Adjust brightness
        baseColor = (baseColor - 0.5) * contrast + 0.5;  // Adjust contrast
    }

    // 2. Color Grading
    if (enableColorGrading)
    {
        baseColor = baseColor * colorFilter;  // Apply color filter
    }

    vec3 finalColor = baseColor;

    // 3. Gaussian Blur
    if (enableGaussianBlur)
    {
        vec3 result = oringinColor * blurWeight[0];
        for (int i = 1; i < 5; ++i)
        {
            result += texture(scene, TexCoords + blurOffset[i] * blurDirection).rgb * blurWeight[i];
            result += texture(scene, TexCoords - blurOffset[i] * blurDirection).rgb * blurWeight[i];
        }
        finalColor = mix(finalColor, result, 0.5);  // Apply the blur result (mixed with 50%)
    }

    // 4. Bloom Effect
    if (enableBloom)
    {
        // Extract bright regions
        vec3 bloomColor = max(vec3(0.0), baseColor - vec3(bloomThreshold));
        bloomColor *= bloomIntensity;

        // Add Bloom back to the original image
        finalColor += bloomColor;
    }

    FragColor = vec4(finalColor, 1.0);
}