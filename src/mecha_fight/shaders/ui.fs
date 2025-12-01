#version 330 core
out vec4 FragColor;

in vec2 uv;

uniform vec4 color;
uniform float fill; // 0..1 horizontal fill amount
uniform int useTexture; // 1 to sample texture, 0 for solid color
uniform sampler2D uTexture;

void main() {
    if (uv.x > fill) discard;

    if (useTexture == 1) {
        vec4 texColor = texture(uTexture, uv);
        FragColor = texColor * color;
    } else {
        FragColor = color;
    }
}
