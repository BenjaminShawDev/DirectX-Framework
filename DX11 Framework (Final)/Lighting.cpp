#include "Lighting.h"

Lighting::Lighting(XMFLOAT4 diffuseLight, XMFLOAT4 ambientLight, XMFLOAT4 specularLight, float specularPower, XMFLOAT3 lightDirection, XMFLOAT3 position) : _diffuseLight(diffuseLight), _ambientLight(ambientLight), _specularLight(specularLight), _specularPower(specularPower), _lightDirection(lightDirection), _position(position)
{
	Update();
}

Lighting::~Lighting()
{

}

void Lighting::Update()
{
	XMMATRIX translation = XMMatrixTranslation(_position.x, _position.y, _position.z);

	XMStoreFloat4x4(&_world, translation);
}