#include <stdio.h>

#include <cuda.h>
#include <cuda_runtime.h>
#include <driver_functions.h>

#include <thrust/scan.h>
#include <thrust/device_ptr.h>
#include <thrust/device_malloc.h>
#include <thrust/device_free.h>

extern float toBW(int bytes, float sec);


/* Helper function to round up to a power of 2. 
 */
static inline int nextPow2(int n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}


__global__ void
exclusive_scan_downsweep_kernel(int* device_data, int length, int twod, int twod1) {
    int i = (blockIdx.x * blockDim.x + threadIdx.x) * twod1;
    int t_index = i + twod - 1;
    int second_index = i + twod1 - 1;

    if (i < length && t_index < length && second_index < length) {
        int t = device_data[t_index];
        device_data[t_index] = device_data[second_index];
        device_data[second_index] += t;
    }
}

__global__ void
exclusive_scan_upsweep_kernel(int* device_data, int length, int twod, int twod1) {
    int i = (blockIdx.x * blockDim.x + threadIdx.x) * twod1;
    int index_add_val = i + twod - 1;
    int index_add_to = i + twod1 - 1;

    if (i < length && index_add_val < length && index_add_to < length)
        device_data[index_add_to] += device_data[index_add_val];
}

__global__ void
set_last_to_zero(int * device_data, int N) {
    device_data[N-1] = 0;
}

__global__ void
set_peeks(int * y_vals, int * peek_sig, int N) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i == 0) peek_sig[i] = 0;
    if (i == N - 1) peek_sig[i] = 0;
    if (1 <= i && i < N - 1) {
      if (y_vals[i - 1] < y_vals[i] && y_vals[i] > y_vals[i+1])
        peek_sig[i] = 1;
      else peek_sig[i] = 0;
    }
}

__global__ void
write_peeks(int * peek_sigs_summed, int * peek_idxs, int N) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (1 <= i && i < N && peek_sigs_summed[i] != peek_sigs_summed[i - 1]){
        peek_idxs[peek_sigs_summed[i - 1]] = i - 1;
    }
}

void exclusive_scan(int* device_data, int length)
{
    /* TODO
     * Fill in this function with your exclusive scan implementation.
     * You are passed the locations of the data in device memory
     * The data are initialized to the inputs.  Your code should
     * do an in-place scan, generating the results in the same array.
     * This is host code -- you will need to declare one or more CUDA 
     * kernels (with the __global__ decorator) in order to actually run code
     * in parallel on the GPU.
     * Note you are given the real length of the array, but may assume that
     * both the data array is sized to accommodate the next
     * power of 2 larger than the input.
     */
    const int threadsPerBlock = 512;
    const int N = nextPow2(length);

    for (int twod = 1; twod < N; twod*=2)
    {
        int twod1 = twod * 2;
        // compute number of blocks and threads per block
        int blocks = ((length / twod1) + 1 + threadsPerBlock - 1) / threadsPerBlock;
        exclusive_scan_upsweep_kernel<<<blocks, threadsPerBlock>>>(
           device_data, N, twod, twod1);
        cudaDeviceSynchronize();
    }
    set_last_to_zero<<<1, 1>>>(device_data, N);
    cudaDeviceSynchronize();
    // downsweep phase.
    for (int twod = N/2; twod >= 1; twod /= 2)
    {
        int twod1 = twod * 2;
        int blocks = (length + (threadsPerBlock * twod1) - 1) / (threadsPerBlock * twod1);
        exclusive_scan_downsweep_kernel<<<blocks, threadsPerBlock>>>(
            device_data, N, twod, twod1);
        cudaDeviceSynchronize();
    }
}

/* This function is a wrapper around the code you will write - it copies the
 * input to the GPU and times the invocation of the exclusive_scan() function
 * above. You should not modify it.
 */
double cudaScan(int* inarray, int* end, int* resultarray)
{
    int* device_data;
    // We round the array size up to a power of 2, but elements after
    // the end of the original input are left uninitialized and not checked
    // for correctness. 
    // You may have an easier time in your implementation if you assume the 
    // array's length is a power of 2, but this will result in extra work on
    // non-power-of-2 inputs.
    int rounded_length = nextPow2(end - inarray);
    cudaMalloc((void **)&device_data, sizeof(int) * rounded_length);

    cudaMemcpy(device_data, inarray, (end - inarray) * sizeof(int), 
               cudaMemcpyHostToDevice);


    exclusive_scan(device_data, end - inarray);

    // Wait for any work left over to be completed.
    cudaDeviceSynchronize();
    double overallDuration = 0;
    
    cudaMemcpy(resultarray, device_data, (end - inarray) * sizeof(int),
               cudaMemcpyDeviceToHost);
    return overallDuration;
}

/* Wrapper around the Thrust library's exclusive scan function
 * As above, copies the input onto the GPU and times only the execution
 * of the scan itself.
 * You are not expected to produce competitive performance to the
 * Thrust version.
 */
double cudaScanThrust(int* inarray, int* end, int* resultarray) {

    int length = end - inarray;
    thrust::device_ptr<int> d_input = thrust::device_malloc<int>(length);
    thrust::device_ptr<int> d_output = thrust::device_malloc<int>(length);
    
    cudaMemcpy(d_input.get(), inarray, length * sizeof(int), 
               cudaMemcpyHostToDevice);


    thrust::exclusive_scan(d_input, d_input + length, d_output);

    cudaDeviceSynchronize();

    cudaMemcpy(resultarray, d_output.get(), length * sizeof(int),
               cudaMemcpyDeviceToHost);
    thrust::device_free(d_input);
    thrust::device_free(d_output);
    double overallDuration = 0;
    return overallDuration;
}



int find_peaks(int *device_input, int length, int *device_output) {
    /* TODO:
     * Finds all elements in the list that are greater than the elements before and after,
     * storing the index of the element into device_result.
     * Returns the number of peak elements found.
     * By definition, neither element 0 nor element length-1 is a peak.
     *
     * Your task is to implement this function. You will probably want to
     * make use of one or more calls to exclusive_scan(), as well as
     * additional CUDA kernel launches.
     * Note: As in the scan code, we ensure that allocated arrays are a power
     * of 2 in size, so you can use your exclusive_scan function with them if 
     * it requires that. However, you must ensure that the results of
     * find_peaks are correct given the original length.
     */    
     // The algorithm consists of setting peeks to 1 and non-peeks to 0.
     // The inclusive scan and remove the duplicates
    const int threadsPerBlock = 512;
    const int blocks = (length + threadsPerBlock - 1) / threadsPerBlock;
    int * peek_sig;
    cudaMalloc(&peek_sig, sizeof(int) * length);

    set_peeks<<<blocks, threadsPerBlock>>>(device_input, peek_sig, length);
    cudaDeviceSynchronize();
    int * peek_sigs_summed = peek_sig;

    exclusive_scan(peek_sigs_summed, length);
    cudaDeviceSynchronize();

    int * numb_peeks = (int *) malloc(sizeof(int));
    cudaMemcpy(numb_peeks, &peek_sigs_summed[length - 1], sizeof(int), cudaMemcpyDeviceToHost);

    write_peeks<<<blocks, threadsPerBlock>>>(peek_sigs_summed, device_output, length);
    cudaDeviceSynchronize();

    int numb_peeks_tmp = numb_peeks[0];
    free(numb_peeks);
    cudaFree(peek_sig);

    return numb_peeks_tmp;
}



/* Timing wrapper around find_peaks. You should not modify this function.
 */
double cudaFindPeaks(int *input, int length, int *output, int *output_length) {
    int *device_input;
    int *device_output;
    int rounded_length = nextPow2(length);
    cudaMalloc((void **)&device_input, rounded_length * sizeof(int));
    cudaMalloc((void **)&device_output, rounded_length * sizeof(int));
    cudaMemcpy(device_input, input, length * sizeof(int), 
               cudaMemcpyHostToDevice);

    
    int result = find_peaks(device_input, length, device_output);

    cudaDeviceSynchronize();

    *output_length = result;

    cudaMemcpy(output, device_output, length * sizeof(int),
               cudaMemcpyDeviceToHost);

    cudaFree(device_input);
    cudaFree(device_output);

    return 0;
}


void computeSection()
{
    // for fun, just print out some stats on the machine

    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);

    printf("---------------------------------------------------------\n");
    printf("Found %d CUDA devices\n", deviceCount);

    for (int i=0; i<deviceCount; i++)
    {
        cudaDeviceProp deviceProps;
        cudaGetDeviceProperties(&deviceProps, i);
        printf("Device %d: %s\n", i, deviceProps.name);
        printf("   SMs:        %d\n", deviceProps.multiProcessorCount);
        printf("   Global mem: %.0f MB\n",
               static_cast<float>(deviceProps.totalGlobalMem) / (1024 * 1024));
        printf("   CUDA Cap:   %d.%d\n", deviceProps.major, deviceProps.minor);
    }
    printf("---------------------------------------------------------\n"); 
}