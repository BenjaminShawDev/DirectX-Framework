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
	_pVertexBufferCube = nullptr;
	_pIndexBufferCube = nullptr;
    _pVertexBufferPyramid = nullptr;
    _pIndexBufferPyramid = nullptr;
    _pVertexBufferPlane = nullptr;
    _pIndexBufferPlane = nullptr;
	_pConstantBuffer = nullptr;
    _pTextureRV = nullptr;
    _pSamplerLinear = nullptr;
    _camera = nullptr;
    _camera2 = nullptr;
    _transparency = nullptr;
    gameObject = nullptr;
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
    //Diffuse material properties (RGBA)
    diffuseMaterial = XMFLOAT4(0.8f, 0.5f, 0.5f, 1.0f);
    //Diffuse light colour (RGBA)
    diffuseLight = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    //Ambient
    ambientMaterial = XMFLOAT4(0.2f, 0.2f, 0.2f, 0.2f);
    ambientLight = XMFLOAT4(0.2f, 0.2f, 0.2f, 0.2f);

    //Specular
    specularMaterial = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    specularLight = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    specularPower = 10.0f;

    //Eye pos
    eyePosW = XMFLOAT3(0.0f, 0.0f, -5.0f);

    selectedCameraNum = 0;

    XMFLOAT3 eye = XMFLOAT3(0.0f, 2.0f, -20.0f);
    XMFLOAT3 at = XMFLOAT3(0.0f, 1.0f, 10.0f);
    XMFLOAT3 up = XMFLOAT3(0.0f, 1.0f, 0.0f);

    _camera = new Camera(eye, at, up, (float)_renderWidth, (float)_renderHeight, 0.01f, 200.0f, true);

    XMFLOAT3 eye2 = XMFLOAT3(20.0f, 1.0f, 0.0f);
    XMFLOAT3 at2 = XMFLOAT3(0.0f, 1.0f, 00.0f);
    XMFLOAT3 up2 = XMFLOAT3(0.0f, 1.0f, 00.0f);

    _camera2 = new Camera(eye2, at2, up2, (float)_renderWidth, (float)_renderHeight, 0.01f, 200.0f, false);

    // Initialize the projection matrix
	//XMStoreFloat4x4(&_projection, XMMatrixPerspectiveFovLH(XM_PIDIV2, _WindowWidth / (FLOAT) _WindowHeight, 0.01f, 100.0f));

    floorMeshData = OBJLoader::Load("Assets/Plane.obj", _pd3dDevice, false);
    cubeMeshData = OBJLoader::Load("Assets/Cube.obj", _pd3dDevice, false);
    aeroplaneMeshData = OBJLoader::Load("Assets/Hercules.obj", _pd3dDevice, false);
    //aeroplaneMeshData = OBJLoader::Load("Assets/Cylinder.obj", _pd3dDevice, false);

    CreateDDSTextureFromFile(_pd3dDevice, L"Assets/Brick.dds", nullptr, &floorTextureData);
    CreateDDSTextureFromFile(_pd3dDevice, L"Assets/PineTree.dds", nullptr, &cubeTextureData);
    CreateDDSTextureFromFile(_pd3dDevice, L"Assets/HERCULES_COLOR.dds", nullptr, &aeroplaneTextureData);
    //CreateDDSTextureFromFile(_pd3dDevice, L"Assets/Cylinder2.dds", nullptr, &aeroplaneTextureData);

    //Geometry floorGeometry;
    floorGeometry.indexBuffer = floorMeshData.IndexBuffer;
    floorGeometry.vertexBuffer = floorMeshData.VertexBuffer;
    floorGeometry.numberOfIndices = floorMeshData.IndexCount;
    floorGeometry.vertexBufferOffset = floorMeshData.VBOffset;
    floorGeometry.vertexBufferStride = floorMeshData.VBStride;

    //Geometry cubeGeometry;
    cubeGeometry.indexBuffer = cubeMeshData.IndexBuffer;
    cubeGeometry.vertexBuffer = cubeMeshData.VertexBuffer;
    cubeGeometry.numberOfIndices = cubeMeshData.IndexCount;
    cubeGeometry.vertexBufferOffset = cubeMeshData.VBOffset;
    cubeGeometry.vertexBufferStride = cubeMeshData.VBStride;

    //Geometry aeroplaneGeometry;
    aeroplaneGeometry.indexBuffer = aeroplaneMeshData.IndexBuffer;
    aeroplaneGeometry.vertexBuffer = aeroplaneMeshData.VertexBuffer;
    aeroplaneGeometry.numberOfIndices = aeroplaneMeshData.IndexCount;
    aeroplaneGeometry.vertexBufferOffset = aeroplaneMeshData.VBOffset;
    aeroplaneGeometry.vertexBufferStride = aeroplaneMeshData.VBStride;

    //Material shinyMaterial;
    shinyMaterial.ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
    shinyMaterial.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    shinyMaterial.specular = XMFLOAT4(0.5f, 0.5f, 0.5, 1.0f);
    shinyMaterial.specularPower = 10.0f;

    //Material noSpecMaterial;
    noSpecMaterial.ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
    noSpecMaterial.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    noSpecMaterial.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    noSpecMaterial.specularPower = 0.0f;

    gameObject = new GameObject("Floor", floorGeometry, noSpecMaterial);
    gameObject->SetPosition(0.0f, 0.0f, 0.0f);
    gameObject->SetScale(1.0f, 1.0f, 1.0f);
    gameObject->SetRotation(0.0f, 0.0f, 0.0f);
    gameObject->SetTextureRV(floorTextureData);

    _gameObjects.push_back(gameObject);

    gameObject = new GameObject("Cube", cubeGeometry, shinyMaterial);
    gameObject->SetPosition(0.0f, 1.0f, 0.0f);
    gameObject->SetScale(1.0f, 1.0f, 1.0f);
    gameObject->SetRotation(0.0f, 0.0f, 0.0f);
    gameObject->SetTextureRV(cubeTextureData);

    _gameObjects.push_back(gameObject);

    gameObject = new GameObject("Aeroplane", aeroplaneGeometry, shinyMaterial);
    gameObject->SetPosition(2.0f, 2.0f, 0.0f);
    gameObject->SetScale(0.2f, 0.2f, 0.2f);
    gameObject->SetRotation(0.0f, 0.0f, 0.0f);
    gameObject->SetTextureRV(aeroplaneTextureData);

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

    _pd3dDevice->CreateBlendState(&blendDesc, &_transparency);

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
    if (_pVertexBufferCube) _pVertexBufferCube->Release();
    if (_pIndexBufferCube) _pIndexBufferCube->Release();
    if (_pVertexBufferPyramid) _pVertexBufferPyramid->Release();
    if (_pIndexBufferPyramid) _pIndexBufferPyramid->Release();
    if (_pVertexBufferPlane) _pVertexBufferPlane->Release();
    if (_pIndexBufferPlane) _pIndexBufferPlane->Release();
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
}

void Application::CreateObject(int objectNum)
{
    switch (objectNum)
    {
    case 0:
        gameObject = new GameObject("Floor", floorGeometry, noSpecMaterial);
        gameObject->SetPosition(_camera->GetPosition());
        gameObject->SetScale(objectScaleNumber, objectScaleNumber, objectScaleNumber);
        gameObject->SetRotation(0.0f, 0.0f, 0.0f);
        gameObject->SetTextureRV(floorTextureData);

        _gameObjects.push_back(gameObject);
        break;
    case 1:
        gameObject = new GameObject("Cube", cubeGeometry, shinyMaterial);
        gameObject->SetPosition(_camera->GetPosition());
        gameObject->SetScale(objectScaleNumber, objectScaleNumber, objectScaleNumber);
        gameObject->SetRotation(0.0f, 0.0f, 0.0f);
        gameObject->SetTextureRV(cubeTextureData);

        _gameObjects.push_back(gameObject);
        break;
    case 2:
        gameObject = new GameObject("Aeroplane", aeroplaneGeometry, shinyMaterial);
        gameObject->SetPosition(_camera->GetPosition());
        gameObject->SetScale(objectScaleNumber, objectScaleNumber, objectScaleNumber);
        gameObject->SetRotation(0.0f, 0.0f, 0.0f);
        gameObject->SetTextureRV(aeroplaneTextureData);

        _gameObjects.push_back(gameObject);
        break;
    default:
        break;
    }
}

void Application::Update()
{
    // Update our time
    static float deltaTime = 0.0f;
    static DWORD dwTimeStart = 0.0f;
    const float frameRate = 1.0f / 60.0f;

    DWORD dwTimeCur = GetTickCount();

    //if (dwTimeStart == 0)
    //    dwTimeStart = dwTimeCur;
    deltaTime += (dwTimeCur - dwTimeStart) / 1000.0f;

    //if (_driverType == D3D_DRIVER_TYPE_REFERENCE)
    //{
    //    t += (float) XM_PI * 0.0125f;
    //}
    //else
    //{
    //    static DWORD dwTimeStart = 0;
    //    DWORD dwTimeCur = GetTickCount();

    //    if (dwTimeStart == 0)
    //        dwTimeStart = dwTimeCur;

    //    t = (dwTimeCur - dwTimeStart) / 1000.0f;
    //}

    
    if (deltaTime < frameRate)
    {
        return;
    }
    else
    {
        for (auto gameObject : _gameObjects)
        {
            gameObject->Update(deltaTime);
        }

        cameraDetectDelay++;

        XMFLOAT3 cubeRotation = _gameObjects[1]->GetRotation();
        cubeRotation.y += 0.01f;
        _gameObjects[1]->SetRotation(cubeRotation);

        POINT cursorPosOld, cursorPosNew;
        cursorPosOld.x = _WindowWidth / 2;
        cursorPosOld.y = _WindowHeight / 2;

        if (cameraDetectDelay == 3)
        {
            SetCursorPos(cursorPosOld.x, cursorPosOld.y);
            cameraDetectDelay = 0;
        }

        //Camera controls
        if (GetAsyncKeyState(VK_UP))
            selectedCameraNum = 0;
        if (GetAsyncKeyState(VK_DOWN))
            selectedCameraNum = 1;

        if (selectedCameraNum == 0)
        {
            XMFLOAT3 cameraPos = _camera->GetPosition();
            XMFLOAT3 atPos = _camera->GetLookAt();
            if (GetAsyncKeyState(VK_ESCAPE))
            {
                exit(0);
            }

            if (GetAsyncKeyState(0x57))
            {
                cameraPos.z += 0.1f;
            }
            if (GetAsyncKeyState(0x53))
            {
                cameraPos.z -= 0.1f;
            }
            if (GetAsyncKeyState(0x41))
            {
                cameraPos.x -= 0.1f;
            }
            if (GetAsyncKeyState(0x44))
            {
                cameraPos.x += 0.1f;
            }

            if (GetAsyncKeyState(VK_NUMPAD0))
                objectCreateNumber = 0;
            if (GetAsyncKeyState(VK_NUMPAD1))
                objectCreateNumber = 1;
            if (GetAsyncKeyState(VK_NUMPAD2))
                objectCreateNumber = 2;

            if (GetAsyncKeyState(VK_ADD))
                objectScaleNumber += 0.01f;
            if (GetAsyncKeyState(VK_SUBTRACT))
                objectScaleNumber -= 0.01f;

            if (objectScaleNumber < 0.01f)
                objectScaleNumber = 0.02f;

            if (GetAsyncKeyState(MK_LBUTTON))
            {
                CreateObject(objectCreateNumber);
            }

            POINT mouseDirection;
            mouseDirection.x = mouseDirection.y = 0;

            GetCursorPos(&cursorPosNew);
            if (cursorPosOld.y < cursorPosNew.y && cursorPosOld.x == cursorPosNew.x) //Detect looking down
                atPos.y -= 0.1f;
            if (cursorPosOld.x > cursorPosNew.x && cursorPosOld.y == cursorPosNew.y) //Right
                atPos.x -= 0.1f;
            if (cursorPosOld.x < cursorPosNew.x && cursorPosOld.y == cursorPosNew.y) //Left
                atPos.x += 0.1f;
            if (cursorPosOld.y > cursorPosNew.y && cursorPosOld.x == cursorPosNew.x) //Up
                atPos.y += 0.1f;
            if (cursorPosOld.y > cursorPosNew.y && cursorPosOld.x < cursorPosNew.x) //Up left
            {
                atPos.x += 0.07f;
                atPos.y += 0.07f;
            }
            if (cursorPosOld.y > cursorPosNew.y && cursorPosOld.x > cursorPosNew.x) //Up right
            {
                atPos.x -= 0.07f;
                atPos.y += 0.07f;
            }
            if (cursorPosOld.y < cursorPosNew.y && cursorPosOld.x < cursorPosNew.x) //Down left
            {
                atPos.x += 0.07f;
                atPos.y -= 0.07f;
            }
            if (cursorPosOld.y < cursorPosNew.y && cursorPosOld.x > cursorPosNew.x) //Down right
            {
                atPos.x -= 0.07f;
                atPos.y -= 0.07f;
            }

            //float distanceBetweenPosAndAt = sqrt(pow((atPos.x + cameraPos.x), 2) + pow((atPos.y + cameraPos.y), 2) + pow((atPos.z - cameraPos.z), 2));

            //if (distanceBetweenPosAndAt > 30)
            //{
            //    atPos.z -= 0.1f;
            //}
            //if (distanceBetweenPosAndAt < 30)
            //{
            //    atPos.z += 0.1f;
            //}

            //if (GetAsyncKeyState(VK_NUMPAD8))
            //    _cameraOrbitRadius = max(_cameraOrbitRadiusMin, _cameraOrbitRadius - (_cameraSpeed * 0.2f));
            //if (GetAsyncKeyState(VK_NUMPAD2))
            //    _cameraOrbitRadius = min(_cameraOrbitRadiusMax, _cameraOrbitRadius + (_cameraSpeed * 0.2f));
            //if (GetAsyncKeyState(VK_NUMPAD6))
            //    _cameraOrbitAngleXZ += _cameraSpeed;
            //if (GetAsyncKeyState(VK_NUMPAD4))
            //    _cameraOrbitAngleXZ -= _cameraSpeed;

            //float angleAroundZ = XMConvertToRadians(_cameraOrbitAngleXZ);
            //float x = _cameraOrbitRadius * cos(angleAroundZ);
            //float z = _cameraOrbitRadius * sin(angleAroundZ);

            //cameraPos.x = x;
            //cameraPos.z = z;

            _camera->SetPosition(cameraPos);
            _camera->SetLookAt(atPos);
            _camera->Update();
        }

        if (selectedCameraNum == 1)
        {
            XMFLOAT3 cameraPos = _camera2->GetPosition();
            _camera2->SetPosition(cameraPos);
            _camera2->Update();
        }

        if (GetAsyncKeyState(VK_RETURN))
        {
            XMFLOAT3 position = _gameObjects[1]->GetPosition();
            position.z += 0.02f;
            _gameObjects[1]->SetPosition(position);
        }
        else if (GetAsyncKeyState(VK_BACK))
        {
            XMFLOAT3 position = _gameObjects[1]->GetPosition();
            position.z -= 0.02f;
            _gameObjects[1]->SetPosition(position);
        }
    }

    deltaTime = deltaTime - frameRate;

    //XMStoreFloat4x4(&_world, XMMatrixRotationY(t));
    //XMStoreFloat4x4(&_world, XMMatrixTranslation(0.0f, 0.0f, 0.0f));
    //XMStoreFloat4x4(&_world2, XMMatrixTranslation(0.0f, 3.0f, 0.0f));
    //XMStoreFloat4x4(&_world2, XMMatrixRotationZ(t) * XMMatrixTranslation(7.5f, 0.0f, 2.0f) * XMMatrixScaling(0.4f, 0.4f, 0.4f) * XMMatrixRotationZ(t * 1.3));
    //XMStoreFloat4x4(&_world3, XMMatrixRotationZ(t) * XMMatrixTranslation(10.0f, 0.0f, 2.0f) * XMMatrixScaling(0.4f, 0.4f, 0.4f) * XMMatrixRotationZ(t));
    //XMStoreFloat4x4(&_world4, XMMatrixRotationZ(t) * XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(5.0f, 0.0f, 2.0f));
    //XMStoreFloat4x4(&_world4, XMMatrixScaling(0.3f, 0.3f, 0.3f) * XMMatrixRotationZ(t) * XMMatrixTranslation(2.0f, 0.0f, 0.0f) * XMMatrixRotationZ(t) * XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixRotationZ(t) * XMMatrixTranslation(3.25f, 0.0f, 0.0f) * XMMatrixRotationZ(t));
    //XMStoreFloat4x4(&_world5, XMMatrixTranslation(0.0f, 0.0f, 4.0f));
}

void Application::Draw()
{
    //
    // Clear the back buffer
    //
    //float ClearColor[4] = {0.0f, 0.125f, 0.3f, 1.0f}; // red,green,blue,alpha
    float ClearColor[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // red,green,blue,alpha
    _pImmediateContext->ClearRenderTargetView(_pRenderTargetView, ClearColor);
    _pImmediateContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    //_pImmediateContext->IAGetInputLayout(_pVertexLayout);

    _pImmediateContext->VSSetShader(_pVertexShader, nullptr, 0);
    _pImmediateContext->VSSetConstantBuffers(0, 1, &_pConstantBuffer);
    _pImmediateContext->PSSetShader(_pPixelShader, nullptr, 0);
    _pImmediateContext->PSSetConstantBuffers(0, 1, &_pConstantBuffer);
    _pImmediateContext->PSSetSamplers(0, 1, &_pSamplerLinear);

	//XMMATRIX world = XMLoadFloat4x4(&_world);
	XMMATRIX view = XMLoadFloat4x4(&_camera->GetView());
	XMMATRIX projection = XMLoadFloat4x4(&_camera->GetProjection());
    XMMATRIX view2 = XMLoadFloat4x4(&_camera2->GetView());
    XMMATRIX projection2 = XMLoadFloat4x4(&_camera2->GetProjection());

    //
    // Update variables
    //
    ConstantBuffer cb;
	//cb.mWorld = XMMatrixTranspose(world);
    //cb.mWorld = XMMatrixTranspose(gameobject->GetWorldMatrix())
    if (selectedCameraNum == 1)
    {
        cb.mView = XMMatrixTranspose(view2);
        cb.mProjection = XMMatrixTranspose(projection2);
    }
    else
    {
        cb.mView = XMMatrixTranspose(view);
        cb.mProjection = XMMatrixTranspose(projection);
    }

    //cb.gTime = timeFromUpdateFunction;
    cb.DiffuseMtrl = diffuseMaterial;
    cb.DiffuseLight = diffuseLight;
    cb.LightVecW = lightDirection;
    cb.AmbientMtrl = ambientMaterial;
    cb.AmbientLight = ambientLight;
    cb.SpecularMtrl = specularMaterial;
    cb.SpecularLight = specularLight;
    cb.SpecularPower = specularPower;
    //cb.EyePosW = eyePosW;
    cb.EyePosW = _camera->GetPosition();

    if (GetAsyncKeyState(VK_LEFT))
    {
        _pImmediateContext->RSSetState(NULL);
    }

    else if (GetAsyncKeyState(VK_RIGHT))
    {
        _pImmediateContext->RSSetState(_wireFrame);
    }

    for (auto gameObject : _gameObjects)
    {
        Material material = gameObject->GetMaterialData();
        cb.AmbientMtrl = material.ambient;
        cb.DiffuseMtrl = material.diffuse;
        cb.SpecularMtrl = material.specular;

        cb.mWorld = XMMatrixTranspose(gameObject->GetWorldMatrix());

        if (gameObject->HasTexture())
        {
            ID3D11ShaderResourceView* textureRV = gameObject->GetTextureRV();
            _pImmediateContext->PSSetShaderResources(0, 1, &textureRV);
        }

        //_pImmediateContext->PSSetSamplers(0, 1, &_pSamplerLinear);

        _pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

        gameObject->Draw(_pImmediateContext);
    }

    //"Fine-tune" the blending equation
    //float blendFactor[] = { 0.75f, 0.75f, 0.75f, 1.0f };
    //Set the default blend state (no blending) for opaque objects
    //_pImmediateContext->OMSetBlendState(0, 0, 0xffffffff);
    //Render opaque objects
    //Set the blend state for transparent objects
    //_pImmediateContext->OMSetBlendState(_transparency, blendFactor, 0xffffffff);

    //world = XMLoadFloat4x4(&_world2);
    //cb.mWorld = XMMatrixTranspose(world);
    //_pImmediateContext->PSSetShaderResources(0, 1, &objTextureData2);
    //_pImmediateContext->IASetVertexBuffers(0, 1, &objMeshData2.VertexBuffer, &objMeshData2.VBStride, &objMeshData2.VBOffset);
    //_pImmediateContext->IASetIndexBuffer(objMeshData2.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    //_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    //_pImmediateContext->DrawIndexed(objMeshData2.IndexCount, 0, 0);

    //world = XMLoadFloat4x4(&_world2);
    //cb.mWorld = XMMatrixTranspose(world);
    //_pImmediateContext->IASetVertexBuffers(0, 1, &_pVertexBufferCube, &stride, &offset);
    //_pImmediateContext->IASetIndexBuffer(_pIndexBufferCube, DXGI_FORMAT_R16_UINT, 0);
    //_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
    //_pImmediateContext->DrawIndexed(cubeIndexNum, 0, 0);

    //
    // Present our back buffer to our front buffer
    //
    _pSwapChain->Present(0, 0);
}