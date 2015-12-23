#version 330
in vec3 position;
in vec3 color;
in vec2 texCoord;

out vec3 vertColor;
out vec2 vertTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main() {
    vertColor = color;
    vertTexCoord = texCoord;
    gl_Position = uProj * uView * uModel * vec4(position, 1.0);
}
