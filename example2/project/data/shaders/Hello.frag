#version 330
in vec3 vertColor;
in vec2 vertTexCoord;

uniform sampler2D ambientTex;

out vec4 fragColor;

void main() {
    fragColor = texture(ambientTex, vertTexCoord);
    
}
