#include "RootSignature.h"
#include"KamataEngine.h"

using namespace KamataEngine;

//RootSignature生成
void RootSignature::Create() {
	//既にインスタンスがあるなら解放する(Create関数が二回以上実行されたとき用)
	if (rootSignature_) {
		rootSignature_->Release();
		rootSignature_ = nullptr;
	}

	//クラス内で取得するために追加
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// RootSignature作成
	// 構造体にデータを用意する
	D3D12_ROOT_SIGNATURE_DESC descripttionRootSignature{};
	descripttionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	
	//デスクリプタレンジ
	D3D12_DESCRIPTOR_RANGE srvDescRange[2]{};
	//t0レジスタを利用可能にする
	srvDescRange[0].BaseShaderRegister = 0;
	srvDescRange[0].NumDescriptors = 1;
	srvDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvDescRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	srvDescRange[1].BaseShaderRegister = 1;
	srvDescRange[1].NumDescriptors = 1;
	srvDescRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvDescRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	//RootParameterの用意 pixelShaderに読ませるために実用
	//複数指定できるので配列の構造をしている。今回は一つだけなので長さ1の配列として用意する
	D3D12_ROOT_PARAMETER rootParameters[2]{};

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].DescriptorTable.pDescriptorRanges = srvDescRange;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(srvDescRange);

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[1].Descriptor.ShaderRegister = 0; 
	rootParameters[1].Descriptor.RegisterSpace = 0;

	descripttionRootSignature.pParameters = rootParameters;
	descripttionRootSignature.NumParameters = _countof(rootParameters);
	
	//Samplerの実装
	D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
	staticSamplers[0].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	staticSamplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
	staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[1].ShaderRegister = 1;
	staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descripttionRootSignature.pStaticSamplers = staticSamplers;
	descripttionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlog = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&descripttionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlog);

	if (FAILED(hr)) {
		DebugText::GetInstance()->ConsolePrintf(reinterpret_cast<char*>(errorBlog->GetBufferPointer()));

		assert(false);
	}

	// バイナリをもとに作成
	ID3D12RootSignature* rootSignature = nullptr;
	hr = dxCommon->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

	//signatureBlobはRootSignatureの生成後解放してもいい
	signatureBlob->Release();

	//生成したRootSignatureを取っておく
	rootSignature_ = rootSignature;
	
}


ID3D12RootSignature* RootSignature::Get() { return rootSignature_; }

RootSignature::RootSignature() {}

RootSignature::~RootSignature() {
	if (rootSignature_){
		rootSignature_->Release();
		rootSignature_ = nullptr;
	} 
}
