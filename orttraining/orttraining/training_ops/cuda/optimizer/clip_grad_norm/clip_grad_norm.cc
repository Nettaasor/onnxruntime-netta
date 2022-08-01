// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <memory>
#include <utility>

#include "orttraining/training_ops/cuda/optimizer/clip_grad_norm/clip_grad_norm.h"
#include "orttraining/training_ops/cuda/reduction/reduction_all_impl.h"
#include "orttraining/training_ops/cuda/optimizer/clip_grad_norm/clip_grad_norm_impl.h"

namespace onnxruntime {
namespace cuda {

namespace {

constexpr int ChunkSize = 2048 * 32;
constexpr float Epsilon = 0.000001f;

void GetGroupedTensors(const TensorSeq* gradients, std::vector<int>* tensor_sizes,
                       std::vector<std::vector<void*>>* grouped_tensor_pointers) {
  for (size_t i = 0; i < gradients->Size(); ++i) {
    (*tensor_sizes)[i] = static_cast<int>(gradients->Get(i).Shape().Size());
    (*grouped_tensor_pointers)[i] = {const_cast<float*>(gradients->Get(i).Data<float>())};
  }
}

Status GetL2Norm(cudaStream_t stream, std::vector<int>& tensor_sizes,
                 std::vector<std::vector<void*>>& grouped_tensor_pointers, float** l2_norm) {
  CUDA_RETURN_IF_ERROR(cudaMemsetAsync(*l2_norm, 0, sizeof(float), stream));
  MultiTensorReduceL2<float, float> multi_tensor_reduce_l2_functor;
  launch_multi_tensor_functor<ClipGradNormGroupSize, MultiTensorReduceL2<float, float>>(
      stream, ChunkSize, tensor_sizes, grouped_tensor_pointers, multi_tensor_reduce_l2_functor, *l2_norm);

  ScalarSqrt(stream, *l2_norm, *l2_norm);

  return Status::OK();
}

Status PopulateOutput(cudaStream_t stream, AllocatorPtr alloc, const TensorSeq* gradients,
                      TensorSeq** clipped_gradients) {
  if (const_cast<TensorSeq*>(gradients) == *clipped_gradients) {
    return Status::OK();
  }

  (*clipped_gradients)->SetType(gradients->DataType());
  (*clipped_gradients)->Reserve(gradients->Size());
  for (size_t gradient_idx = 0; gradient_idx < gradients->Size(); ++gradient_idx) {
    const Tensor& source_tensor = gradients->Get(gradient_idx);
    std::unique_ptr<Tensor> target_tensor = Tensor::Create(source_tensor.DataType(),
                                                           source_tensor.Shape(), alloc);
    CUDA_RETURN_IF_ERROR(cudaMemcpyAsync(target_tensor->MutableDataRaw(),
                                         source_tensor.DataRaw(),
                                         source_tensor.SizeInBytes(),
                                         cudaMemcpyDeviceToDevice, stream));
    (*clipped_gradients)->Add(std::move(*target_tensor));  // Add will check for type consistency
  }

  return Status::OK();
}

}  // namespace

ONNX_OPERATOR_KERNEL_EX(
    ClipGradNorm,
    kMSDomain,
    1,
    kCudaExecutionProvider,
    (*KernelDefBuilder::Create())
        .Alias(0, 0) /* Return updated gradients in-place */
        .TypeConstraint("S_GRAD", DataTypeImpl::AllFixedSizeSequenceTensorTypes()),
    ClipGradNorm);

Status ClipGradNorm::ComputeInternal(OpKernelContext* ctx) const {
  // Prepare the inputs
  const TensorSeq* gradients = ctx->Input<TensorSeq>(0);
  std::vector<int> tensor_sizes(gradients->Size());
  std::vector<std::vector<void*>> grouped_tensor_pointers(gradients->Size());
  GetGroupedTensors(gradients, &tensor_sizes, &grouped_tensor_pointers);

  AllocatorPtr alloc;
  ORT_ENFORCE(ctx->GetTempSpaceAllocator(&alloc).IsOK(), "ClipGradNorm: Unable to get an allocator.");

  // Get frobenius norm for the grouped inputs
  float* l2_norm = reinterpret_cast<float*>(alloc->Alloc(sizeof(float)));
  ORT_RETURN_IF_ERROR(GetL2Norm(Stream(), tensor_sizes, grouped_tensor_pointers, &l2_norm));

  // Perform gradient clipping
  ClipGradNormFunctor<float> clip_grad_functor;
  launch_multi_tensor_functor<ClipGradNormGroupSize, decltype(clip_grad_functor)>(
      Stream(), ChunkSize, tensor_sizes, grouped_tensor_pointers, clip_grad_functor, l2_norm,
      Epsilon, max_norm_);

  // Populate the output sequence tensors.
  TensorSeq* clipped_gradients = ctx->Output<TensorSeq>(0);
  ORT_RETURN_IF_ERROR(PopulateOutput(Stream(), alloc, gradients, &clipped_gradients));

  return Status::OK();
}

}  // namespace cuda
}  // namespace onnxruntime
