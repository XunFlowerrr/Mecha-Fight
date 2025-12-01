#version 330 core
layout (location = 0) in vec2 aPos; // in [0,1]

uniform vec2 rectPos;      // pixels (top-left)
uniform vec2 rectSize;     // pixels (width,height)
uniform vec2 screenSize;   // pixels

out vec2 uv;

void main() {
    vec2 pixelPos = rectPos + aPos * rectSize; // top-left origin
    // convert to NDC. Note: OpenGL origin bottom-left, our rectPos uses top-left
    float ndcX = (pixelPos.x / screenSize.x) * 2.0 - 1.0;
    float ndcY = 1.0 - (pixelPos.y / screenSize.y) * 2.0;
    gl_Position = vec4(ndcX, ndcY, 0.0, 1.0);
    uv = aPos;
}
