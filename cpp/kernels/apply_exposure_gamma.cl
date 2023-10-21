__kernel void apply_exposure_gamma(
    __global float* pixels, 
    const unsigned int count,
    __global float* params, 
    const unsigned int param_count) 
{
    // Ensure that the correct number of parameters are provided
    // Here, we expect 2 parameters: [gamma, exposure]
    if(param_count != 2) {
        return;
    }

    int gid = get_global_id(0);
    if(gid < count) {
        float exposure = params[0];
        float inv_gamma = params[1];
        
        pixels[gid] = pixels[gid] * pow(2.0f, exposure); // Apply exposure correction first
        pixels[gid] = pow(pixels[gid], inv_gamma); // Apply gamma correction then
    }
}