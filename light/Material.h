
struct Material {
    v3_f32 diffuse;
    v3_f32 specular;
    v3_f32 refraction;
    float luminosity;
    float subsurface_scattering;
    float reflectivity;

    void* bump_map;
    void* normal_map;
    void* displacement_map; // parallax_map
};