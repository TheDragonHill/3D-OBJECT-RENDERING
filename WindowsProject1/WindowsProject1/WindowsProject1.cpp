#include <Windows.h>
#include <d3dx9.h>
#include <d3d9.h>
#include <timeapi.h>

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment (lib, "winmm.lib")


#define KUCHEN 3.141

// ---------- win32 Stuff
LPCTSTR WndClassName = L"DirektSaftFenster";
HWND hwnd = 0;
const int Width = 1280;
const int Height = 720;

//--------DX STuff
IDirect3D9* d3d9 = NULL;
IDirect3DDevice9* d3ddev = NULL;

IDirect3DVertexBuffer9* vertexBuffer = 0;
IDirect3DIndexBuffer9* indexBuffer = 0;

IDirect3DTexture9* cubetex = 0;

D3DMATERIAL9 white;
D3DLIGHT9 light;

struct Vertex
{
	float x, y, z;    // pos
	float nx, ny, nz; // normals
	float u, v;       // textur koordinaten
};

const DWORD VertexFVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;


// immer wenn ich dich angzcke, bekomm ich auch direkt ne nullpointer exception

//----------prototypes

bool SetupScene();
bool Draw(float deltaTime);
void CleanUp();
bool InitWindow(HINSTANCE hInstance, int ShowWnd,
	int width, int height, bool windowed,
	D3DDEVTYPE deviceType, IDirect3DDevice9** d3dDevice);
int messageloop();
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
	WPARAM wparam, LPARAM lparam);


//---HIER GEHTS LOOOOOOOooOOOoos

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE prevHinstance, LPSTR lpCmdLine, int nShowCmd)
{

	if (!InitWindow(hInstance, nShowCmd, Width, Height, true, D3DDEVTYPE_HAL, &d3ddev))
	{ //wenn init abkackt
		MessageBox(0, L"Init window failed", L"Artist Error", MB_OK);
		return 1;
	}

	if (!SetupScene())
	{ //wenn abkackt
		MessageBox(0, L"SceneSetup failed", L"Artist Error", MB_OK);
		return 1;
	}

	messageloop();
	CleanUp();
	return 0;
}

bool SetupScene()
{
	D3DXVECTOR3 pos(0, 3, -7);  // CAMERA POS
	D3DXVECTOR3 targ(0, 0, 0); // CAMERA Target
	D3DXVECTOR3 up(0, 1, 0);  // CAMERA UP

	D3DXMATRIX View;
	D3DXMatrixLookAtLH(&View, &pos, &targ, &up);

	d3ddev->SetTransform(D3DTS_VIEW, &View);

	D3DXMATRIX Proj;

	D3DXMatrixPerspectiveFovLH(
		&Proj,
		KUCHEN * 0.5f, // FOV Y
		((float)Width / (float)Height), // aspect ratio
		1.0f, //near plane
		1000.0f);  //far plane

	d3ddev->SetTransform(D3DTS_PROJECTION, &Proj);

	//---------- VERTEX BUFFER

	Vertex vertices[] =
	{

		//front face
		{ 1.0f,-1.0f,-1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f},
		{-1.0f,-1.0f,-1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f},
		{-1.0f, 1.0f,-1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f},

		{ 1.0f,-1.0f,-1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f},
		{-1.0f, 1.0f,-1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f},
		{ 1.0f, 1.0f,-1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f},

		//right side
		{ 1.0f,-1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f},
		{ 1.0f,-1.0f,-1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f},
		{ 1.0f, 1.0f,-1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},

		{ 1.0f,-1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f},
		{ 1.0f, 1.0f,-1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f},

		//left side
		{-1.0f,-1.0f,-1.0f,-1.0f, 0.0f, 0.0f, 1.0f, 1.0f},
		{-1.0f,-1.0f, 1.0f,-1.0f, 0.0f, 0.0f, 0.0f, 1.0f},
		{-1.0f, 1.0f, 1.0f,-1.0f, 0.0f, 0.0f, 0.0f, 0.0f},

		{-1.0f,-1.0f,-1.0f,-1.0f, 0.0f, 0.0f, 1.0f, 1.0f},
		{-1.0f, 1.0f, 1.0f,-1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
		{-1.0f, 1.0f,-1.0f,-1.0f, 0.0f, 0.0f, 1.0f, 0.0f},

		//back side
		{-1.0f,-1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f},
		{ 1.0f,-1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f},
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},

		{-1.0f,-1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f},
		{ 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
		{-1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f},

		//top side
		{ 1.0f, 1.0f,-1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f},
		{-1.0f, 1.0f,-1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},
		{-1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},

		{ 1.0f, 1.0f,-1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f},
		{-1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
		{ 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f},

		//bottom side
		{ 1.0f,-1.0f, 1.0f, 0.0f,-1.0f, 0.0f, 1.0f, 1.0f},
		{-1.0f,-1.0f, 1.0f, 0.0f,-1.0f, 0.0f, 0.0f, 1.0f},
		{-1.0f,-1.0f,-1.0f, 0.0f,-1.0f, 0.0f, 0.0f, 0.0f},

		{ 1.0f,-1.0f, 1.0f, 0.0f,-1.0f, 0.0f, 1.0f, 1.0f},
		{-1.0f,-1.0f,-1.0f, 0.0f,-1.0f, 0.0f, 0.0f, 0.0f},
		{ 1.0f,-1.0f,-1.0f, 0.0f,-1.0f, 0.0f, 1.0f, 0.0f},
	};

	d3ddev->CreateVertexBuffer(36 * sizeof(Vertex), 0, VertexFVF,
		D3DPOOL_MANAGED, &vertexBuffer, NULL);

	VOID* pVoid;
	vertexBuffer->Lock(0, 0, (void**)& pVoid, 0);
	memcpy(pVoid, vertices, sizeof(vertices));
	vertexBuffer->Unlock();

	// -------- INDEX BUFFER

	short indices[] =
	{
		0, 1, 2,    // side 1
		2, 1, 3,
		4, 0, 6,    // side 2
		6, 0, 2,
		7, 5, 6,    // side 3
		6, 5, 4,
		3, 1, 7,    // side 4
		7, 1, 5,
		4, 5, 0,    // side 5
		0, 5, 1,
		3, 7, 2,    // side 6
		2, 7, 6,
	};

	d3ddev->CreateIndexBuffer(36 * sizeof(short), 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &indexBuffer, NULL);

	indexBuffer->Lock(0, 0, (void**)& pVoid, 0);
	memcpy(pVoid, indices, sizeof(indices));
	indexBuffer->Unlock();

	{
		//d3ddev->CreateVertexBuffer(
		//  4 * sizeof(Vertex), D3DUSAGE_WRITEONLY,
		//  VertexFVF, D3DPOOL_MANAGED, &vertexBuffer, 0);

		//Vertex* vertices;

		//vertexBuffer->Lock(0, 0, (void**)& vertices, 0);

		//vertices[0].x = -0.5f;
		//vertices[0].y =  -0.5f;
		//vertices[0].z =  0.0f;
		//vertices[0].color = D3DCOLOR_XRGB(255, 0, 0);

		//vertices[1].x = -0.5f;
		//vertices[1].y = 0.5f;
		//vertices[1].z = 0.0f;
		//vertices[1].color = D3DCOLOR_XRGB(0, 255, 0);

		//vertices[2].x = +0.5f;
		//vertices[2].y = -0.5f;
		//vertices[2].z = 0.0f;
		//vertices[2].color = D3DCOLOR_XRGB(0, 0, 255);

		//vertices[3].x = +0.5f;
		//vertices[3].y = +0.5f;
		//vertices[3].z = 0.0f;
		//vertices[3].color = D3DCOLOR_XRGB(0, 255, 255);

		//vertexBuffer->Unlock();

		//// ----------- index Buffer

		//WORD* indices;

		//d3ddev->CreateIndexBuffer(
		//  6 * sizeof(WORD),
		//  D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED,
		//  &indexBuffer, 0);

		//indexBuffer->Lock(0, 0, (void**)& indices, 0);

		//indices[0] = 0; indices[1] = 1; indices[2] = 2;
		//indices[3] = 1; indices[4] = 3; indices[5] = 2;

		//indexBuffer->Unlock();

	}

	//TEXTURE

	D3DXCreateTextureFromFile(d3ddev, L"cube.jpg", &cubetex);
	d3ddev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	d3ddev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	d3ddev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);


	// LIGHT
	ZeroMemory(&light, sizeof(light));
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Diffuse = D3DXCOLOR(1, 1, 1, 1);
	light.Ambient = D3DXCOLOR(1, 1, 1, 1) * 0.1f;
	light.Specular = D3DXCOLOR(1, 1, 1, 1) * 0.05f;
	light.Direction = D3DXVECTOR3(1, -0.4f, 0.2f);

	//Material
	ZeroMemory(&white, sizeof(white));
	white.Diffuse = D3DXCOLOR(1, 1, 1, 1);
	white.Ambient = D3DXCOLOR(1, 1, 1, 1);
	white.Specular = D3DXCOLOR(1, 1, 1, 1);
	white.Emissive = D3DXCOLOR(0, 0, 0, 1);
	white.Power = 4;

	d3ddev->SetLight(0, &light);
	d3ddev->LightEnable(0, true);

	// Rasterizer States
//d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	d3ddev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	d3ddev->SetRenderState(D3DRS_LIGHTING, true);
	d3ddev->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	d3ddev->SetRenderState(D3DRS_NORMALIZENORMALS, true);

	return true;
}

bool Draw(float deltaTime)
{
	if (d3ddev)
	{
		d3ddev->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
			0x003866FF, 1.0f, 0);

		D3DXMATRIX RotY;
		D3DXMATRIX RotX;
		D3DXMATRIX RotZ;
		D3DXMATRIX T;
		D3DXMATRIX S;
		D3DXMATRIX WORLD;
		D3DXMATRIX WORLD2;

		static float rY = 0.0f;
		static float rX = 0.0f;
		static float rZ = 0.0f;
		static float s = 1.0f;
		static float timer = 0;

		D3DXMatrixRotationY(&RotY, rY);
		D3DXMatrixRotationX(&RotX, rX);
		D3DXMatrixRotationZ(&RotZ, rZ);
		D3DXMatrixTranslation(&T, -4, 0, 0);
		D3DXMatrixScaling(&S, s, s, s);

		timer += deltaTime;
		s = 1.0f + sinf(timer * KUCHEN * 2) * 0.5f;

		WORLD = S * RotX;
		WORLD2 = RotZ * T * RotY;

		rY += 3 * deltaTime;
		rX += 3 * deltaTime;
		rZ += 3 * deltaTime;
		/*
				if (GetAsyncKeyState(VK_RIGHT) & 0x8000f)
				{           rY += 3 * deltaTime;        }
				if (GetAsyncKeyState(VK_UP) & 0x8000f)
				{           rX += 3 * deltaTime;        }
				if (GetAsyncKeyState(VK_LEFT) & 0x8000f)
				{           rY -= 3 * deltaTime;        }
				if (GetAsyncKeyState(VK_DOWN) & 0x8000f)
				{           rX -= 3 * deltaTime;        }
		*/

		d3ddev->SetTexture(0, cubetex);
		d3ddev->SetMaterial(&white);

		d3ddev->BeginScene();
		d3ddev->SetStreamSource(0, vertexBuffer, 0, sizeof(Vertex));
		d3ddev->SetFVF(VertexFVF);
		d3ddev->SetIndices(indexBuffer);
		d3ddev->SetTransform(D3DTS_WORLD, &WORLD);
		//d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12);
		d3ddev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 12);
		d3ddev->SetTransform(D3DTS_WORLD, &WORLD2);
		//d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12);
		d3ddev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 12);
		d3ddev->EndScene();

		d3ddev->Present(0, 0, 0, 0);

	}
	return true;
}

void CleanUp()
{
	d3ddev->Release();
	d3d9->Release();
	vertexBuffer->Release();
	indexBuffer->Release();
	cubetex->Release();
}

bool InitWindow(HINSTANCE hInstance, int ShowWnd, int width,
	int height, bool windowed, D3DDEVTYPE deviceType, IDirect3DDevice9** d3dDevice)
{
	WNDCLASSEX wc;

	wc.cbClsExtra = NULL;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.cbWndExtra = NULL;
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW + 2;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_HAND);
	wc.hIconSm = LoadIcon(NULL, IDI_HAND);
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = WndClassName;
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&wc))
	{
		MessageBox(0, L"Registrering win failed", L"Artist Error", MB_OK);
		return 1;
	}

	hwnd = CreateWindowEx(
		NULL, WndClassName, L"DirektSaft9", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL,
		hInstance, NULL);

	if (!hwnd)
	{
		MessageBox(0, L"Creating win failed", L"Artist Error", MB_OK);
		return 1;
	}

	ShowWindow(hwnd, ShowWnd);
	UpdateWindow(hwnd);

	// --------------------INIT DX

	d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d9)
	{
		MessageBox(0, L"Creating d3d failed", L"Artist Error", MB_OK);
		return 1;
	}

	D3DCAPS9 caps;
	d3d9->GetDeviceCaps(D3DADAPTER_DEFAULT, deviceType, &caps);
	int vertexproc = NULL;

	if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
		vertexproc = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		vertexproc = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	D3DPRESENT_PARAMETERS d3dpp;

	d3dpp.BackBufferWidth = width;
	d3dpp.BackBufferHeight = height;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality = NULL;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hwnd;
	d3dpp.Windowed = windowed;
	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.Flags = NULL;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	HRESULT hr = 0;

	hr = d3d9->CreateDevice(D3DADAPTER_DEFAULT, deviceType, hwnd,
		vertexproc, &d3dpp, d3dDevice);

	if (FAILED(hr))
	{
		MessageBox(0, L"Creating d3DEVICE failed", L"Artist Error", MB_OK);
		d3d9->Release();
		return false;
	}

	return true;
}

int messageloop()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	static DWORD lastTime = timeGetTime();

	while (true) // MAIN LOOP
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// GAME LOOP

			DWORD currTime = timeGetTime();
			float time = ((float)(currTime - lastTime)) * 0.001f;
			Draw(time);
			lastTime = currTime;

		}

	}

	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		//case WM_KEYDOWN:
		//  if (wparam == VK_ESCAPE) { DestroyWindow(hwnd); return 0; }
	case WM_DESTROY:
		PostQuitMessage(0); return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}