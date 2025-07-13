#include "helper.hlsli"

uniform vec3 eye;
varying vec3 position;
const vec3 underwaterColor = vec3(0.4, 0.9, 1.0);

void main() {
    vec2 coord = position.xz * 0.5 + 0.5;
    vec4 info = texture2D(water, coord);

    /* make water look more "peaked" */
    for (int i = 0; i < 5; i++) {
        coord += info.ba * 0.005;
        info = texture2D(water, coord);
    }

    vec3 normal = vec3(info.b, sqrt(1.0 - dot(info.ba, info.ba)), info.a);
    vec3 incomingRay = normalize(position - eye);

    // @todo avove and below water are almost the same maybe we can merge the shaders
    normal = -normal;
    vec3 reflectedRay = reflect(incomingRay, normal);
    vec3 refractedRay = refract(incomingRay, normal, IOR_WATER / IOR_AIR);
    float fresnel = mix(0.5, 1.0, pow(1.0 - dot(normal, -incomingRay), 3.0));

    vec3 reflectedColor = getSurfaceRayColor(position, reflectedRay, underwaterColor);
    vec3 refractedColor = getSurfaceRayColor(position, refractedRay, vec3(1.0)) * vec3(0.8, 1.0, 1.1);

    gl_FragColor = vec4(mix(reflectedColor, refractedColor, (1.0 - fresnel) * length(refractedRay)), 1.0);
}