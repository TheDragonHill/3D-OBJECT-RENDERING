#include <Windows.h>
#include <d3d11.h>
#include <d3d10.h>
#include <d3dx11.h>
#include <d3dx9.h>
#include <xnamath.h>
#include <dinput.h>

#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "Winmm.lib")
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3d10.lib")
#pragma comment (lib, "d3dx11.lib")

#define KUCHEN 3.1415f

// -------- win32 Stuff
// Long Pointer Controll Type String
// Mit L wird die Zeichenfolge zu einem Long Pointer konvertiert
LPCTSTR WndClassName = L"Zwei Fenster um sie alle zu beherschen";

// Eventhandler
HWND hwnd = NULL;

// NULL ist ein eigenes Objekt für Windows
// -> Ein leerer Speicher, 0 würde auch funktionieren

const int Width = 1280;
const int Height = 720;

// --------- DirectX 11 Stuff

// Interface DirectX Graphic Interface Swap Chain
IDXGISwapChain* Swapchain;
ID3D11Device* device;
ID3D11DeviceContext* d3ddevcon;
ID3D11RenderTargetView* renderTargetView;

ID3D11Buffer* cbPerObjectBuffer;

struct cbPerObject
{
	XMMATRIX WVP;
	XMMATRIX World;
};
cbPerObject cbPerObj;

ID3D11Buffer* vertexBuffer;
ID3D11Buffer* indexBuffer;
ID3D11VertexShader* VS;
ID3D11PixelShader* PS;
ID3D10Blob* VS_Buffer;
ID3D10Blob* PS_Buffer;
ID3D11InputLayout* vertLayout;

ID3D11RasterizerState* RS_WireFrame;
ID3D11RasterizerState* RS_Solid;

ID3D11DepthStencilView* depthStencilView;
ID3D11Texture2D* depthStencilBuffer;

ID3D11ShaderResourceView* CubesTexture;
ID3D11SamplerState* CubesTextureSamplerState;

IDirectInputDevice8* DIKeyboard;
IDirectInputDevice8* DIMouse;

DIMOUSESTATE mouseLastState;
DIMOUSESTATE mouseCurrState;
BYTE keyboardState[256];
LPDIRECTINPUT8 DirectInput;

bool QUIT = false;

struct Light
{
	Light()
	{
		ZeroMemory(this, sizeof(Light));
	};

	XMFLOAT3 dir;
	float pad;
	XMFLOAT3 pos;
	float range;
	XMFLOAT3 att;
	float pad2;
	XMFLOAT4 ambient;
	XMFLOAT4 diffuse;
};

Light light;
ID3D11Buffer* cbPerFrameBuffer;

struct cbPerFrame
{
	Light light;
};
cbPerFrame cbPerFrm;

XMMATRIX WVP;
XMMATRIX World;
XMMATRIX World2;
XMMATRIX World3;
XMMATRIX View;
XMMATRIX Projection;

XMVECTOR camPos;
XMVECTOR camTarget;
XMVECTOR camUp;

XMMATRIX Translate;
XMMATRIX Rotation;


struct Vertex
{
	Vertex(){}
	Vertex(float x, float y, float z, float u, float v, float nx, float ny, float nz) : pos(x, y, z), texCoord(u,v), normal(nx,ny,nz){}

	XMFLOAT3 pos;
	XMFLOAT2 texCoord;
	XMFLOAT3 normal;
};

D3D11_INPUT_ELEMENT_DESC layout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,20,D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

UINT numElements = ARRAYSIZE(layout);

// --------- Prototypes

bool InitDirectInput(HINSTANCE hInstance);
void DetectInput(float deltaTime);
bool SetupScene();
void Update(float deltaTime);
void Draw(float deltaTime);
void CleanUp();

bool InitWindow(
	HINSTANCE hInstance,
	int ShowWnd,
	int width,
	int height,
	bool windowed
);
int messageLoop();

// Callback von Windows mit codierten Nachrichten
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

int WinMain(HINSTANCE hInstance, HINSTANCE prevHinstance, LPSTR lpCmdLine, int nShowCmd)
{
	// Wenn Init nicht funktioniert
	// Mit D3DDEVTYPE_HAL wird gesagt, dass die Grafikkarte verwendet werden soll
	if (!InitWindow(hInstance, nShowCmd, Width, Height, true))
	{
		MessageBox(0, L"InitWindow(hInstance, nShowCmd, Width, Height, true) failed", L"Einfach so Error", MB_OK);
		// return 0 alles ok
		// Alles andere heißt Fehler
		// return 1 = Fenster nicht ok
		return 1;
	}

	if (!SetupScene())
	{
		MessageBox(0, L"SetupScene() failed", L"Einfach so Error", MB_OK);
		return 1;
	}


	if (!InitDirectInput(hInstance))
	{
		MessageBox(0, L"DIRECT INPUT INIT failed", L"Artist Error", MB_OK);
		return 1;
	}

	messageLoop();
	CleanUp();

	// Alles ok beendet
	return 0;
}

bool InitDirectInput(HINSTANCE hInstance)
{
	DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)& DirectInput, NULL);
	DirectInput->CreateDevice(GUID_SysKeyboard, &DIKeyboard, NULL);
	DirectInput->CreateDevice(GUID_SysMouse, &DIMouse, NULL);

	DIKeyboard->SetDataFormat(&c_dfDIKeyboard);
	DIKeyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

	DIMouse->SetDataFormat(&c_dfDIMouse);
	DIMouse->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND);

	return true;
}

void DetectInput(float deltaTime)
{
	DIKeyboard->Acquire();
	DIMouse->Acquire();

	DIMouse->GetDeviceState(sizeof(DIMOUSESTATE), &mouseCurrState);
	DIKeyboard->GetDeviceState(sizeof(keyboardState), (LPVOID)&keyboardState);

	return;
}

bool SetupScene()
{
	D3DX11CompileFromFile(L"shader.hlsl", 0, 0, "VS", "vs_4_0", 0, 0, 0, &VS_Buffer, 0, 0);
	D3DX11CompileFromFile(L"shader.hlsl", 0, 0, "PS", "ps_4_0", 0, 0, 0, &PS_Buffer, 0, 0);

	device->CreateVertexShader(VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), NULL, &VS);
	device->CreatePixelShader(PS_Buffer->GetBufferPointer(), PS_Buffer->GetBufferSize(), NULL, &PS);

	d3ddevcon->VSSetShader(VS, 0, 0);
	d3ddevcon->PSSetShader(PS, 0, 0);

	light.dir = XMFLOAT3(0.3f, 0.5f, -1);
	light.pos = XMFLOAT3(2.0f, 3.0f, -1);
	light.range = 100.0f;
	light.att = XMFLOAT3(0.0f, 0.6f, 0.0f);
	light.ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1);
	light.diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1);

	Vertex v[] =
    {
        // Front Face
        Vertex(-1.0f, -1.0f,  -1.0f, 0.0f, 1.0f,	-1.0f, -1.0f,  -1.0f),
        Vertex(-1.0f,  1.0f,  -1.0f, 0.0f, 0.0f,	-1.0f,  1.0f,  -1.0f),
        Vertex(1.0f,   1.0f, -1.0f, 1.0f, 0.0f,		1.0f,   1.0f, -1.0f),
        Vertex(1.0f,  -1.0f, -1.0f, 1.0f, 1.0f,		1.0f,-1.0f, -1.0f),
 
        // Back Face
        Vertex(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f,		-1.0f,-1.0f, 1.0f),
        Vertex(1.0f,  -1.0f, 1.0f, 0.0f, 1.0f,		1.0f,-1.0f,1.0f),
        Vertex(1.0f,   1.0f, 1.0f, 0.0f, 0.0f,		1.0f,1.0f,1.0f),
        Vertex(-1.0f,  1.0f, 1.0f, 1.0f, 0.0f,		-1.0f,1.0f,1.0f),
 
        // Top Face
        Vertex(-1.0f, 1.0f, -1.0f, 0.0f, 1.0f,		-1.0f,1.0f,-1.0f),
        Vertex(-1.0f, 1.0f,  1.0f, 0.0f, 0.0f,		-1.0f,1.0f,1.0f),
        Vertex(1.0f,  1.0f,  1.0f, 1.0f, 0.0f,		1.0f,1.0f,1.0f),
        Vertex(1.0f,  1.0f, -1.0f, 1.0f, 1.0f,		1.0f,1.0f,-1.0f),
 
        // Bottom Face
        Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f,		-1.0f,-1.0f,-1.0f),
        Vertex( 1.0f, -1.0f, -1.0f, 0.0f, 1.0f,		1.0f,-1.0f,-1.0f),
        Vertex( 1.0f, -1.0f,  1.0f, 0.0f, 0.0f,		1.0f,-1.0f,1.0f),
        Vertex(-1.0f, -1.0f,  1.0f, 1.0f, 0.0f,		-1.0f,-1.0f,1.0f),
 
        // Left Face
        Vertex(-1.0f, -1.0f,  1.0f, 0.0f, 1.0f,		-1.0f,-1.0f,1.0f),
        Vertex(-1.0f,  1.0f,  1.0f, 0.0f, 0.0f,		-1.0f,1.0f,1.0f),
        Vertex(-1.0f,  1.0f, -1.0f, 1.0f, 0.0f,		-1.0f,1.0f,-1.0f),
        Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f,		-1.0f,-1.0f,-1.0f),
 
        // Right Face
        Vertex( 1.0f, -1.0f, -1.0f, 0.0f, 1.0f,		1.0f,-1.0f,-1.0f),
        Vertex( 1.0f,  1.0f, -1.0f, 0.0f, 0.0f,		1.0f,1.0f,-1.0f),
        Vertex( 1.0f,  1.0f,  1.0f, 1.0f, 0.0f,		1.0f,1.0f,1.0f),
        Vertex( 1.0f, -1.0f,  1.0f, 1.0f, 1.0f,		1.0f,-1.0f,1.0f),
    };

	DWORD indices[] = {
		// Front Face
		0,  1,  2,
		0,  2,  3,

		// Back Face
		4,  5,  6,
		4,  6,  7,

		// Top Face
		8,  9, 10,
		8, 10, 11,

		// Bottom Face
		12, 13, 14,
		12, 14, 15,

		// Left Face
		16, 17, 18,
		16, 18, 19,

		// Right Face
		20, 21, 22,
		20, 22, 23
	};

	//-------------------Index Buffer

	D3D11_BUFFER_DESC indexBufferDESC;
	ZeroMemory(&indexBufferDESC, sizeof(D3D11_BUFFER_DESC));

	indexBufferDESC.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDESC.ByteWidth = sizeof(DWORD) * 36 * 3;
	indexBufferDESC.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDESC.CPUAccessFlags = 0;
	indexBufferDESC.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA indexBufferData;
	ZeroMemory(&indexBufferData, sizeof(D3D11_SUBRESOURCE_DATA));

	indexBufferData.pSysMem = indices;
	device->CreateBuffer(&indexBufferDESC, &indexBufferData, &indexBuffer);

	d3ddevcon->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	//-------------Vertex Buffer

	D3D11_BUFFER_DESC vertexBufferDESC;
	ZeroMemory(&vertexBufferDESC, sizeof(D3D11_BUFFER_DESC));

	vertexBufferDESC.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDESC.ByteWidth = sizeof(Vertex) * 24;
	vertexBufferDESC.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDESC.CPUAccessFlags = 0;
	vertexBufferDESC.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;
	ZeroMemory(&vertexBufferData, sizeof(D3D11_SUBRESOURCE_DATA));

	vertexBufferData.pSysMem = v;
	device->CreateBuffer(&vertexBufferDESC, &vertexBufferData, &vertexBuffer);

	//set buffer
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	d3ddevcon->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	device->CreateInputLayout(layout, numElements, VS_Buffer->GetBufferPointer(),VS_Buffer->GetBufferSize(),&vertLayout);

	d3ddevcon->IASetInputLayout(vertLayout);

	d3ddevcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Height = Height;
	viewport.Width = Width;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	d3ddevcon->RSSetViewports(1, &viewport);

	//CB PER OBJECT
	D3D11_BUFFER_DESC cbDESC;
	ZeroMemory(&cbDESC, sizeof(D3D11_BUFFER_DESC));

	cbDESC.Usage = D3D11_USAGE_DEFAULT;
	cbDESC.ByteWidth = sizeof(cbPerObject);
	cbDESC.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDESC.CPUAccessFlags = 0;
	cbDESC.MiscFlags = 0;

	device->CreateBuffer(&cbDESC, NULL, &cbPerObjectBuffer);

	// CB PER FRAME
	ZeroMemory(&cbDESC, sizeof(D3D11_BUFFER_DESC));
	cbDESC.Usage = D3D11_USAGE_DEFAULT;
	cbDESC.ByteWidth = sizeof(cbPerFrame);
	cbDESC.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDESC.CPUAccessFlags = 0;
	cbDESC.MiscFlags = 0;
	device->CreateBuffer(&cbDESC, NULL, &cbPerFrameBuffer);

	//CAMERA
	camPos = XMVectorSet(0, 2, -5.0f, 0);
	camTarget = XMVectorSet(0, 0, 0, 0);
	camUp = XMVectorSet(0, 1, 0, 0);

	View = XMMatrixLookAtLH(camPos, camTarget, camUp);
	Projection = XMMatrixPerspectiveFovLH(0.5f * KUCHEN, (float)Width / (float)Height, 1.0f, 1000.0f);

	D3D11_RASTERIZER_DESC WireFrameDESC;
	ZeroMemory(&WireFrameDESC, sizeof(D3D11_RASTERIZER_DESC));
	WireFrameDESC.FillMode = D3D11_FILL_WIREFRAME;
	WireFrameDESC.CullMode = D3D11_CULL_NONE;
	device->CreateRasterizerState(&WireFrameDESC, &RS_WireFrame);

	D3D11_RASTERIZER_DESC SolidDESC;
	ZeroMemory(&SolidDESC, sizeof(D3D11_RASTERIZER_DESC));
	SolidDESC.FillMode = D3D11_FILL_SOLID;
	SolidDESC.CullMode = D3D11_CULL_BACK;
	device->CreateRasterizerState(&SolidDESC, &RS_Solid);

	D3DX11CreateShaderResourceViewFromFile(device, L"cubetex.png", NULL, NULL, &CubesTexture, NULL);
	D3D11_SAMPLER_DESC samplerDESC;
	ZeroMemory(&samplerDESC, sizeof(D3D11_SAMPLER_DESC));
	samplerDESC.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDESC.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDESC.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDESC.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDESC.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDESC.MinLOD = 0;
	samplerDESC.MaxLOD = D3D11_FLOAT32_MAX;

	device->CreateSamplerState(&samplerDESC, &CubesTextureSamplerState);

	return true;
}

void Update(float deltaTime)
{
	static float rotY = 0;
	World = XMMatrixIdentity();

	static float X_Axis = 0;
	static float Y_Axis = 0;
	if (keyboardState[DIK_LEFT] & 0x80)
	{
		//Taste Links gedrückt
		X_Axis -= 10 * deltaTime;
	}
	if (keyboardState[DIK_RIGHT] & 0x80)
	{
		//Taste Rechts gedrückt
		X_Axis += 10 * deltaTime;
	}
	if (keyboardState[DIK_UPARROW] & 0x80)
	{
		//Taste Rechts gedrückt
		Y_Axis += 10 * deltaTime;
	}
	if (keyboardState[DIK_DOWNARROW] & 0x80)
	{
		//Taste Rechts gedrückt
		Y_Axis -= 10 * deltaTime;
	}
	if (keyboardState[DIK_ESCAPE] & 0x80)
	{
		QUIT = true;
	}

	rotY += deltaTime;
	Rotation = XMMatrixRotationAxis(XMVectorSet(0, 1, 0, 1), rotY);
	Translate = XMMatrixTranslation(X_Axis, Y_Axis, 1);

	World = Rotation;
	World2 = Translate * Rotation;
	World3 = Rotation;

	//Light
	XMVECTOR lightVector = XMVectorSet(-3, 0, 1, 0);
	lightVector = XMVector3TransformCoord(lightVector, World);
	light.pos.x = XMVectorGetX(lightVector);
	light.pos.y = XMVectorGetY(lightVector);
	light.pos.z = XMVectorGetZ(lightVector);

	cbPerFrm.light = light;
	d3ddevcon->UpdateSubresource(cbPerFrameBuffer, 0, NULL, &cbPerFrm, 0, 0);
	d3ddevcon->PSSetConstantBuffers(0, 1, &cbPerFrameBuffer);

	d3ddevcon->VSSetShader(VS, 0, 0);
	d3ddevcon->PSSetShader(PS, 0, 0);
	
	mouseLastState = mouseCurrState;
}

void Draw(float deltaTime)
{
	static float r = 0.1f;
	static float g = 0.3f;
	static float b = 0.6f;

	r += deltaTime * 9 * 0.01f;
	g += deltaTime * 8 * 0.01f;
	b += deltaTime * 7 * 0.01f;

	if (r > 1) r = 0;
	if (g > 1) g = 0;
	if (b > 1) b = 0;

	D3DXCOLOR bg(r, g, b, 1);
	d3ddevcon->ClearRenderTargetView(renderTargetView, bg);
	d3ddevcon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);

	//DRAW World1 OBJECT
	WVP = World * View * Projection;
	cbPerObj.WVP = XMMatrixTranspose(WVP);
	cbPerObj.World = XMMatrixTranspose(World);
	d3ddevcon->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &cbPerObj, 0, 0);
	d3ddevcon->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);
	d3ddevcon->RSSetState(RS_WireFrame);
	d3ddevcon->PSSetShaderResources(0, 1, &CubesTexture);
	d3ddevcon->PSSetSamplers(0, 1, &CubesTextureSamplerState);
	d3ddevcon->DrawIndexed(36, 0, 0);
	//--end

	//DRAW World2 OBJECT
	WVP = World2 * View * Projection;
	cbPerObj.WVP = XMMatrixTranspose(WVP);
	cbPerObj.World = XMMatrixTranspose(World2);
	d3ddevcon->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &cbPerObj, 0, 0);
	d3ddevcon->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);
	d3ddevcon->RSSetState(RS_Solid);
	d3ddevcon->PSSetShaderResources(0, 1, &CubesTexture);
	d3ddevcon->PSSetSamplers(0, 1, &CubesTextureSamplerState);
	d3ddevcon->DrawIndexed(36, 0, 0);
	//--end

	//DRAW World3 OBJECT
	WVP = World3 * View * Projection;
	cbPerObj.WVP = XMMatrixTranspose(WVP);
	cbPerObj.World = XMMatrixTranspose(World3);
	d3ddevcon->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &cbPerObj, 0, 0);
	d3ddevcon->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);
	d3ddevcon->RSSetState(RS_Solid);
	d3ddevcon->PSSetShaderResources(0, 1, &CubesTexture);
	d3ddevcon->PSSetSamplers(0, 1, &CubesTextureSamplerState);
	d3ddevcon->DrawIndexed(36, 0, 0);
	//--end

	Swapchain->Present(0, 0);
}

void CleanUp()
{
	Swapchain->Release();
	device->Release();
	d3ddevcon->Release();
	renderTargetView->Release();

	vertexBuffer->Release();
	indexBuffer->Release();
	VS->Release();
	PS->Release();
	VS_Buffer->Release();
	PS_Buffer->Release();
	vertLayout->Release();

	RS_WireFrame->Release();
	RS_Solid->Release();

	depthStencilView->Release();
	depthStencilBuffer->Release();

	cbPerObjectBuffer->Release();
	cbPerFrameBuffer->Release();
}

bool InitWindow(HINSTANCE hInstance, int ShowWnd, int width, int height, bool windowed)
{
	// Erstmal einen leeren Container für das Fenster erstellen
	// Windows Class Extended
	WNDCLASSEX wc;

	// Kann auch Initialiserlist gesetzt werden
	// Hat keinen Geschwindigkeitsnachteil
	// So ist es besser lesbar
	wc.cbClsExtra = NULL;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.cbWndExtra = NULL;
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW + 2;
	// Cursor festlegen
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_SHIELD);
	wc.hIconSm = LoadIcon(NULL, IDI_SHIELD);
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = WndClassName;
	wc.lpszMenuName = NULL;
	// Zeichne das Bild horizontal und vertikal neu
	wc.style = CS_HREDRAW | CS_VREDRAW;

	// Regristriere das neue Fenster bei Windows
	// Als Adresse mithilfe des & Symbols
	if (!RegisterClassEx(&wc))
	{
		MessageBox(0, L"RegisterClassEx(&wc) failed", L"Einfach so Error", MB_OK);
		return 1;
	}

	// Erstelle das Fenster wenn Fenster registriert wurde
	hwnd = CreateWindowEx(
		NULL,
		WndClassName,
		L"Einfach nur ein Fenster",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	// Prüfe ob Fenster erstellt werden konnte
	if (!hwnd)
	{
		MessageBox(0, L"hwnd = CreateWindowEx() failed", L"Einfach so Error", MB_OK);
		return 1;
	}

	ShowWindow(hwnd, ShowWnd);
	UpdateWindow(hwnd);

	// Init DX

	// Lege Description für Buffer an
	DXGI_MODE_DESC bufferDESC;
	ZeroMemory(&bufferDESC, sizeof(DXGI_MODE_DESC));

	bufferDESC.Width = Width;
	bufferDESC.Height = Height;
	bufferDESC.RefreshRate.Numerator = 60;
	bufferDESC.RefreshRate.Denominator = 1;
	bufferDESC.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	bufferDESC.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	bufferDESC.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	DXGI_SWAP_CHAIN_DESC swapChainDESC;
	ZeroMemory(&swapChainDESC, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapChainDESC.BufferDesc = bufferDESC;
	swapChainDESC.SampleDesc.Count = 1; // Wählt Pixel aus und ordnet diese neu zu
	swapChainDESC.SampleDesc.Quality = 0;
	swapChainDESC.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDESC.BufferCount = 1;
	swapChainDESC.OutputWindow = hwnd;
	swapChainDESC.Windowed = windowed;
	swapChainDESC.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		NULL,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&swapChainDESC,
		&Swapchain,
		&device,
		NULL,
		&d3ddevcon
	);

	ID3D11Texture2D* Backbuffer;
	Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)& Backbuffer);

	device->CreateRenderTargetView(Backbuffer, NULL, &renderTargetView);
	Backbuffer->Release();

	//Depth Stencil View
	D3D11_TEXTURE2D_DESC depthStencilDESC;

	depthStencilDESC.Width = Width;
	depthStencilDESC.Height = Height;
	depthStencilDESC.MipLevels = 1;
	depthStencilDESC.ArraySize = 1;
	depthStencilDESC.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDESC.SampleDesc.Count = 1;
	depthStencilDESC.SampleDesc.Quality = 0;
	depthStencilDESC.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDESC.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDESC.CPUAccessFlags = 0;
	depthStencilDESC.MiscFlags = 0;

	device->CreateTexture2D(&depthStencilDESC, NULL, &depthStencilBuffer);
	device->CreateDepthStencilView(depthStencilBuffer, NULL, &depthStencilView);

	d3ddevcon->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

	return true;
}


int messageLoop()
{
	// Wenn keine Nachrichten, dann weiterloopen

	MSG msg;
	// Explizit das Objekt leeren
	// Es ist nicht sicher, dass beim Beenden msg nicht gelöscht wird
	ZeroMemory(&msg, sizeof(msg));

	static DWORD lastTime = timeGetTime();

	while (true) // Main Loop
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// Game Loop

			DWORD currTime = timeGetTime();
			float deltaTime = ((float)(currTime - lastTime)) * 0.0010000000000000f;
			DetectInput(deltaTime);
			Update(deltaTime);
			Draw(deltaTime);
			lastTime = currTime;
		}
		if (QUIT) break;
	}

	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		//case WM_KEYDOWN:
			//if (wparam == VK_ESCAPE) { DestroyWindow(hwnd); return 0; }
	case WM_DESTROY:
		PostQuitMessage(0); return 0;
	}


	return DefWindowProc(hwnd, msg, wparam, lparam);
}
