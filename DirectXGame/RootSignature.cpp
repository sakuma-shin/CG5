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
