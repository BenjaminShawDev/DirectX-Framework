#include "Application.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

Application::Application()
{
	_hInst = nullptr;
	_hWnd = nullptr;
	_driverType = D3D_DRIVER_TYPE_NULL;
	_featureLevel = D3D_FEATURE_LEVEL_11_0;
	_pd3dDevice = nullptr;
	_pImmediateContext = nullptr;
	_pSwapChain = nullptr;
	_pRenderTargetView = nullptr;
	_pVertexShader = nullptr;
	_pPixelShader = nullptr;
	_pVertexLayout = nullptr;
	_pConstantBuffer = nullptr;
    _pTextureRV = nullptr;
    _pSamplerLinear = nullptr;
    _transparency = nullptr;
    gameObject = nullptr;
    prevGameObject = nullptr;
    _camera = nullptr;
    _light = nullptr;
}

Application::~Application()
{
	Cleanup();
}

HRESULT Application::Initialise(HINSTANCE hInstance, int nCmdShow)
{
    if (FAILED(InitWindow(hInstance, nCmdShow)))
	{
        return E_FAIL;
	}

    RECT rc;
    GetClientRect(_hWnd, &rc);
    _WindowWidth = rc.right - rc.left;
    _WindowHeight = rc.bottom - rc.top;

    if (FAILED(InitDevice()))
    {
        Cleanup();

        return E_FAIL;
    }

    ShowCursor(false);

	// Initialize the world matrix
	XMStoreFloat4x4(&_world, XMMatrixIdentity());

    //Light direction from surface (XYZ)
    lightDirection = XMFLOAT3(0.25f, 0.5f, -1.0f);
    //Diffuse light colour (RGBA)
    diffuseLight = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    //diffuseLight = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    //Ambient
    ambientLight = XMFLOAT4(0.2f, 0.2f, 0.2f, 0.2f);
    //ambientLight = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    //Specular
    specularLight = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    //specularLight = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
    specularPower = 20.0f;

    XMFLOAT3 lightPosition = XMFLOAT3(0.0f, 10.0f, 0.0f);

    //Create a light
    _light = new Lighting(diffuseLight, ambientLight, specularLight, specularPower, lightDirection, lightPosition);
    _lighting.push_back(_light);

    //Variable to check which camera we are on
    selectedCameraNum = 0;

    //Create the cameras
    XMFLOAT3 eye = XMFLOAT3(0.0f, 2.0f, -1.0f);
    XMFLOAT3 at = XMFLOAT3(0.0f, 0.0f, 1.0f);
    XMFLOAT3 up = XMFLOAT3(0.0f, 1.0f, 0.0f);

    _camera = new Camera(eye, at, up, (float)_renderWidth, (float)_renderHeight, 0.1f, 200.0f, true);
    _cameras.push_back(_camera);

    eye = XMFLOAT3(30.0f, 1.0f, 0.0f);
    at = XMFLOAT3(0.0f, 1.0f, 0.0f);
    up = XMFLOAT3(0.0f, 1.0f, 0.0f);

    _camera = new Camera(eye, at, up, (float)_renderWidth, (float)_renderHeight, 0.01f, 200.0f, false);
    _cameras.push_back(_camera);

    eye = XMFLOAT3(0.0f, 30.0f, 1.0f);
    at = XMFLOAT3(0.0f, 0.0f, 0.0f);
    up = XMFLOAT3(0.0f, 1.0f, 0.0f);

    _camera = new Camera(eye, at, up, (float)_renderWidth, (float)_renderHeight, 0.01f, 200.0f, false);
    _cameras.push_back(_camera);

    eye = XMFLOAT3(0.0f, 2.0f, -1.0f);
    at = XMFLOAT3(0.0f, 0.0f, -1.0f);
    up = XMFLOAT3(0.0f, 1.0f, 0.0f);

    _camera = new Camera(eye, at, up, (float)_renderWidth, (float)_renderHeight, 0.01f, 200.0f, true);
    _cameras.push_back(_camera);

    //Load the OBJ files
    floorMeshData = OBJLoader::Load("Assets/Plane.obj", _pd3dDevice, false);
    cubeMeshData = OBJLoader::Load("Assets/Cube.obj", _pd3dDevice, false);
    aeroplaneMeshData = OBJLoader::Load("Assets/Hercules.obj", _pd3dDevice, false);
    barrelMeshData = OBJLoader::Load("Assets/Cylinder.obj", _pd3dDevice, false);
    ufoMeshData = OBJLoader::Load("Assets/UFO.obj", _pd3dDevice, false);

    //Load the object textures
    CreateDDSTextureFromFile(_pd3dDevice, L"Assets/Brick.dds", nullptr, &floorTextureData);
    CreateDDSTextureFromFile(_pd3dDevice, L"Assets/Crate_COLOR.dds", nullptr, &cubeTextureData);
    CreateDDSTextureFromFile(_pd3dDevice, L"Assets/HERCULES_COLOR.dds", nullptr, &aeroplaneTextureData);
    CreateDDSTextureFromFile(_pd3dDevice, L"Assets/Cylinder.dds", nullptr, &barrelTextureData);
    CreateDDSTextureFromFile(_pd3dDevice, L"Assets/UFO.dds", nullptr, &ufoTextureData);

    HRESULT hr;

    //Floor Geometry;
    floorGeometry.indexBuffer = floorMeshData.IndexBuffer;
    floorGeometry.vertexBuffer = floorMeshData.VertexBuffer;
    floorGeometry.numberOfIndices = floorMeshData.IndexCount;
    floorGeometry.vertexBufferOffset = floorMeshData.VBOffset;
    floorGeometry.vertexBufferStride = floorMeshData.VBStride;

    //Cube Geometry;
    cubeGeometry.indexBuffer = cubeMeshData.IndexBuffer;
    cubeGeometry.vertexBuffer = cubeMeshData.VertexBuffer;
    cubeGeometry.numberOfIndices = cubeMeshData.IndexCount;
    cubeGeometry.vertexBufferOffset = cubeMeshData.VBOffset;
    cubeGeometry.vertexBufferStride = cubeMeshData.VBStride;

    //Aeroplane Geometry;
    aeroplaneGeometry.indexBuffer = aeroplaneMeshData.IndexBuffer;
    aeroplaneGeometry.vertexBuffer = aeroplaneMeshData.VertexBuffer;
    aeroplaneGeometry.numberOfIndices = aeroplaneMeshData.IndexCount;
    aeroplaneGeometry.vertexBufferOffset = aeroplaneMeshData.VBOffset;
    aeroplaneGeometry.vertexBufferStride = aeroplaneMeshData.VBStride;

    //Barrel geometry
    barrelGeometry.indexBuffer = barrelMeshData.IndexBuffer;
    barrelGeometry.vertexBuffer = barrelMeshData.VertexBuffer;
    barrelGeometry.numberOfIndices = barrelMeshData.IndexCount;
    barrelGeometry.vertexBufferOffset = barrelMeshData.VBOffset;
    barrelGeometry.vertexBufferStride = barrelMeshData.VBStride;

    //UFO geometry
    ufoGeometry.indexBuffer = ufoMeshData.IndexBuffer;
    ufoGeometry.vertexBuffer = ufoMeshData.VertexBuffer;
    ufoGeometry.numberOfIndices = ufoMeshData.IndexCount;
    ufoGeometry.vertexBufferOffset = ufoMeshData.VBOffset;
    ufoGeometry.vertexBufferStride = ufoMeshData.VBStride;

    //Shiny material;
    shinyMaterial.ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
    shinyMaterial.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    shinyMaterial.specular = XMFLOAT4(0.5f, 0.5f, 0.5, 1.0f);
    shinyMaterial.specularPower = 10.0f;

    //No specular light material;
    noSpecMaterial.ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
    noSpecMaterial.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    noSpecMaterial.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    noSpecMaterial.specularPower = 0.0f;

    //Create the floor
    gameObject = new GameObject("Floor", floorGeometry, noSpecMaterial);
    gameObject->SetPosition(0.0f, 0.0f, 0.0f);
    gameObject->SetScale(10.0f, 1.0f, 10.0f);
    gameObject->SetRotation(0.0f, 0.0f, 0.0f);
    gameObject->SetTextureRV(floorTextureData);

    _gameObjects.push_back(gameObject);

    //Create the sample state
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    _pd3dDevice->CreateSamplerState(&sampDesc, &_pSamplerLinear);

    //Create the blend state
    D3D11_BLEND_DESC blendDesc;
    ZeroMemory(&blendDesc, sizeof(blendDesc));

    D3D11_RENDER_TARGET_BLEND_DESC rtbd;
    ZeroMemory(&rtbd, sizeof(rtbd));
    rtbd.BlendEnable = true;
    rtbd.SrcBlend = D3D11_BLEND_SRC_COLOR;
    rtbd.DestBlend = D3D11_BLEND_BLEND_FACTOR;
    rtbd.BlendOp = D3D11_BLEND_OP_ADD;
    rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
    rtbd.DestBlend = D3D11_BLEND_ZERO;
    rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rtbd.RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;
    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.RenderTarget[0] = rtbd;

    //_pd3dDevice->CreateBlendState(&blendDesc, &_transparency);

	return S_OK;
}

HRESULT Application::InitShadersAndInputLayout()
{
	HRESULT hr;

    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    hr = CompileShaderFromFile(L"DX11 Framework.fx", "VS", "vs_4_0", &pVSBlob);

    if (FAILED(hr))
    {
        MessageBox(nullptr,
                   L"(VS) The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

	// Create the vertex shader
	hr = _pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &_pVertexShader);

	if (FAILED(hr))
	{	
		pVSBlob->Release();
        return hr;
	}

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile(L"DX11 Framework.fx", "PS", "ps_4_0", &pPSBlob);

    if (FAILED(hr))
    {
        MessageBox(nullptr,
                   L"(PS) The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

	// Create the pixel shader
	hr = _pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &_pPixelShader);
	pPSBlob->Release();

    if (FAILED(hr))
        return hr;

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
	hr = _pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
                                        pVSBlob->GetBufferSize(), &_pVertexLayout);
	pVSBlob->Release();

	if (FAILED(hr))
        return hr;

    // Set the input layout
    _pImmediateContext->IASetInputLayout(_pVertexLayout);

	return hr;
}

HRESULT Application::InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW );
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    _hInst = hInstance;
    //RECT rc = {0, 0, 640, 480};
    RECT rc = {0, 0, 1280, 720};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    _hWnd = CreateWindow(L"TutorialWindowClass", L"DX11 Framework", WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                         nullptr);
    if (!_hWnd)
		return E_FAIL;

    ShowWindow(_hWnd, nCmdShow);

    return S_OK;
}

HRESULT Application::CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob);

    if (FAILED(hr))
    {
        if (pErrorBlob != nullptr)
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());

        if (pErrorBlob) pErrorBlob->Release();

        return hr;
    }

    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
}

HRESULT Application::InitDevice()
{
    HRESULT hr = S_OK;

    UINT createDeviceFlags = 0;

#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    D3D11_TEXTURE2D_DESC depthStencilDesc;
    depthStencilDesc.Width = _WindowWidth;
    depthStencilDesc.Height = _WindowHeight;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = _WindowWidth;
    sd.BufferDesc.Height = _WindowHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = _hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        _driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(nullptr, _driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                           D3D11_SDK_VERSION, &sd, &_pSwapChain, &_pd3dDevice, &_featureLevel, &_pImmediateContext);
        if (SUCCEEDED(hr))
            break;
    }

    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = _pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    if (FAILED(hr))
        return hr;

    hr = _pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &_pRenderTargetView);
    pBackBuffer->Release();

    if (FAILED(hr))
        return hr;

    _pd3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &_depthStencilBuffer);
    _pd3dDevice->CreateDepthStencilView(_depthStencilBuffer, nullptr, &_depthStencilView);
    _pImmediateContext->OMSetRenderTargets(1, &_pRenderTargetView, _depthStencilView);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)_WindowWidth;
    vp.Height = (FLOAT)_WindowHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    _pImmediateContext->RSSetViewports(1, &vp);

	InitShadersAndInputLayout();

    // Set primitive topology
    _pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the constant buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
    hr = _pd3dDevice->CreateBuffer(&bd, nullptr, &_pConstantBuffer);

    D3D11_RASTERIZER_DESC wfdesc;
    ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
    wfdesc.FillMode = D3D11_FILL_WIREFRAME;
    wfdesc.CullMode = D3D11_CULL_NONE;
    hr = _pd3dDevice->CreateRasterizerState(&wfdesc, &_wireFrame);

    if (FAILED(hr))
        return hr;

    return S_OK;
}

void Application::Cleanup()
{
    if (_pImmediateContext) _pImmediateContext->ClearState();

    if (_pConstantBuffer) _pConstantBuffer->Release();
    if (_pVertexLayout) _pVertexLayout->Release();
    if (_pVertexShader) _pVertexShader->Release();
    if (_pPixelShader) _pPixelShader->Release();
    if (_pRenderTargetView) _pRenderTargetView->Release();
    if (_pSwapChain) _pSwapChain->Release();
    if (_pImmediateContext) _pImmediateContext->Release();
    if (_pd3dDevice) _pd3dDevice->Release();
    if (_depthStencilView) _depthStencilView->Release();
    if (_depthStencilBuffer) _depthStencilBuffer->Release();
    if (_wireFrame) _wireFrame->Release();
    if (_solidShape) _solidShape->Release();
    if (_transparency) _transparency->Release();
    if (_pTextureRV) _pTextureRV->Release();
    if (_pSamplerLinear) _pSamplerLinear->Release();
    if (floorTextureData) floorTextureData->Release();
    if (cubeTextureData) cubeTextureData->Release();
    if (aeroplaneTextureData) aeroplaneTextureData->Release();
    if (barrelTextureData) barrelTextureData->Release();
    if (ufoTextureData) ufoTextureData->Release();
}

void Application::CreateObject(int objectNum, bool isShiny)
{
    Material material;
    if (isShiny)
        material = shinyMaterial;
    else
        material = noSpecMaterial;

    switch (objectNum)
    {
    case 1:
        gameObject = new GameObject("Aeroplane", aeroplaneGeometry, material);
        gameObject->SetTextureRV(aeroplaneTextureData);
        break;
    case 2:
        gameObject = new GameObject("Barrel", barrelGeometry, material);
        gameObject->SetTextureRV(barrelTextureData);
        break;
    case 3:
        gameObject = new GameObject("UFO", ufoGeometry, material);
        gameObject->SetTextureRV(ufoTextureData);
        break;
    default:
        gameObject = new GameObject("Cube", cubeGeometry, material);
        gameObject->SetTextureRV(cubeTextureData);
        break;
    }
    XMFLOAT3 objectPosition = _cameras[0]->GetPosition();
    gameObject->SetPosition(objectPosition.x, objectPosition.y, objectPosition.z += 4.0f);
    gameObject->SetScale(objectScaleNumber, objectScaleNumber, objectScaleNumber);
    gameObject->SetRotation(0.0f, 0.0f, 0.0f);

    _gameObjects.push_back(gameObject);
}

void Application::UserKeyboardInput()
{
    //Move cameras 0 and 3 (1 and 2 are static)
    XMFLOAT3 cameraPos = _cameras[0]->GetPosition();
    XMFLOAT3 atPos = _cameras[0]->GetLookAt();
    if (selectedCameraNum == 0)
    {
        if (GetAsyncKeyState('W') && !GetAsyncKeyState(VK_CONTROL))
        {
            cameraPos.z += 0.1f;
        }
        if (GetAsyncKeyState('S') && !GetAsyncKeyState(VK_CONTROL))
        {
            cameraPos.z -= 0.1f;
        }
        if (GetAsyncKeyState('A') && !GetAsyncKeyState(VK_CONTROL))
        {
            cameraPos.x -= 0.1f;
        }
        if (GetAsyncKeyState('D') && !GetAsyncKeyState(VK_CONTROL))
        {
            cameraPos.x += 0.1f;
        }
        if (GetAsyncKeyState('Q') && !GetAsyncKeyState(VK_CONTROL))
            cameraPos.y += 0.1f;
        if (GetAsyncKeyState('E') && cameraPos.y > 2.0f && !GetAsyncKeyState(VK_CONTROL))
            cameraPos.y -= 0.1f;
    }
    else if (selectedCameraNum == 3)
    {
        if (GetAsyncKeyState('W'))
        {
            cameraPos.z -= 0.1f;
        }
        if (GetAsyncKeyState('S'))
        {
            cameraPos.z += 0.1f;
        }
        if (GetAsyncKeyState('A'))
        {
            cameraPos.x += 0.1f;
        }
        if (GetAsyncKeyState('D'))
        {
            cameraPos.x -= 0.1f;
        }
    }
    _cameras[0]->SetPosition(cameraPos);
    _cameras[3]->SetPosition(cameraPos);

    //Toggle if the camera follows the mouse
    if (GetAsyncKeyState('M') && isMdown)
        isMdown = true;
    else
        isMdown = false;

    if (GetAsyncKeyState('M') && !isMdown)
    {
        mouseMovement = !mouseMovement;
        isMdown = true;
    }

    //Switch between the differet cameras controls
    if (GetAsyncKeyState(VK_TAB) && isTabDown)
        isTabDown = true;
    else
        isTabDown = false;

    if (GetAsyncKeyState(VK_TAB) && !isTabDown)
    {
        selectedCameraNum++;
        isTabDown = true;
    }
    if (selectedCameraNum >= _cameras.size())
        selectedCameraNum = 0;

    if (GetAsyncKeyState(MK_LBUTTON) && isLMouseDown)
        isLMouseDown = true;
    else
        isLMouseDown = false;

    //Create objects at camera 0, if you hold down control it creates the object with the no specular light material
    if (GetAsyncKeyState(MK_LBUTTON) && !GetAsyncKeyState(VK_LCONTROL) && !isLMouseDown)
    {
        CreateObject(objectCreateNumber, true);
        isLMouseDown = true;
    }
    else if (GetAsyncKeyState(MK_LBUTTON) && GetAsyncKeyState(VK_LCONTROL) && !isLMouseDown)
    {
        CreateObject(objectCreateNumber, false);
        isLMouseDown = true;
    }

    //Sends the most recently created object to camera 0
    if (GetAsyncKeyState(MK_RBUTTON) && _gameObjects.size() > 1)
    {
        _gameObjects.back()->SetPosition(cameraPos.x, cameraPos.y, cameraPos.z += 4.0f);
    }

    if (GetAsyncKeyState('Z') && isZDown)
        isZDown = true;
    else
        isZDown = false;

    //Control+Z, removes the last object that was created
    if (GetAsyncKeyState(VK_LCONTROL) && GetAsyncKeyState('Z') && !isZDown && _gameObjects.size() > 1)
    {
        if (_gameObjects.size() > 2)
        {
            XMFLOAT3 prevObjectScale = _gameObjects[_gameObjects.size() - 2]->GetScale();
            objectScaleNumber = prevObjectScale.x;
        }
        prevGameObject = _gameObjects.back();
        XMFLOAT3 oldObjectScale = prevGameObject->GetScale();
        formerObjectScale = oldObjectScale.x;
        _gameObjects.pop_back();
        if (_gameObjects.size() > 1)
            _gameObjects.back()->SetScale(objectScaleNumber, objectScaleNumber, objectScaleNumber);
        isZDown = true;
    }

    //Control+Y, brings back the most recent deleted object
    if (GetAsyncKeyState(VK_LCONTROL) && GetAsyncKeyState('Y') && prevGameObject != nullptr)
    {
        _gameObjects.push_back(prevGameObject);
        objectScaleNumber = formerObjectScale;
        prevGameObject = nullptr;
    }

    //Changes the speed at which an object is transformed
    if (GetAsyncKeyState(VK_RETURN))
        objectTransformRate += 0.01f;
    else if (GetAsyncKeyState(VK_BACK) && objectTransformRate > 0.02f)
        objectTransformRate -= 0.01f;

    //Translate the most recently created object
    XMFLOAT3 position = _gameObjects.back()->GetPosition();
    if (GetAsyncKeyState(VK_UP) && !GetAsyncKeyState(VK_LSHIFT) && !GetAsyncKeyState(VK_LCONTROL) && _gameObjects.size() > 1)
    {
        _gameObjects.back()->SetPosition(position.x, position.y, position.z += (0.02f * objectTransformRate));
    }
    if (GetAsyncKeyState(VK_DOWN) && !GetAsyncKeyState(VK_LSHIFT) && !GetAsyncKeyState(VK_LCONTROL) && _gameObjects.size() > 1)
    {
        _gameObjects.back()->SetPosition(position.x, position.y, position.z -= (0.02f * objectTransformRate));
    }
    if (GetAsyncKeyState(VK_LEFT) && !GetAsyncKeyState(VK_LSHIFT) && !GetAsyncKeyState(VK_LCONTROL) && _gameObjects.size() > 1)
    {
        _gameObjects.back()->SetPosition(position.x -= (0.02f * objectTransformRate), position.y, position.z);
    }
    if (GetAsyncKeyState(VK_RIGHT) && !GetAsyncKeyState(VK_LSHIFT) && !GetAsyncKeyState(VK_LCONTROL) && _gameObjects.size() > 1)
    {
        _gameObjects.back()->SetPosition(position.x += (0.02f * objectTransformRate), position.y, position.z);
    }
    if (GetAsyncKeyState(VK_UP) && !GetAsyncKeyState(VK_LSHIFT) && GetAsyncKeyState(VK_LCONTROL) && _gameObjects.size() > 1)
    {
        _gameObjects.back()->SetPosition(position.x, position.y += (0.02f * objectTransformRate), position.z);
    }
    if (GetAsyncKeyState(VK_DOWN) && !GetAsyncKeyState(VK_LSHIFT) && GetAsyncKeyState(VK_LCONTROL) && _gameObjects.size() > 1)
    {
        _gameObjects.back()->SetPosition(position.x, position.y -= (0.02f * objectTransformRate), position.z);
    }

    //Translate all of the objects forward in the z axis
    if (GetAsyncKeyState(' '))
    {
        for (int i = 1; i < _gameObjects.size(); i++)
        {
            XMFLOAT3 loopPosition = _gameObjects[i]->GetPosition();
            loopPosition.z += 0.02f * objectTransformRate;
            _gameObjects[i]->SetPosition(loopPosition);
        }
    }

    //Rotate the most recently created object
    XMFLOAT3 rotation = _gameObjects.back()->GetRotation();
    if (GetAsyncKeyState(VK_UP) && GetAsyncKeyState(VK_LSHIFT) && _gameObjects.size() > 1)
    {
        _gameObjects.back()->SetRotation(rotation.x += (0.02f * objectTransformRate), rotation.y, rotation.z);
    }
    if (GetAsyncKeyState(VK_DOWN) && GetAsyncKeyState(VK_LSHIFT) && _gameObjects.size() > 1)
    {
        _gameObjects.back()->SetRotation(rotation.x -= (0.02f * objectTransformRate), rotation.y, rotation.z);
    }
    if (GetAsyncKeyState(VK_LEFT) && GetAsyncKeyState(VK_LSHIFT) && _gameObjects.size() > 1)
    {
        _gameObjects.back()->SetRotation(rotation.x, rotation.y, rotation.z += (0.02f * objectTransformRate));
    }
    if (GetAsyncKeyState(VK_RIGHT) && GetAsyncKeyState(VK_LSHIFT) && _gameObjects.size() > 1)
    {
        _gameObjects.back()->SetRotation(rotation.x, rotation.y, rotation.z -= (0.02f * objectTransformRate));
    }
    if (GetAsyncKeyState(VK_LEFT) && GetAsyncKeyState(VK_LCONTROL) && _gameObjects.size() > 1)
    {
        _gameObjects.back()->SetRotation(rotation.x, rotation.y += (0.02f * objectTransformRate), rotation.z);
    }
    if (GetAsyncKeyState(VK_RIGHT) && GetAsyncKeyState(VK_LCONTROL) && _gameObjects.size() > 1)
    {
        _gameObjects.back()->SetRotation(rotation.x, rotation.y -= (0.02f * objectTransformRate), rotation.z);
    }

    //Scale the size of the most recently created object
    if (GetAsyncKeyState(VK_ADD))
        objectScaleNumber += (0.01f * objectTransformRate);
    if (GetAsyncKeyState(VK_SUBTRACT) && objectScaleNumber > 0.02f)
        objectScaleNumber -= (0.01f * objectTransformRate);
    if (_gameObjects.size() > 1)
        _gameObjects.back()->SetScale(objectScaleNumber, objectScaleNumber, objectScaleNumber);

    //Change which object is created
    if (GetAsyncKeyState('0'))
        objectCreateNumber = 0;
    if (GetAsyncKeyState('1'))
        objectCreateNumber = 1;
    if (GetAsyncKeyState('2'))
        objectCreateNumber = 2;
    if (GetAsyncKeyState('3'))
        objectCreateNumber = 3;

    //Change the colour of the light
    if (GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState('W')) //Purple
    {
        XMFLOAT4 specLight = _lighting[0]->GetSpecularLight();
        XMFLOAT4 ambLight = _lighting[0]->GetAmbientLight();
        _lighting[0]->SetSpecularLight(specLight.x, specLight.y, specLight.z += 0.01f, specLight.w);
        _lighting[0]->SetAmbientLight(ambLight.x, ambLight.y, ambLight.z += 0.01f, ambLight.w);
    }
    if (GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState('S')) //Yellow
    {
        XMFLOAT4 specLight = _lighting[0]->GetSpecularLight();
        XMFLOAT4 ambLight = _lighting[0]->GetAmbientLight();
        _lighting[0]->SetSpecularLight(specLight.x, specLight.y, specLight.z -= 0.01f, specLight.w);
        _lighting[0]->SetAmbientLight(ambLight.x, ambLight.y, ambLight.z -= 0.01f, ambLight.w);
    }
    if (GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState('A')) //Red
    {
        XMFLOAT4 specLight = _lighting[0]->GetSpecularLight();
        XMFLOAT4 ambLight = _lighting[0]->GetAmbientLight();
        _lighting[0]->SetSpecularLight(specLight.x += 0.01f, specLight.y, specLight.z, specLight.w);
        _lighting[0]->SetAmbientLight(ambLight.x += 0.01f, ambLight.y, ambLight.z += 0.01f, ambLight.w);
    }
    if (GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState('D')) //Blue
    {
        XMFLOAT4 specLight = _lighting[0]->GetSpecularLight();
        XMFLOAT4 ambLight = _lighting[0]->GetAmbientLight();
        _lighting[0]->SetSpecularLight(specLight.x -= 0.01f, specLight.y, specLight.z, specLight.w);
        _lighting[0]->SetAmbientLight(ambLight.x -= 0.01f, ambLight.y, ambLight.z, ambLight.w);
    }
    
    //Toggle rendering everything as wireframes
    if (GetAsyncKeyState(VK_MULTIPLY) && isAsteriskDown)
        isAsteriskDown = true;
    else
        isAsteriskDown = false;

    if (GetAsyncKeyState(VK_MULTIPLY) && !isAsteriskDown)
    {
        showWireFrame = !showWireFrame;
        isAsteriskDown = true;
    }

    //Exit the game
    if (GetAsyncKeyState(VK_ESCAPE))
    {
        exit(0);
    }
}

void Application::Update()
{
    // Update our time
    static float deltaTime = 0.0f;
    static DWORD dwTimeStart = 0.0f;
    const float frameRate = 1.0f / 60.0f;
    //const float frameRate = 60.0f;

    DWORD dwTimeCur = GetTickCount();

    if (dwTimeStart == 0)
        dwTimeStart = dwTimeCur;
 
    deltaTime += (dwTimeCur - dwTimeStart) / 1000.0f;

    if (deltaTime < frameRate)
    {
        return;
    }
    else
    {
        //Updates all the objects in the game
        for (auto gameObject : _gameObjects)
        {
            gameObject->Update(deltaTime);
        }

        //Update the direction the camera is looking using the mouse
        XMFLOAT3 atPos = _cameras[0]->GetLookAt();
        POINT cursorPosOld, cursorPosNew;
        cursorPosOld.x = _WindowWidth / 2;
        cursorPosOld.y = _WindowHeight / 2;
        mouseMovementDetectDelay++;

        if (mouseMovement)
        {
            if (mouseMovementDetectDelay == 3)
            {
                //SetCursorPos(cursorPosOld.x, cursorPosOld.y);
                SetCursorPos(_WindowWidth / 2, _WindowHeight / 2);
                mouseMovementDetectDelay = 0;
            }

            if (selectedCameraNum == 0)
            {
                GetCursorPos(&cursorPosNew);
                if (cursorPosOld.y < cursorPosNew.y && cursorPosOld.x == cursorPosNew.x) //Detect looking down
                    atPos.y -= 0.01f;
                if (cursorPosOld.x > cursorPosNew.x && cursorPosOld.y == cursorPosNew.y) //Left
                    atPos.x -= 0.01f;
                if (cursorPosOld.x < cursorPosNew.x && cursorPosOld.y == cursorPosNew.y) //Right
                    atPos.x += 0.01f;
                if (cursorPosOld.y > cursorPosNew.y && cursorPosOld.x == cursorPosNew.x) //Up
                    atPos.y += 0.01f;
                if (cursorPosOld.y > cursorPosNew.y && cursorPosOld.x < cursorPosNew.x) //Up right
                {
                    atPos.x += 0.007f;
                    atPos.y += 0.007f;
                }
                if (cursorPosOld.y > cursorPosNew.y && cursorPosOld.x > cursorPosNew.x) //Up left
                {
                    atPos.x -= 0.007f;
                    atPos.y += 0.007f;
                }
                if (cursorPosOld.y < cursorPosNew.y && cursorPosOld.x < cursorPosNew.x) //Down right
                {
                    atPos.x += 0.007f;
                    atPos.y -= 0.007f;
                }
                if (cursorPosOld.y < cursorPosNew.y && cursorPosOld.x > cursorPosNew.x) //Down left
                {
                    atPos.x -= 0.007f;
                    atPos.y -= 0.007f;
                }
            }
        }
        
        //Update the cameras that move
        _cameras[0]->SetLookAt(atPos);
        _cameras[0]->Update();
        _cameras[3]->Update();

        //Toggle between rendering wireframe
        if (showWireFrame)
        {
            _pImmediateContext->RSSetState(_wireFrame);
        }
        else
        {
            _pImmediateContext->RSSetState(NULL);
        }

        UserKeyboardInput();
        _lighting[0]->Update();
    }


    deltaTime = deltaTime - frameRate;

}

void Application::Draw()
{
    //
    // Clear the back buffer
    //
    float ClearColor[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // red,green,blue,alpha
    _pImmediateContext->ClearRenderTargetView(_pRenderTargetView, ClearColor);
    _pImmediateContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    //
    // Update variables
    //
    ConstantBuffer cb;

    _pImmediateContext->VSSetShader(_pVertexShader, nullptr, 0);
    _pImmediateContext->VSSetConstantBuffers(0, 1, &_pConstantBuffer);
    _pImmediateContext->PSSetShader(_pPixelShader, nullptr, 0);
    _pImmediateContext->PSSetConstantBuffers(0, 1, &_pConstantBuffer);
    _pImmediateContext->PSSetSamplers(0, 1, &_pSamplerLinear);

    cb.mView = XMMatrixTranspose(XMLoadFloat4x4(&_cameras[selectedCameraNum]->GetView()));
    cb.mProjection = XMMatrixTranspose(XMLoadFloat4x4(&_cameras[selectedCameraNum]->GetProjection()));
    cb.DiffuseLight = _lighting[0]->GetDiffuseLight();
    cb.AmbientLight = _lighting[0]->GetAmbientLight();
    cb.SpecularLight = _lighting[0]->GetSpecularLight();
    cb.SpecularPower = _lighting[0]->GetSpecularPower();
    cb.LightVecW = _lighting[0]->GetLightDirection();
    cb.EyePosW = _cameras[selectedCameraNum]->GetPosition();

    for (auto gameObject : _gameObjects)
    {
        Material material = gameObject->GetMaterialData();

        _pImmediateContext->IASetInputLayout(_pVertexLayout);

        cb.mWorld = XMMatrixTranspose(gameObject->GetWorldMatrix());

        cb.DiffuseMtrl = material.diffuse;
        cb.AmbientMtrl = material.ambient;
        cb.SpecularMtrl = material.specular;

        if (gameObject->HasTexture())
        {
            ID3D11ShaderResourceView* textureRV = gameObject->GetTextureRV();
            _pImmediateContext->PSSetShaderResources(0, 1, &textureRV);
        }

        _pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

        gameObject->Draw(_pImmediateContext);
    }

    //
    // Present our back buffer to our front buffer
    //
    _pSwapChain->Present(0, 0);
}