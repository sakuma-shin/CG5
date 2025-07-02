#include "WorldTransformEX.h"

using namespace KamataEngine;
using namespace KamataEngine::MathUtility;

void WorldTransformEX::UpdateMatrix() {

//world行列を計算し、matworld_に格納する
	matWorld_ = MakeAffineMatrix();
	//定数バッファへ転送する
	TransferMatrix();
}

KamataEngine::Matrix4x4 WorldTransformEX::MakeAffineMatrix() { 
	//アフィン変換行列を作る
	//  スケーリング行列の作成
	Matrix4x4 matScale = MakeScaleMatrix(scale_);
	// 回転行列の作成
	Matrix4x4 matRotX = MakeRotateXMatrix(rotation_.x);
	Matrix4x4 matRotY = MakeRotateYMatrix(rotation_.y);
	Matrix4x4 matRotZ = MakeRotateZMatrix(rotation_.z);
	Matrix4x4 matRot = matRotZ * matRotX * matRotY;

	// 平行移動行列の作成
	Matrix4x4 matTrans = MakeTranslateMatrix(translation_);
	// スケーリング、回転、平行移動の合成
	Matrix4x4 matWorld = matScale * matRot * matTrans;
	return matWorld;
}
