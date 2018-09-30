#version 330 core

layout (location = 0) in vec3 aVertex;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out VS_OUT {
    vec3 fragPosition;
    vec3 fragNormal;
    vec2 texCoord;
} vs_out;

void main() {
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(aVertex, 1.0f);

    //transform positions and normals to world space
    vs_out.fragPosition = vec3(modelMatrix * vec4(aVertex, 1.0f));
    vs_out.fragNormal = vec3(modelMatrix * vec4(aNormal, 0.0f));
    vs_out.texCoord = aUV;
}