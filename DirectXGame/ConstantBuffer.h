#pragma once
#include <d3d12.h>

class ConstantBuffer {
public:
	// 定数バッファの作成（バイトサイズ指定）
	void Create(UINT size);

	// 定数バッファのマッピング先ポインタ取得
	void* Map();

	// 定数バッファのゲッター
	ID3D12Resource* Get();
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress();

	// コンストラクタ / デストラクタ
	ConstantBuffer();
	~ConstantBuffer();

private:
	ID3D12Resource* constantBuffer_ = nullptr; // 定数バッファリソース
	void* mappedPtr_ = nullptr;                // マッピング先アドレス
};
