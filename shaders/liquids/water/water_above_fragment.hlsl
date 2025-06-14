#include "helper.hlsli"

uniform vec3 eye;
varying vec3 position;
const vec3 abovewaterColor = vec3(0.25, 1.0, 1.25);

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

    vec3 reflectedRay = reflect(incomingRay, normal);
    vec3 refractedRay = refract(incomingRay, normal, IOR_AIR / IOR_WATER);
    float fresnel = mix(0.25, 1.0, pow(1.0 - dot(normal, -incomingRay), 3.0));

    vec3 reflectedColor = getSurfaceRayColor(position, reflectedRay, abovewaterColor);
    vec3 refractedColor = getSurfaceRayColor(position, refractedRay, abovewaterColor);

    gl_FragColor = vec4(mix(refractedColor, reflectedColor, fresnel), 1.0);
}