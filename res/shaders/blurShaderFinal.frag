#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform bool bloom;
uniform float exposure;

void main()
{             
    const float gamma = 2.2;
    vec3 pbrColor = texture(scene, TexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb*0.1;
    if(bloom)
        pbrColor += bloomColor; // additive blending
    // tone mapping
    vec3 result = vec3(1.0) - exp(-pbrColor * exposure);
    // also gamma correct while we're at it       
    result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0);
}