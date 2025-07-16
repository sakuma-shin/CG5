#include "PipelineState.h"
#include "KamataEngine.h"

using namespace KamataEngine;

void PipelineState::Create(D3D12_GRAPHICS_PIPELINE_STATE_DESC desc) {
	// クラス内で取得するために追加
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// PSOを生成する
	ID3D12PipelineState* graphicsPipeLineState = nullptr;
	[[maybe_unused]]HRESULT hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&graphicsPipeLineState));
#ifdef DEBUG



	assert(SUCCEEDED(hr));
#endif // DEBUG

	//生成したPipelineStateを取っておく
	pipelineState_ = graphicsPipeLineState;
}

//生成したPipelineStateを返す
ID3D12PipelineState* PipelineState::Get() { return pipelineState_; }

//コンストラクタ
PipelineState::PipelineState() {}

//デストラクタ
PipelineState::~PipelineState() {
	if (pipelineState_) {
		pipelineState_->Release();
		pipelineState_ = nullptr;
	}
}
