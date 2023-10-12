__kernel void apply_exposure(__global float* pixels, const unsigned int count, const float exposure)
{
    int id = get_global_id(0);
    if(id < count)
    {
        pixels[id] = pixels[id] * pow(2.0f, exposure);
    }
}