// if has derivative
#extension GL_OES_standard_derivatives : enablen

varying vec3 oldPos;
varying vec3 newPos;
varying vec3 ray;

void main() {
    /* if the triangle gets smaller, it gets brighter, and vice versa */
    // if has derivative
    float oldArea = length(dFdx(oldPos)) * length(dFdy(oldPos));
    float newArea = length(dFdx(newPos)) * length(dFdy(newPos));
    gl_FragColor = vec4(oldArea / newArea * 0.2, 1.0, 0.0, 0.0);

    // gl_FragColor = vec4(0.2, 0.2, 0.0, 0.0);

    vec3 refractedLight = refract(-light, vec3(0.0, 1.0, 0.0), IOR_AIR / IOR_WATER);

    /* compute a blob shadow and make sure we only draw a shadow if the player is blocking the light */
    vec3 dir = (sphereCenter - newPos) / sphereRadius;
    vec3 area = cross(dir, refractedLight);
    float shadow = dot(area, area);
    float dist = dot(dir, -refractedLight);
    shadow = 1.0 + (shadow - 1.0) / (0.05 + dist * 0.025);
    shadow = clamp(1.0 / (1.0 + exp(-shadow)), 0.0, 1.0);
    shadow = mix(1.0, shadow, clamp(dist * 2.0, 0.0, 1.0));
    gl_FragColor.g = shadow;

    /* shadow for the rim of the pool */
    vec2 t = intersectCube(newPos, -refractedLight, vec3(-1.0, -poolHeight, -1.0), vec3(1.0, 2.0, 1.0));
    gl_FragColor.r *= 1.0 / (1.0 + exp(-200.0 / (1.0 + 10.0 * (t.y - t.x)) * (newPos.y - refractedLight.y * t.y - 2.0 / 12.0)));
}