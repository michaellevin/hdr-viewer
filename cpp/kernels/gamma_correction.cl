__kernel void apply_gamma(__global float* pixels, const unsigned int count, const float inv_gamma)
{
    int id = get_global_id(0);
    if(id < count)
    {
        pixels[id] = pow(pixels[id], inv_gamma);
    }
} 