/*

  NOTE: References

    - https://learnopengl.com/Lighting/Basic-Lighting
    - https://learnopengl.com/Advanced-Lighting/Advanced-Lighting

    This model for lighting models light with 3 parameters that get added together. These are:

    1) Ambient Light
    2) Diffuse Light
    3) Specular Light

    When talking about the above light types, we will be talking about it as a surface point relative to some light source.
    
    Ambient Light is modeled as a flat color that is applied to everything regardless of where it is on the surface of the object. The idea
    is that this is the light that bounces around the scene and hits the surface of everything in the environment. You can make this light
    more realistic via ambient occlusion.

    Diffuse Light is essentially directional light. The surface gets more diffuse light the closer it is relative to the light, and we take
    into account the normal of the surface relative to the light direction. If the surface is facing away from the light, it proportionally
    loses light via dot product (the more perpendicular the directions are, the closer to 0 influence we get from the light). In the model,
    difufse light is modeled to scatter equally in all directions of the surface point, but its modulated by the angles as mentioned.

    Specular Light is a small bulb of light that reflects like a mirror. So if we are looking at its perfect reflection, then we see a strong
    specular color. But the range it reflects is small so this is view dependent (unlike diffuse which is assumed to reflect equally in all
    directions from the surface, this bulb goes in a small range). This is the white reflection you see in materials. We model specular by
    reflecting the light dir along the normal, dot'ing it to our eye vector, and taking it to a high power. This means the model says that
    we get exponentially more light as we get closer to looking at the perfect reflection, and exponentially less elsewhere.

    The above models phong lighting but it has a issue with specular. The dot product becomes negative if the angle between view and
    reflection is > 90 degrees. We clamp to 0 but if specular power is low, this will clip the specular results and give us a hard edge
    around them. The idea is to construct a halfway vector which is a rotated normal vector so that the angle between view halfways and
    light halfway equals. This way, our dot is always <= 90 degrees. We can get our specular intensity by taking dot between halfway and
    normal. This does change the actual lighting a bit but is more visually plausible.
  
 */

vec3 BlinnPhongLighting(vec3 CameraView,
                        vec3 SurfaceColor, vec3 SurfaceNormal, float SurfaceSpecularPower,
                        vec3 LightDir, vec3 LightColor)
{
    // IMPORTANT: We assume LightDir is pointing from the surface to the light
    vec3 Result = vec3(0);
    float LightIntensity = 0.0f;
    
    // NOTE: Diffuse Light
    {
        float DiffuseIntensity = max(dot(-LightDir, SurfaceNormal), 0.0);
        LightIntensity += DiffuseIntensity;
    }

    // NOTE: Specular Light
    {
        vec3 HalfwayDir = normalize(-LightDir + CameraView);
        float SpecularIntensity = pow(max(dot(SurfaceNormal, HalfwayDir), 0.0), SurfaceSpecularPower);
        LightIntensity += SpecularIntensity;
    }

    // NOTE: Light can only reflect the colors in the surface
    Result = LightIntensity * SurfaceColor * LightColor;
    
    return Result;
}
