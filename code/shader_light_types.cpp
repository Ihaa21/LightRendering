
struct directional_light
{
    vec3 Color;
    vec3 Dir;
    vec3 AmbientLight;
};

struct point_light
{
    vec3 Color;
    vec3 Pos; // NOTE: Camera Space Position
    float MaxDistance; // TODO: Rename to radius
};

vec3 PointLightAttenuate(vec3 SurfacePos, point_light Light)
{
    vec3 Result = vec3(0);

    /*
    // NOTE: This is regular attenuation model
    float Distance = length(Light.Pos - SurfacePos);
    float Attenuation = 1.0 / (Distance * Distance);
    Result = Light.Color * Attenuation;
    */

    // NOTE: This is a sorta fake attenuation model but gives a more exact sphere size
    float Distance = length(Light.Pos - SurfacePos);
    float PercentDist = clamp((Light.MaxDistance - Distance) / Light.MaxDistance, 0, 1);
    Result = Light.Color * PercentDist;
    
    return Result;
}

