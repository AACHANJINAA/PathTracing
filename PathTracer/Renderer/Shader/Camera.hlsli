#ifndef CAMERA_HLSLI
#define CAMERA_HLSLI

// Pinhole camera (docs/RENDERING_MODEL.md section 2). Requires g_frame.
// pixel_position is in pixels with (0, 0) at the top-left corner of the image.
RAY generate_camera_ray(float pixel_x, float pixel_y)
{
    float ndc_x = 2.0f * pixel_x / float(g_frame.width) - 1.0f;
    float ndc_y = 1.0f - 2.0f * pixel_y / float(g_frame.height);
    float3 direction = g_frame.camera_forward
        + (ndc_x * g_frame.aspect * g_frame.tan_half_fov_y) * g_frame.camera_right
        + (ndc_y * g_frame.tan_half_fov_y) * g_frame.camera_up;
    return make_ray(g_frame.camera_position, normalize(direction));
}

#endif
