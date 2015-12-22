#version 330
in vec2 position;
in vec3 color;
in vec2 texCoord;

out vec3 vertColor;
out vec2 vertTexCoord;

void main() {
    vertColor = color;
    vertTexCoord = texCoord;
    gl_Position = vec4(position, 0.0, 1.0);
}
