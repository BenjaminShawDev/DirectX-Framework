#pragma once

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "resource.h"
#include "DDSTextureLoader.h"
//#include "Structures.h"
#include "OBJLoader.h"
#include "Camera.h"
#include "GameObject.h"
#include <vector>

using namespace DirectX;

class Application
{
private:
	HINSTANCE               _hInst;
	HWND                    _hWnd;
	D3D_DRIVER_TYPE         _driverType;
	D3D_FEATURE_LEVEL       _featureLevel;
	ID3D11Device*           _pd3dDevice;
	ID3D11DeviceContext*    _pImmediateContext;
	IDXGISwapChain*         _pSwapChain;
	ID3D11RenderTargetView* _pRenderTargetView;
	ID3D11VertexShader*     _pVertexShader;
	ID3D11PixelShader*      _pPixelShader;
	ID3D11InputLayout*      _pVertexLayout;
	ID3D11Buffer*           _pVertexBufferCube;
	ID3D11Buffer*           _pIndexBufferCube;
	ID3D11Buffer*			_pVertexBufferPyramid;
	ID3D11Buffer*			_pIndexBufferPyramid;
	ID3D11Buffer*			_pVertexBufferPlane;
	ID3D11Buffer*			_pIndexBufferPlane;
	ID3D11Buffer*           _pConstantBuffer;
	//XMFLOAT4X4              _world, _world2, _world3, _world4, _world5;
	XMFLOAT4X4              _world;
	//XMFLOAT4X4              _view;
	//XMFLOAT4X4              _projection;
	ID3D11DepthStencilView* _depthStencilView;
	ID3D11Texture2D*		_depthStencilBuffer;
	ID3D11RasterizerState*  _wireFrame;
	ID3D11RasterizerState*  _solidShape;
	ID3D11BlendState*		_transparency;

	ID3D11ShaderResourceView* _pTextureRV;
	ID3D11SamplerState*		_pSamplerLinear;

	Geometry				cubeGeometry;
	Geometry				floorGeometry;
	Geometry				aeroplaneGeometry;

	Material				shinyMaterial;
	Material				noSpecMaterial;

	XMFLOAT3				lightDirection;
	XMFLOAT4				diffuseMaterial;
	XMFLOAT4				diffuseLight;
	XMFLOAT4				ambientMaterial;
	XMFLOAT4				ambientLight;
	XMFLOAT4				specularMaterial;
	XMFLOAT4				specularLight;
	float					specularPower;
	XMFLOAT3				eyePosW;

	MeshData				floorMeshData;
	MeshData				cubeMeshData;
	MeshData				aeroplaneMeshData;
	ID3D11ShaderResourceView* floorTextureData = nullptr;
	ID3D11ShaderResourceView* cubeTextureData = nullptr;
	ID3D11ShaderResourceView* aeroplaneTextureData = nullptr;

	int selectedCameraNum = 0;
	int cameraDetectDelay = 0;
	int objectCreateNumber = 1;
	float objectScaleNumber = 1;

private:
	HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
	HRESULT InitDevice();
	void Cleanup();
	HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	HRESULT InitShadersAndInputLayout();

	void CreateObject(int objectNum);

	UINT _WindowHeight;
	UINT _WindowWidth;

	vector<GameObject*> _gameObjects;
	GameObject* gameObject;

	Camera* _camera;
	Camera* _camera2;
	float _cameraOrbitRadius = 7.0f;
	float _cameraOrbitRadiusMin = 2.0f;
	float _cameraOrbitRadiusMax = 50.0f;
	float _cameraOrbitAngleXZ = -90.0f;
	float _cameraSpeed = 2.0f;

	UINT _renderHeight = 1080;
	UINT _renderWidth = 1920;

public:
	Application();
	~Application();

	HRESULT Initialise(HINSTANCE hInstance, int nCmdShow);

	void Update();
	void Draw();
};

