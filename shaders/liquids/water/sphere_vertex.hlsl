uniform vec3 sphereCenter;
uniform float sphereRadius;
varying vec3 position;

void main() {
    position = sphereCenter + gl_Vertex.xyz * sphereRadius;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0);
}