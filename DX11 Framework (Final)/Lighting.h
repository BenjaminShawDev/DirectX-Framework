#pragma once

#include <DirectXMath.h>
#include <d3d11.h>

using namespace DirectX;

class Lighting
{
public:
	Lighting(XMFLOAT4 diffuseLight, XMFLOAT4 ambientLight, XMFLOAT4 specularLight, float specularPower, XMFLOAT3 lightDirection, XMFLOAT3 position);
	~Lighting();

	void SetPosition(XMFLOAT3 position) { _position = position; }
	void SetPosition(float x, float y, float z) { _position.x = x; _position.y = y; _position.z = z; }

	XMFLOAT3 GetPosition() const { return _position; }

	void SetDiffuseLight(XMFLOAT4 diffuseLight) { _diffuseLight = diffuseLight; }
	void SetDiffuseLight(float r, float g, float b, float a) { _diffuseLight.x = r; _diffuseLight.y = g; _diffuseLight.z = b; _diffuseLight.w = a; }

	XMFLOAT4 GetDiffuseLight() const { return _diffuseLight; }

	void SetAmbientLight(XMFLOAT4 ambientLight) { _ambientLight = ambientLight; }
	void SetAmbientLight(float r, float g, float b, float a) { _ambientLight.x = r; _ambientLight.y = g; _ambientLight.z = b; _ambientLight.w = a; }

	XMFLOAT4 GetAmbientLight() const { return _ambientLight; }

	void SetSpecularLight(XMFLOAT4 specularLight) { _specularLight = specularLight; }
	void SetSpecularLight(float r, float g, float b, float a) { _specularLight.x = r; _specularLight.y = g; _specularLight.z = b; _specularLight.w = a; }

	XMFLOAT4 GetSpecularLight() const { return _specularLight; }

	void SetSpecularPower(float specularPower) { _specularPower = specularPower; }

	float GetSpecularPower() const { return _specularPower; }

	void SetLightDirection(XMFLOAT3 lightDirection) { _lightDirection = lightDirection; }
	void SetLightDirection(float x, float y, float z) { _lightDirection.x = x; _lightDirection.y = y; _lightDirection.z = z; }

	XMFLOAT3 GetLightDirection() const { return _lightDirection; }

	void Update();
private:
	XMFLOAT3 _position;
	XMFLOAT4 _diffuseLight;
	XMFLOAT4 _ambientLight;
	XMFLOAT4 _specularLight;
	float _specularPower;
	XMFLOAT3 _lightDirection;


	XMFLOAT4X4 _world;
};

