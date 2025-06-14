#include "helper.hlsli"

varying vec3 position;
uniform sampler2D water;
const vec3 underwaterColor = vec3(0.4, 0.9, 1.0);

void main() {
    gl_FragColor = vec4(getSphereColor(position), 1.0);
    vec4 info = texture2D(water, position.xz * 0.5 + 0.5);
    if (position.y < info.r) {
        gl_FragColor.rgb *= underwaterColor * 1.2;
    }
}