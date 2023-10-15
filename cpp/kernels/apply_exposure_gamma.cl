__kernel void apply_exposure_gamma(
    __global float* pixels, 
    const unsigned int count,
    __global float* params, 
    const unsigned int param_count) 
{
    // Ensure that the correct number of parameters are provided
    // Here, we expect 2 parameters: [gamma, exposure]
    if(param_count != 2) {
        // Optionally handle error, e.g., by setting all pixels to an error value
        return;
    }

    int gid = get_global_id(0);
    if(gid < count) {
        float exposure = params[0];
        float inv_gamma = params[1];
        
        pixels[gid] = pixels[gid] * pow(2.0f, exposure); // Apply exposure correction first
        pixels[gid] = pow(pixels[gid], inv_gamma); // Apply gamma correction then
        // Apply corrections only to RGB, leave A as it is
        // for (int i = 0; i < 3; ++i) {  // Assuming RGB are contiguous in memory
        //     pixels[4*gid + i] = pixels[4*gid + i] * pow(2.0f, exposure);
        //     pixels[4*gid + i] = pow(pixels[4*gid + i], inv_gamma);
        // }
    }
}