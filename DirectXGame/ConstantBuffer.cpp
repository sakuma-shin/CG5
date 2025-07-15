#include "ConstantBuffer.h"
#include "KamataEngine.h"

#include <cassert>
#include <d3d12.h>

using namespace KamataEngine;

ConstantBuffer::ConstantBuffer() {}

ConstantBuffer::~ConstantBuffer() {
	if (constantBuffer_) {
		constantBuffer_->Release();
		constantBuffer_ = nullptr;
	}
}

void ConstantBuffer::Create(UINT size) {

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// アップロードヒープ
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = size;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = dxCommon->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer_));
	assert(SUCCEEDED(hr));

	// マッピング
	hr = constantBuffer_->Map(0, nullptr, &mappedPtr_);
	assert(SUCCEEDED(hr));
}

void* ConstantBuffer::Map() { return mappedPtr_; }

ID3D12Resource* ConstantBuffer::Get() { return constantBuffer_; }

D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GetGPUVirtualAddress() { return constantBuffer_->GetGPUVirtualAddress(); }
