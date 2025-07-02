#pragma once
#include "KamataEngine.h"
class WorldTransformEX : public KamataEngine::WorldTransform {

public:
	// Affine変換行列の作成と定数バッファへの転送を行う
	void UpdateMatrix();

	//Affine変換行列の作成
	KamataEngine::Matrix4x4 MakeAffineMatrix();
};
