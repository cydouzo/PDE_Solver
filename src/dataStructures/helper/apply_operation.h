#include <nvfunctional>

#include "dataStructures/array.hpp"

// typedef nvstd::function<T &> Apply;
// template <typename T1, typename T2> __global__ void inserter(T1 *f, T2 l) {
//     *f = l;
// }
// typedef void (*FunctionDev)(...);

template <typename Apply, typename C>
__global__ void ApplyFunctionK(D_Array<C> &vector, Apply func) {
    int i = threadIdx.x + blockIdx.x * blockDim.x;
    if (i >= vector.n)
        return;
    (func)(vector.data[i]);
    return;
}

template <typename Apply, typename C>
__host__ void ApplyFunction(D_Array<C> &vector, Apply func) {
    auto tb = Make1DThreadBlock(vector.n);
    ApplyFunctionK<<<tb.block, tb.thread>>>(*vector._device, func);
};

template <typename Apply, typename C>
__global__ void ApplyFunctionConditionalK(D_Array<C> &vector,
                                          D_Array<C> &booleans, Apply func) {
    int i = threadIdx.x + blockIdx.x * blockDim.x;
    if (i >= vector.n)
        return;
    if (booleans.data[i] > 0)
        (func)(vector.data[i]);
    return;
}

template <typename Apply, typename C>
__host__ void ApplyFunctionConditional(D_Array<C> &vector, D_Array<C> &booleans,
                                       Apply func) {
    auto tb = Make1DThreadBlock(vector.n);
    ApplyFunctionConditionalK<<<tb.block, tb.thread>>>(*vector._device,
                                                       *booleans._device, func);
};

template <typename Reduction, typename C>
__global__ void ReductionFunctionK(D_Array<C> &A, int nValues, int shift,
                                   Reduction func) {
    int i = threadIdx.x + blockIdx.x * blockDim.x;
    if (i >= nValues)
        return;
    for (int exp = 0; (1 << exp) < blockDim.x; exp++) {
        if (threadIdx.x % (2 << exp) == 0 &&
            threadIdx.x + (1 << exp) < blockDim.x && i + (1 << exp) < nValues) {
            A.data[shift * i] =
                func(A.data[shift * i], A.data[shift * (i + (1 << exp))]);
        }
        __syncthreads();
    }
};

template <typename Reduction, typename C>
C ReductionFunction(D_Array<C> &A, Reduction func) {
    int nValues = A.n;
    dim3Pair threadblock;
    int shift = 1;
    do {
        threadblock = Make1DThreadBlock(nValues);
        ReductionK<<<threadblock.block.x, threadblock.thread.x>>>(
            *A._device, nValues, shift, func);
        gpuErrchk(cudaDeviceSynchronize());
        nValues = int((nValues - 1) / threadblock.thread.x) + 1;
        shift *= threadblock.thread.x;
    } while (nValues > 1);
    return 0;
}

template <typename Reduction, typename C>
__global__ void ReductionFunctionConditionalK(D_Array<C> &A,
                                              D_Array<C> &booleans, int nValues,
                                              int shift, Reduction func) {
    int i = threadIdx.x + blockIdx.x * blockDim.x;
    if (i >= nValues)
        return;
    for (int exp = 0; (1 << exp) < blockDim.x; exp++) {
        if (threadIdx.x % (2 << exp) == 0 &&
            threadIdx.x + (1 << exp) < blockDim.x && i + (1 << exp) < nValues) {
            if (booleans.data[shift * (i + (1 << exp))] >
                0) // TODO Make this as array of bool
                if (booleans.data[shift * i] > 0)
                    A.data[shift * i] = func(A.data[shift * i],
                                             A.data[shift * (i + (1 << exp))]);
                else
                    A.data[shift * i] = A.data[shift * (i + (1 << exp))];
            booleans.data[shift * i] = booleans.data[shift * i] +
                                       booleans.data[shift * (i + (1 << exp))];
        }
        __syncthreads();
    }
};

template <typename Reduction, typename C>
C ReductionFunctionConditional(D_Array<C> &A, D_Array<C> &booleans,
                               Reduction func) {
    int nValues = A.n;
    dim3Pair threadblock;
    int shift = 1;
    do {
        threadblock = Make1DThreadBlock(nValues);
        ReductionFunctionConditionalK<<<threadblock.block.x,
                                        threadblock.thread.x>>>(
            *A._device, *booleans._device, nValues, shift, func);
        gpuErrchk(cudaDeviceSynchronize());
        nValues = int((nValues - 1) / threadblock.thread.x) + 1;
        shift *= threadblock.thread.x;
    } while (nValues > 1);
    return 0;
}
