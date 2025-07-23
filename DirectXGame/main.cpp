#include "ConstantBuffer.h"
#include "DirectXTex.h"
#include "IndexBuffer.h"
#include "MiscUtility.h"
#include "PipelineState.h"
#include "RootSignature.h"
#include "Shader.h"
#include "VertexBuffer.h"
#include "WorldTransformEX.h"
#include "kamataEngine.h"
#include <Windows.h>
#include <random>
#include<cassert>

using namespace KamataEngine;
using namespace MathUtility;

struct ViewData {
	Matrix4x4 InverseProjection;
};

struct RandomTime {
	float time;
};

struct Threshold {
	float threshold;
};

// 関数プロトタイプ宣言
void SetupPipeLineState(PipelineState& pipelineState, RootSignature& rs, Shader& vs, Shader& ps);
// RenderTexTureResourceの生成
ID3D12Resource* CreateRenderTextureResource(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, const FLOAT* clearColor);
// DepthStencilTextureResourceの生成
ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);

DirectX::ScratchImage LoadTexture(const std::string& filePath);

// std::wstring ConvertString(const std::string& str);
//
// std::string ConvertString(const std::wstring& str);

ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);

void UpLoadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	KamataEngine::Initialize(L"LE3C_10_サクマ_シン");

	// DirectXCommonインスタンスの取得
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// DirectXCommonクラスが管理している、ウインドウの幅と高さの取得
	int32_t w = dxCommon->GetBackBufferWidth();
	int32_t h = dxCommon->GetBackBufferHeight();
	DebugText::GetInstance()->ConsolePrintf(std::format("width:{},height: {}\n", w, h).c_str());

	// DirectXCommonクラスが管理している、コマンドリストの取得
	ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

	// RootSignature作成
	RootSignature rs;
	rs.Create();

	// 頂点シェーダーの読み込みとコンパイル
	Shader vs;
	vs.LoadDxc(L"Resources/shaders/TestVS.hlsl", L"vs_6_0");
#ifdef DEBUG
	assert(vs.GetDxcBlob() != nullptr);
#endif // DEBUG

	// ピクセルシェーダーの読み込みとコンパイル

	const int kNumPS = 10;
	Shader ps[kNumPS];
	const std::wstring PS[kNumPS] = {
	    L"Resources/shaders/TestPS.hlsl",
	    L"Resources/shaders/GrayScalePS.hlsl",
	    L"Resources/shaders/VignettePS.hlsl",
	    L"Resources/shaders/BoxFilterPS.hlsl",
	    L"Resources/shaders/GaussianFilterPS.hlsl",
	    L"Resources/shaders/LuminanceBasedOutlinePS.hlsl",
	    L"Resources/shaders/DepthBasedOutlinePS.hlsl",
	    L"Resources/shaders/RadialBlurPS.hlsl",
	    L"Resources/shaders/RandomPS.hlsl",
	    L"Resources/shaders/DissolvePS.hlsl",
	};
	// pipelineStateの作成
	PipelineState pipelineState[kNumPS];

	for (int i = 0; i < kNumPS; i++) {
		ps[i].LoadDxc(PS[i], L"ps_6_0");
#ifdef DEBUG
		assert(ps[i].GetDxcBlob() != nullptr);
#endif // DEBUG
		SetupPipeLineState(pipelineState[i], rs, vs, ps[i]);
	}

	// リソースの確保含め、頂点情報を柔軟に対応できるようにVertexData構造体を新たに作成する
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
	};

	// 頂点データの準備
	VertexData vertices[]{
	    {{-1.0f, 1.0f, 0.0f, 1.0f},  {0.0f, 0.0f}}, // 左上
	    {{1.0f, 1.0f, 0.0f, 1.0f},   {1.0f, 0.0f}}, // 右上
	    {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // 左下
	    {{1.0f, -1.0f, 0.0f, 1.0f},  {1.0f, 1.0f}}, // 右下
	};

	// VertexBuffer,VertexResourceViewの生成
	VertexBuffer vb;
	vb.Create(sizeof(vertices), sizeof(VertexData));

	// 頂点リソースにデータを書き込む
	VertexData* pGpuVertices = nullptr;
	vb.Get()->Map(0, nullptr, reinterpret_cast<void**>(&pGpuVertices));

	for (int i = 0; i < _countof(vertices); ++i) {
		pGpuVertices[i] = vertices[i];
	}

	// 頂点インデックスデータの準備
	uint16_t indices[] = {
	    0, 1, 2, 2, 1, 3,
	};

	// IndexBuffer(IndexResource,IndexResourceView)の生成
	IndexBuffer ib;
	ib.Create(sizeof(indices), sizeof(indices[0]));

	// 頂点インデックスリソースにデータを読み込む
	uint16_t* pGpuIndices = nullptr;
	ib.Get()->Map(0, nullptr, reinterpret_cast<void**>(&pGpuIndices));

	for (int i = 0; i < _countof(indices); ++i) {
		pGpuIndices[i] = indices[i];
	}

	// Resource生成,Heap生成、View生成で再利用される変数の準備
	ID3D12Device* device = dxCommon->GetDevice();
	HRESULT hr;
	// RenderTexture関係
	// 0. RenderTextureResourceの作成
	// 画面クリア色 ※分かりやすいように赤とする
	const FLOAT kRenderTargetClearColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};
	ID3D12Resource* renderTextureResource = CreateRenderTextureResource(device, WinApp::kWindowWidth, WinApp::kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, kRenderTargetClearColor);

	// 1.RTV用の DescriptorHeapを作成する
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.NumDescriptors = 2;

	hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
#ifdef DEBUG
	assert(SUCCEEDED(hr));
#endif // DEBUG

	// CPU側から見たHANDLEを取得しておく
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleCPU = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// 2.RTV用のViewの作成
	device->CreateRenderTargetView(
	    renderTextureResource, // Viewと関連付けたいリソース
	    nullptr,
	    rtvHandleCPU // RTV用のディスクリプタヒープのCPUHandle
	);

	// DepthStencilTexture関係
	ID3D12Resource* depthStencilResource = CreateDepthStencilTextureResource(device, WinApp::kWindowWidth, WinApp::kWindowHeight);

	// 1,DSV用の　DescriptorHeapの生成
	ID3D12DescriptorHeap* dsvDescriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc{};
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvDescriptorHeapDesc.NumDescriptors = 1;
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&dsvDescriptorHeap));
#ifdef DEBUG
	assert(SUCCEEDED(hr));
#endif // DEBUG

	// CPU側から見たHandleを取得しておく
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandleCPU = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// 2.DSV用のViewの生成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	// DSVheapの先頭にDSVを作る
	device->CreateDepthStencilView(depthStencilResource, &dsvDesc, dsvHandleCPU);

	// SRV用のDesCriptorHeapの作成
	ID3D12DescriptorHeap* srvDescriptorHeap;

	D3D12_DESCRIPTOR_HEAP_DESC srvDescriptorHeapDesc = {};
	srvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvDescriptorHeapDesc.NumDescriptors = 7;

	hr = device->CreateDescriptorHeap(&srvDescriptorHeapDesc, IID_PPV_ARGS(&srvDescriptorHeap));
#ifdef DEBUG
	assert(SUCCEEDED(hr));
#endif // DEBUG

	// CPU側から見たHANDLE,GPU側から見たHANDLEを取得しておく
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	// 2.SRV(Shader Resource View)の作成 (t0)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(renderTextureResource, &srvDesc, srvHandleCPU);

	srvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Depth TextureのSRV作成
	D3D12_SHADER_RESOURCE_VIEW_DESC depthTextureSrvDesc{};
	depthTextureSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	depthTextureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	depthTextureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	depthTextureSrvDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(depthStencilResource, &depthTextureSrvDesc, srvHandleCPU);

	srvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// ViewDataのConstantBuffer
	ConstantBuffer cbViewData;
	cbViewData.Create(sizeof(ViewData) * 4);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = cbViewData.GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = sizeof(ViewData) * 4;

	device->CreateConstantBufferView(&cbvDesc, srvHandleCPU);

	srvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// RandomTimeのConstantBuffer
	ConstantBuffer cbRandomTime;
	cbRandomTime.Create(sizeof(RandomTime) * 64);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbRandomTimeDesc{};
	cbRandomTimeDesc.BufferLocation = cbRandomTime.GetGPUVirtualAddress();
	cbRandomTimeDesc.SizeInBytes = sizeof(RandomTime) * 64;
	device->CreateConstantBufferView(&cbRandomTimeDesc, srvHandleCPU);

	srvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// ThresholdのConstantBuffer
	ConstantBuffer cbThreshold;
	cbThreshold.Create(sizeof(Threshold) * 64);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbThresholdDesc{};
	cbThresholdDesc.BufferLocation = cbThreshold.GetGPUVirtualAddress();
	cbThresholdDesc.SizeInBytes = sizeof(Threshold) * 64;
	device->CreateConstantBufferView(&cbThresholdDesc, srvHandleCPU);


	srvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	DirectX::ScratchImage maskMipImages = LoadTexture("Resources/noise0.png");
	const DirectX::TexMetadata& maskMetadata = maskMipImages.GetMetadata();
	ID3D12Resource* maskTextureResource = CreateTextureResource(device, maskMetadata);
	UpLoadTextureData(maskTextureResource, maskMipImages);

	D3D12_SHADER_RESOURCE_VIEW_DESC maskSrvDesc{};
	maskSrvDesc.Format = maskMetadata.format;
	maskSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	maskSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	maskSrvDesc.Texture2D.MipLevels = UINT(maskMetadata.mipLevels);

	device->CreateShaderResourceView(maskTextureResource, &maskSrvDesc, srvHandleCPU);

	// アプリで利用する3Dモデル
	// 被写体の準備
	Model* model = Model::CreateFromOBJ("terrain");

	WorldTransformEX worldTransform;
	worldTransform.Initialize();
	worldTransform.scale_ = Vector3(1.0f, 1.0f, 1.0f);

	// カメラの準備
	Camera camera;
	camera.Initialize();
	camera.translation_ = Vector3(0.0f, 12.0f, -15.0f);
	camera.rotation_ = Vector3(0.8f, 0.0f, 0.0f);

	std::random_device seeGenerator;
	std::mt19937 randomEngine(seeGenerator());
	std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

	float useThresShold = 0.0f;

	KamataEngine::Input* input = Input::GetInstance();
	int usePS = 0;

	
	// メインループ
	while (true) {
		// エンジンの更新
		if (KamataEngine::Update()) {
			break;
		}

		if (input->TriggerKey(DIK_A)) {
			usePS--;
		}

		if (input->TriggerKey(DIK_D)) {
			usePS++;
		}

		if (kNumPS <= usePS) {
			usePS = 0;
		} else if (usePS < 0) {
			usePS = kNumPS - 1;
		}

		if (usePS == 9) {

			if (input->PushKey(DIK_UP)) {
				useThresShold += 0.01f;
			}
			if (input->PushKey(DIK_DOWN)) {
				useThresShold -= 0.01f;
			}

			if (useThresShold >= 1.0f) {
				useThresShold = 1.0f;
			} else if (useThresShold <= 0.0f) {
				useThresShold = 0.0f;
			}
		} else {
			useThresShold = 0.5f;
		}

		////world変換行列の定数バッファへの転送
		worldTransform.rotation_.y += 0.005f;
		worldTransform.UpdateMatrix();

		/*ImGui::Begin("camera");
		ImGui::DragFloat3("camera.translation",&camera.translation_.x, 0.01f);
		ImGui::DragFloat3("camera.rotation", &camera.rotation_.x, 0.01f);
		ImGui::End();*/

		// Cameraの更新と定数バッファへの転送
		camera.UpdateMatrix();

		ViewData* viewData = nullptr;
		cbViewData.Get()->Map(0, nullptr, reinterpret_cast<void**>(&viewData));

		viewData->InverseProjection = Inverse(camera.matProjection);

		RandomTime* randomTime = nullptr;
		cbRandomTime.Get()->Map(0, nullptr, reinterpret_cast<void**>(&randomTime));
		randomTime->time = distribution(randomEngine);
		Threshold* threShold = nullptr;
		cbThreshold.Get()->Map(0, nullptr, reinterpret_cast<void**>(&threShold));
		threShold->threshold = useThresShold;

		// 描画開始

		// trabsitionBarrierをSRV=>RTVに設定する
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = renderTextureResource;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList->ResourceBarrier(1, &barrier);

		D3D12_RESOURCE_BARRIER depthBarrier{};
		depthBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		depthBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		depthBarrier.Transition.pResource = depthStencilResource;
		depthBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		depthBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;

		commandList->ResourceBarrier(1, &depthBarrier);

		// 描画先のRTVとDSVを設定する
		commandList->OMSetRenderTargets(1, &rtvHandleCPU, false, &dsvHandleCPU);

		// ViewPortの設定
		D3D12_VIEWPORT viewPort{};
		viewPort.Width = WinApp::kWindowWidth;
		viewPort.Height = WinApp::kWindowHeight;
		viewPort.TopLeftX = 0;
		viewPort.TopLeftY = 0;
		viewPort.MinDepth = 0.0f;
		viewPort.MaxDepth = 1.0f;

		commandList->RSSetViewports(1, &viewPort);

		// Scissorの設定
		D3D12_RECT scissorRect{};
		// 基本的にビューポートと同じ矩形が構成されるようにする
		scissorRect.left = 0;
		scissorRect.right = WinApp::kWindowWidth;
		scissorRect.top = 0;
		scissorRect.bottom = WinApp::kWindowHeight;

		commandList->RSSetScissorRects(1, &scissorRect);

		// 全画面クリア
		commandList->ClearRenderTargetView(rtvHandleCPU, kRenderTargetClearColor, 0, nullptr);
		// 指定した深度で画面全体をクリアする
		commandList->ClearDepthStencilView(dsvHandleCPU, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// 描画

		Model::PreDraw(commandList);
		model->Draw(worldTransform, camera);
		Model::PostDraw();

		// TransitionBarrierを元に戻し,PixelShaderが扱えるようにする
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = renderTextureResource;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		commandList->ResourceBarrier(1, &barrier);

		depthBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		depthBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		depthBarrier.Transition.pResource = depthStencilResource;
		depthBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		depthBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		commandList->ResourceBarrier(1, &depthBarrier);

		dxCommon->PreDraw();
		// コマンドを積む
		commandList->SetGraphicsRootSignature(rs.Get());
		commandList->SetPipelineState(pipelineState[usePS].Get());
		commandList->IASetVertexBuffers(0, 1, vb.GetView());
		commandList->IASetIndexBuffer(ib.GetView());
		commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 使用するディスクリプタヒープの設定
		/*ID3D12DescriptorHeap* Heaps[] = {srvDescriptorHeap};*/
		/*commandList->SetDescriptorHeaps(_countof(Heaps), Heaps);*/

		commandList->SetDescriptorHeaps(1, &srvDescriptorHeap);

		// t0 (renderTextureResource)
		commandList->SetGraphicsRootDescriptorTable(0, srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		// b0 (ViewData)
		commandList->SetGraphicsRootConstantBufferView(1, cbViewData.GetGPUVirtualAddress());
		// b1 (RandomTime)
		commandList->SetGraphicsRootConstantBufferView(2, cbRandomTime.GetGPUVirtualAddress());
		// t1 (maskTextureResource)
		// maskTextureResourceのSRVのオフセットを計算
		D3D12_GPU_DESCRIPTOR_HANDLE maskSrvHandleGPU = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		maskSrvHandleGPU.ptr +=
		    device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5; // 0:renderTexture, 1:depthTexture, 2:cbViewData, 3:cbRandomTime, 4:cbThreshold, 5:maskTexture
		commandList->SetGraphicsRootDescriptorTable(3, maskSrvHandleGPU);
		// b2 (Threshold)
		commandList->SetGraphicsRootConstantBufferView(4, cbThreshold.GetGPUVirtualAddress());
		commandList->DrawIndexedInstanced(_countof(indices), 1, 0, 0, 0);

		// 描画終了
		dxCommon->PostDraw();
	}
	// 解放
	delete model;
	renderTextureResource->Release();
	srvDescriptorHeap->Release();
	rtvDescriptorHeap->Release();

	depthStencilResource->Release();
	dsvDescriptorHeap->Release();
	maskTextureResource->Release(); // maskTextureResourceも解放します

	// エンジンの終了処理
	KamataEngine::Finalize();

	return 0;
}

void SetupPipeLineState(PipelineState& pipelineState, RootSignature& rs, Shader& vs, Shader& ps) {
	// inputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// BlemdState
	D3D12_BLEND_DESC blendDesc{};
	// 全ての色要素を書きこむ
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// RasterizerState
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// 裏面(反時計回りをカリングする)
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	// 塗りつぶしモードをソリッドにする
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// PSO(PipelineStateObject)の生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rs.Get();                                                    // RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;                                                // InputLayout
	graphicsPipelineStateDesc.VS = {vs.GetDxcBlob()->GetBufferPointer(), vs.GetDxcBlob()->GetBufferSize()}; // VertexShader
	graphicsPipelineStateDesc.PS = {ps.GetDxcBlob()->GetBufferPointer(), ps.GetDxcBlob()->GetBufferSize()}; // PixelShader
	graphicsPipelineStateDesc.BlendState = blendDesc;                                                       // BlendState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1; // 1つのRTVに書き込む ※2つ同時にしようと思えばできる
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	// 利用するトポロジ(形状)のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// どのように画面に色を打ち込むかの設定(今は気にしなくていい)
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// DepthStencilState
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; //

	// 準備が整ったのでPSOを生成する
	pipelineState.Create(graphicsPipelineStateDesc);
}

ID3D12Resource* CreateRenderTextureResource(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, const FLOAT* clearColor) {
	// 1,生成するRenderTextureのDescの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(width);                             // RenderTextureの幅
	resourceDesc.Height = UINT(height);                           // Textureの高さ
	resourceDesc.MipLevels = 1;                                   // mipmapの数
	resourceDesc.DepthOrArraySize = 1;                            // 奥行き or 配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;        // Textureのformat
	resourceDesc.SampleDesc.Count = 1;                            // サンプリングカウント 1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  // Textureの次元数。普段使っているのは二次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; // RenderTargetとして使う通知
	// 2.利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapPropaties{};
	heapPropaties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// 3,ClearValueの用意
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = format;
	clearValue.Color[0] = clearColor[0];
	clearValue.Color[1] = clearColor[1];
	clearValue.Color[2] = clearColor[2];
	clearValue.Color[3] = clearColor[3];

	// 4. RenderTextureResourceの生成
	ID3D12Resource* resource = nullptr;
	[[maybe_unused]] HRESULT hr = device->CreateCommittedResource(
	    &heapPropaties,                             // Heapのプロパティ
	    D3D12_HEAP_FLAG_NONE,                       // Heapの特殊な設定
	    &resourceDesc,                              // Resourceの設定
	    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // pixelShaderでアクセスできるようにする
	    &clearValue,                                // Clear最速値
	    IID_PPV_ARGS(&resource));
#ifdef DEBUG
	assert(SUCCEEDED(hr));
#endif // DEBUG

	return resource;
}

ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height) {
	// 1.生成するDepthStencikTextureのDescの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;        // Textureの幅
	resourceDesc.Height = height;      // Textureの高さ
	resourceDesc.MipLevels = 1;        // mipmapの数 DepthStencilなので一つで良い
	resourceDesc.DepthOrArraySize = 1; // Textureの配列数　DepthStencilなので一つで良い
	resourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	// DepthStencilとして利用可能なフォーマット

	resourceDesc.SampleDesc.Count = 1;                            // サンプリングカウント
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  // 二次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

	// 利用するヒープの設定

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// 深度地のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	// 3.Resourceの生成
	ID3D12Resource* resource = nullptr;
	[[maybe_unused]] HRESULT hr =
	    device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &depthClearValue, IID_PPV_ARGS(&resource));

#ifdef DEBUG
	assert(SUCCEEDED(hr));
#endif // DEBUG

	return resource;
}

DirectX::ScratchImage LoadTexture(const std::string& filePath) {
	// テクスチャファイルを呼んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミップマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	// ミップマップ付きのデータを消す
	return mipImages;
}

ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata) {
	// 1.metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);                             // Textureの幅
	resourceDesc.Height = UINT(metadata.height);                           // Textureの高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);                   // mipmapの数
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);            // 奥行きor配列Textureの配列数
	resourceDesc.Format = metadata.format;                                 // TextureのFormat
	resourceDesc.SampleDesc.Count = 1;                                     // サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // Textureの次元数。普段使っているのは二次元
	// 2.利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;                        // 細かい設定を行う
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // WriteBackポリシーでCPUアクセス可能
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;          // プロセッサの近くに配置
	// 3.Resourceを生成する
	ID3D12Resource* resource = nullptr;
	[[maybe_unused]] HRESULT hr = device->CreateCommittedResource(
	    &heapProperties,                   // Heapの設定
	    D3D12_HEAP_FLAG_NONE,              // heapの特殊な設定　特になし
	    &resourceDesc,                     // Resourceの設定
	    D3D12_RESOURCE_STATE_GENERIC_READ, // 初回のResourceState。　Textureは基本読むだけ
	    nullptr,                           // Clear最適値。使わないのでnullptr
	    IID_PPV_ARGS(&resource));          // 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));
	return resource;
}

void UpLoadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages) {
	// Meta情報を取得
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	// 全MipMapについて
	for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
		// MipMapLevelを指定して各Imageを取得
		const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);
		// Textureに転送
		[[maybe_unused]]HRESULT hr = texture->WriteToSubresource(
		    UINT(mipLevel),
		    nullptr,             // 全領域へコピー
		    img->pixels,         // 元データアドレス
		    UINT(img->rowPitch), // 1ラインサイズ
		    UINT(img->slicePitch));
		assert(SUCCEEDED(hr));
	}
}