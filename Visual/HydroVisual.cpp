#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTsettingsdlg.h"
#include "SDKmisc.h"
#include "resource.h"
#include "MapReader.h"

#include <cstdio>
#include <ctime>
#include <string>
#include <fstream>
#include <vector>
#include <windows.h>

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CD3DSettingsDlg                 g_SettingsDlg;
CDXUTDialogResourceManager      g_DialogResourceManager;
CModelViewerCamera              g_Camera;

//Iterations*						iterations = NULL;
ID3DXEffect*                    g_pEffect = NULL;

CDXUTDialog						g_HUD;
CDXUTDialog                     g_SampleUI;

D3DXHANDLE                      g_HandleTechnique;
D3DXHANDLE                      g_HandleBoxInstance_Position;
D3DXHANDLE                      g_HandleBoxInstance_Color;
D3DXHANDLE                      g_HandleWorld, g_HandleView, g_HandleProjection;
D3DXHANDLE                      g_HandleTexture;

unsigned int					g_sizeX = 20;
unsigned int					g_sizeY = 20;
unsigned int					g_sizeZ = 20;
unsigned int					g_currentIteration = 0;
unsigned int					g_warstwa = 0;
unsigned int					g_axis = 3; // 0 - x, 1 - y, 2 - z

bool							g_globalNormalize = true;
bool							g_toTransform = true;
bool							g_endedTransform = true;
bool							g_fluxParam = false;
bool							g_firstRender = true;
bool							startSimulate = false;
bool                            g_bAppIsDebug = false;
bool                            g_bRuntimeDebug = false;
const int                       g_nMaxBoxes = 1000000;
const int                       g_nNumBatchInstance = 120;
int                             g_NumBoxes = 1000000;
double                          g_fLastTime = 0.0;
double                          g_fBurnAmount = 0.0;
int								g_ParameterToShow = 0;
clock_t							g_timeFrame = 0.0;

D3DXCOLOR						g_vBoxInstance_Color[g_nMaxBoxes];
D3DXVECTOR4						g_vBoxInstance_Position[g_nMaxBoxes];

IDirect3DTexture9*              g_pBoxTexture;

IDirect3DVertexBuffer9*         g_pVBInstanceData = 0;
IDirect3DVertexBuffer9*         g_pVBBox = 0;
IDirect3DIndexBuffer9*          g_pIBBox = 0;

//--------------------------------------------------------------------------------------
// Structes 
//--------------------------------------------------------------------------------------

struct BOX_VERTEX
{
	D3DXVECTOR3 pos;
	D3DXVECTOR3 norm;
	float u, v;
};

// Format dla shadera
struct BOX_VERTEX_INSTANCE
{
	D3DXVECTOR3 pos;
	D3DXVECTOR3 norm;
	float u, v;
	float boxInstance;
};

struct BOX_INSTANCEDATA_POS
{
	D3DCOLOR color;
	BYTE x;
	BYTE y;
	BYTE z;
	BYTE rotation;
};

#define IDC_RENDERMETHOD_LIST   1
#define IDC_STATIC              10

Iterations*						g_Iterations;

IDirect3DVertexDeclaration9*    g_pVertexDeclHardware = NULL;
D3DVERTEXELEMENT9 g_VertexElemHardware[] =
{
	{ 0, 0,     D3DDECLTYPE_FLOAT3,     D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_POSITION,  0 },
	{ 0, 3 * 4, D3DDECLTYPE_FLOAT3,     D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_NORMAL,    0 },
	{ 0, 6 * 4, D3DDECLTYPE_FLOAT2,     D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_TEXCOORD,  0 },
	{ 1, 0,     D3DDECLTYPE_D3DCOLOR,   D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_COLOR,     0 },
	{ 1, 4,     D3DDECLTYPE_D3DCOLOR,   D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_COLOR,     1 },
	D3DDECL_END()
};

IDirect3DVertexDeclaration9*    g_pVertexDeclShader = NULL;
D3DVERTEXELEMENT9 g_VertexElemShader[] =
{
	{ 0, 0,     D3DDECLTYPE_FLOAT3,     D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_POSITION,  0 },
	{ 0, 3 * 4, D3DDECLTYPE_FLOAT3,     D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_NORMAL,    0 },
	{ 0, 6 * 4, D3DDECLTYPE_FLOAT2,     D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_TEXCOORD,  0 },
	{ 0, 8 * 4, D3DDECLTYPE_FLOAT1,     D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_TEXCOORD,  1 },
	D3DDECL_END()
};

IDirect3DVertexDeclaration9*    g_pVertexDeclConstants = NULL;
D3DVERTEXELEMENT9 g_VertexElemConstants[] =
{
	{ 0, 0,     D3DDECLTYPE_FLOAT3,     D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_POSITION,  0 },
	{ 0, 3 * 4, D3DDECLTYPE_FLOAT3,     D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_NORMAL,    0 },
	{ 0, 6 * 4, D3DDECLTYPE_FLOAT2,     D3DDECLMETHOD_DEFAULT,  D3DDECLUSAGE_TEXCOORD,  0 },
	D3DDECL_END()
};

bool CALLBACK IsD3D9DeviceAcceptable(D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat,
	bool bWindowed, void* pUserContext);
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext);

bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
	assert(DXUT_D3D9_DEVICE == pDeviceSettings->ver);

	HRESULT hr;
	IDirect3D9* pD3D = DXUTGetD3D9Object();
	D3DCAPS9 caps;

	V(pD3D->GetDeviceCaps(pDeviceSettings->d3d9.AdapterOrdinal,
		pDeviceSettings->d3d9.DeviceType,
		&caps));

	pDeviceSettings->d3d9.pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	g_SettingsDlg.GetDialogControl()->GetComboBox(DXUTSETTINGSDLG_PRESENT_INTERVAL)->SetEnabled(false);

	if ((caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 ||
		caps.VertexShaderVersion < D3DVS_VERSION(2, 0))
	{
		pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}

#ifdef DEBUG_PS
	pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif
#ifdef DEBUG_VS
	if (pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF)
	{
		pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
		pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_MIXED_VERTEXPROCESSING;
		pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
		pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}
#endif

	if (pDeviceSettings->d3d9.pp.Windowed == FALSE)
		pDeviceSettings->d3d9.pp.SwapEffect = D3DSWAPEFFECT_FLIP;

	static bool s_bFirstTime = true;
	if (s_bFirstTime)
	{
		s_bFirstTime = false;
		if (pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF)
			DXUTDisplaySwitchingToREFWarning(pDeviceSettings->ver);
	}

	return true;
}


HRESULT CALLBACK OnD3D9CreateDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext);
HRESULT CALLBACK OnD3D9ResetDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext);
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext);
void CALLBACK OnD3D9FrameRender(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext);
void WorkaroudCamer();
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	bool* pbNoFurtherProcessing, void* pUserContext);
void CALLBACK OnD3D9LostDevice(void* pUserContext);
void CALLBACK OnD3D9DestroyDevice(void* pUserContext);

void InitApp();
HRESULT OnCreateBuffers(IDirect3DDevice9* pd3dDevice);
void OnRenderHWInstancing(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime);
void OnRenderShaderInstancing(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime);
//void OnRenderConstantsInstancing(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime);
//void OnRenderStreamInstancing(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime);

INT WINAPI wWinMain( HINSTANCE, HINSTANCE, LPWSTR argv, int )
{
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	char buffer[500];
	wcstombs(buffer, argv, 500);
	MapReader* mapReader = new MapReader(buffer, g_fluxParam, g_globalNormalize);
	g_Iterations = mapReader->GetIterations();

	g_sizeX = g_Iterations->sizeX;
	g_sizeY = g_Iterations->sizeY;
	g_sizeZ = g_Iterations->sizeZ;
	g_NumBoxes = g_sizeX*g_sizeY*g_sizeZ;

    DXUTSetCallbackD3D9DeviceAcceptable( IsD3D9DeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( OnD3D9CreateDevice );
    DXUTSetCallbackD3D9DeviceReset( OnD3D9ResetDevice );
    DXUTSetCallbackD3D9FrameRender( OnD3D9FrameRender );
    DXUTSetCallbackD3D9DeviceLost( OnD3D9LostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnD3D9DestroyDevice );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackFrameMove( OnFrameMove );

	InitApp();

    DXUTInit( true, true );
    DXUTSetHotkeyHandling( true, true, true );
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"Hydro" );
    DXUTCreateDevice( true, 1200, 800 );

#if defined(DEBUG) | defined(_DEBUG)
	g_bAppIsDebug = true;
#endif

	if (GetModuleHandleW(L"d3d9d.dll"))
		g_bRuntimeDebug = true;

    DXUTMainLoop();

    return DXUTGetExitCode();
}

void InitApp()
{
	g_SampleUI.Init(&g_DialogResourceManager);
	g_SettingsDlg.Init(&g_DialogResourceManager);
	g_HUD.Init(&g_DialogResourceManager);

	g_SampleUI.SetCallback(OnGUIEvent); int iY = 24;

	g_SampleUI.AddComboBox(IDC_RENDERMETHOD_LIST, 0, 0, 166, 22);
	g_SampleUI.GetComboBox(IDC_RENDERMETHOD_LIST)->SetDropHeight(12 * 4);
	g_SampleUI.GetComboBox(IDC_RENDERMETHOD_LIST)->AddItem(L"Intensity", NULL);
	g_SampleUI.GetComboBox(IDC_RENDERMETHOD_LIST)->AddItem(L"Flux", NULL);
	g_SampleUI.GetComboBox(IDC_RENDERMETHOD_LIST)->SetSelectedByIndex(g_ParameterToShow);

	g_SampleUI.AddStatic(IDC_STATIC, L"P - Next slice\nO - previous slice\nS - center slice\nI - Start/Stop simulation\nU - reset simulation\n1 - X axis\n2 - Y axis\n3 - Z axis\n4 - All axis", 0, 100, 200, 300);

	g_HUD.SetCallback(OnGUIEvent); iY = 10;

	g_warstwa = g_sizeZ / 2;

}

HRESULT CALLBACK OnD3D9CreateDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	HRESULT hr;

	V_RETURN(g_DialogResourceManager.OnD3D9CreateDevice(pd3dDevice));
	V_RETURN(g_SettingsDlg.OnD3D9CreateDevice(pd3dDevice));

	DWORD dwShaderFlags = D3DXFX_NOT_CLONEABLE;

#if defined( DEBUG ) || defined( _DEBUG )
	dwShaderFlags |= D3DXSHADER_DEBUG;
#endif

#ifdef DEBUG_VS
	dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
#endif
#ifdef DEBUG_PS
	dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
#endif
#ifdef D3DXFX_LARGEADDRESS_HANDLE
	dwShaderFlags |= D3DXFX_LARGEADDRESSAWARE;
#endif

	WCHAR str[MAX_PATH];
	V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"Instancing.fx"));

	V_RETURN(D3DXCreateEffectFromFile(pd3dDevice, str, NULL, NULL, dwShaderFlags,
		NULL, &g_pEffect, NULL));

	g_HandleWorld = g_pEffect->GetParameterBySemantic(NULL, "WORLD");
	g_HandleView = g_pEffect->GetParameterBySemantic(NULL, "VIEW");
	g_HandleProjection = g_pEffect->GetParameterBySemantic(NULL, "PROJECTION");
	g_HandleTexture = g_pEffect->GetParameterBySemantic(NULL, "TEXTURE");

	D3DXVECTOR3 vecEye(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 vecAt(0.0f, 0.0f, 0.0f);

	g_Camera.SetViewParams(&vecEye, &vecAt);

	V_RETURN(OnCreateBuffers(pd3dDevice));

	// Tektura do wymiany <-------------- !!!!!!!!!!!! tklempka
	hr = DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"env2.bmp");
	if (FAILED(hr))
		return DXTRACE_ERR(L"DXUTFindDXSDKMediaFileCch", hr);
	//-------------------------------------------------
	hr = D3DXCreateTextureFromFile(pd3dDevice, str, &g_pBoxTexture);
	if (FAILED(hr))
		return DXTRACE_ERR(L"D3DXCreateTextureFromFile", hr);

	return S_OK;
}

void CALLBACK OnD3D9FrameRender(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext)
{
	HRESULT hr;

	D3DXMATRIXA16 mWorld;
	D3DXMATRIXA16 mView;
	D3DXMATRIXA16 mProj;

	V(pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 45, 50, 170), 1.0f, 0));

	if (SUCCEEDED(pd3dDevice->BeginScene()))
	{
		if (g_firstRender)
		{
			WorkaroudCamer();
			g_firstRender = false;
		}
		else
		{
			mWorld = *g_Camera.GetWorldMatrix();
			mProj = *g_Camera.GetProjMatrix();
			mView = *g_Camera.GetViewMatrix();

			V(g_pEffect->SetMatrix(g_HandleWorld, &mWorld));
			V(g_pEffect->SetMatrix(g_HandleView, &mView));
			V(g_pEffect->SetMatrix(g_HandleProjection, &mProj));
		}
		OnRenderShaderInstancing(pd3dDevice, fTime, fElapsedTime);

		V(pd3dDevice->EndScene());
	}
}

void WorkaroudCamer()
{
	// Workaround for camera - tklempka
	/*
	D3DXMATRIXA16 mWorld;
	D3DXMATRIXA16 mView;
	D3DXMATRIXA16 mProj;

	mWorld._11 = -0.626782;
	mWorld._12 = -0.374161;
	mWorld._13 = -0.683481;
	mWorld._14 = 0;
	mWorld._21 = -0.01524;
	mWorld._22 = 0.88288;
	mWorld._23 = -0.46934;
	mWorld._24 = 0;
	mWorld._31 = 0.779044;
	mWorld._32 = -0.283;
	mWorld._33 = -0.55908;
	mWorld._34 = 0;
	mWorld._41 = 0;
	mWorld._42 = 0;
	mWorld._43 = 0;
	mWorld._44 = 1.0;

	mProj._11 = 1.609;
	mProj._12 = 0;
	mProj._13 = 0;
	mProj._14 = 0;
	mProj._21 = 0;
	mProj._22 = 2.41421;
	mProj._23 = 0;
	mProj._24 = 0;
	mProj._31 = 0;
	mProj._32 = 0;
	mProj._33 = 1.00010;
	mProj._34 = 1;
	mProj._41 = 0;
	mProj._42 = 0;
	mProj._43 = -0.10001;
	mProj._44 = 0;

	mView._11 = 0.9999;
	mView._12 = 0;
	mView._13 = 0;
	mView._14 = 0;
	mView._21 = 0;
	mView._22 = 0.9999;
	mView._23 = 0;
	mView._24 = 0;
	mView._31 = 0;
	mView._32 = 0;
	mView._33 = 1.0000;
	mView._34 = 0;
	mView._41 = 0;
	mView._42 = 0;
	mView._43 = 42.2275;
	mView._44 = 1.0;

	(g_pEffect->SetMatrix(g_HandleWorld, &mWorld));
	(g_pEffect->SetMatrix(g_HandleView, &mView));
	(g_pEffect->SetMatrix(g_HandleProjection, &mProj));
	*/
}

bool CALLBACK IsD3D9DeviceAcceptable(D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat,
	bool bWindowed, void* pUserContext)
{
	IDirect3D9* pD3D = DXUTGetD3D9Object();
	if (FAILED(pD3D->CheckDeviceFormat(pCaps->AdapterOrdinal, pCaps->DeviceType,
		AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
		D3DRTYPE_TEXTURE, BackBufferFormat)))
		return false;

	if (pCaps->PixelShaderVersion < D3DPS_VERSION(2, 0))
		return false;

	if (!(pCaps->DevCaps2 & D3DDEVCAPS2_STREAMOFFSET))
		return false;

	return true;
}

HRESULT CALLBACK OnD3D9ResetDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	HRESULT hr;

	V_RETURN(g_DialogResourceManager.OnD3D9ResetDevice());
	V_RETURN(g_SettingsDlg.OnD3D9ResetDevice());

	if (g_pEffect)
		V_RETURN(g_pEffect->OnResetDevice());
	
	g_HUD.SetLocation(pBackBufferSurfaceDesc->Width - 170, 0);
	g_HUD.SetSize(170, 170);
	g_SampleUI.SetLocation(0, 0);
	g_SampleUI.SetSize(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);


	float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
	g_Camera.SetProjParams(D3DX_PI/3, fAspectRatio, 0.1f, 1000.0f);
	g_Camera.SetWindow(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
	g_Camera.SetScalers(0.1f, 1.0f);
	//g_Camera.SetScalers();

	g_Camera.SetEnableYAxisMovement(true);
	g_Camera.SetEnablePositionMovement(true);
	
	return S_OK;
}

void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{     
	g_Camera.FrameMove(fElapsedTime);
}

void FillFace( BOX_VERTEX* pVerts, WORD* pIndices, int iFace,
	D3DXVECTOR3 vCenter, D3DXVECTOR3 vNormal, D3DXVECTOR3 vUp)
{
	D3DXVECTOR3 vRight;
	D3DXVec3Cross(&vRight, &vNormal, &vUp);

	pIndices[iFace * 6 + 0] = (WORD)(iFace * 4 + 0);
	pIndices[iFace * 6 + 1] = (WORD)(iFace * 4 + 1);
	pIndices[iFace * 6 + 2] = (WORD)(iFace * 4 + 2);
	pIndices[iFace * 6 + 3] = (WORD)(iFace * 4 + 3);
	pIndices[iFace * 6 + 4] = (WORD)(iFace * 4 + 2);
	pIndices[iFace * 6 + 5] = (WORD)(iFace * 4 + 1);

	pVerts[iFace * 4 + 0].pos = vCenter + vRight + vUp;
	pVerts[iFace * 4 + 1].pos = vCenter + vRight - vUp;
	pVerts[iFace * 4 + 2].pos = vCenter - vRight + vUp;
	pVerts[iFace * 4 + 3].pos = vCenter - vRight - vUp;

	for (int i = 0; i < 4; i++)
	{
		pVerts[iFace * 4 + i].u = (float)((i / 2) & 1) * 1.0f;
		pVerts[iFace * 4 + i].v = (float)((i) & 1) * 1.0f;
		pVerts[iFace * 4 + i].norm = vNormal;
	}
}

void FillFaceInstance( BOX_VERTEX_INSTANCE* pVerts, WORD* pIndices, int iFace,
	D3DXVECTOR3 vCenter, D3DXVECTOR3 vNormal, D3DXVECTOR3 vUp, WORD iInstanceIndex)
{
	D3DXVECTOR3 vRight;
	D3DXVec3Cross(&vRight, &vNormal, &vUp);

	WORD offsetIndex = iInstanceIndex * 6 * 2 * 3;  // offset index 
	WORD offsetVertex = iInstanceIndex * 4 * 6;     // offset vertex

	pIndices[offsetIndex + iFace * 6 + 0] = (WORD)(offsetVertex + iFace * 4 + 0);
	pIndices[offsetIndex + iFace * 6 + 1] = (WORD)(offsetVertex + iFace * 4 + 1);
	pIndices[offsetIndex + iFace * 6 + 2] = (WORD)(offsetVertex + iFace * 4 + 2);
	pIndices[offsetIndex + iFace * 6 + 3] = (WORD)(offsetVertex + iFace * 4 + 3);
	pIndices[offsetIndex + iFace * 6 + 4] = (WORD)(offsetVertex + iFace * 4 + 2);
	pIndices[offsetIndex + iFace * 6 + 5] = (WORD)(offsetVertex + iFace * 4 + 1);

	pVerts[offsetVertex + iFace * 4 + 0].pos = vCenter + vRight + vUp;
	pVerts[offsetVertex + iFace * 4 + 1].pos = vCenter + vRight - vUp;
	pVerts[offsetVertex + iFace * 4 + 2].pos = vCenter - vRight + vUp;
	pVerts[offsetVertex + iFace * 4 + 3].pos = vCenter - vRight - vUp;

	for (int i = 0; i < 4; i++)
	{
		pVerts[offsetVertex + iFace * 4 + i].boxInstance = (float)iInstanceIndex;
		pVerts[offsetVertex + iFace * 4 + i].u = (float)((i / 2) & 1) * 1.0f;
		pVerts[offsetVertex + iFace * 4 + i].v = (float)((i) & 1) * 1.0f;
		pVerts[offsetVertex + iFace * 4 + i].norm = vNormal;
	}
}

HRESULT OnCreateBuffers(IDirect3DDevice9* pd3dDevice)
{
	HRESULT hr;

	g_HandleTechnique = g_pEffect->GetTechniqueByName("TShader_Instancing");
	g_HandleBoxInstance_Position = g_pEffect->GetParameterBySemantic(NULL, "BOXINSTANCEARRAY_POSITION");
	g_HandleBoxInstance_Color = g_pEffect->GetParameterBySemantic(NULL, "BOXINSTANCEARRAY_COLOR");

	V_RETURN(pd3dDevice->CreateVertexDeclaration(g_VertexElemShader, &g_pVertexDeclShader));

	V_RETURN(pd3dDevice->CreateVertexBuffer(g_nNumBatchInstance * 4 * 6 * sizeof(BOX_VERTEX_INSTANCE), 0, 0,
		D3DPOOL_MANAGED, &g_pVBBox, 0));

	V_RETURN(pd3dDevice->CreateIndexBuffer(g_nNumBatchInstance * (6 * 2 * 3) * sizeof(WORD), 0,
		D3DFMT_INDEX16, D3DPOOL_MANAGED, &g_pIBBox, 0));

	BOX_VERTEX_INSTANCE* pVerts;

	hr = g_pVBBox->Lock(0, NULL, (void**)&pVerts, 0);

	if (SUCCEEDED(hr))
	{
		WORD* pIndices;
		hr = g_pIBBox->Lock(0, NULL, (void**)&pIndices, 0);

		if (SUCCEEDED(hr))
		{
			for (WORD iInstance = 0; iInstance < g_nNumBatchInstance; iInstance++)
			{
				FillFaceInstance(pVerts, pIndices, 0,
					D3DXVECTOR3(0.f, 0.f, -1.f),
					D3DXVECTOR3(0.f, 0.f, -1.f),
					D3DXVECTOR3(0.f, 1.f, 0.f),
					iInstance);

				FillFaceInstance(pVerts, pIndices, 1,
					D3DXVECTOR3(0.f, 0.f, 1.f),
					D3DXVECTOR3(0.f, 0.f, 1.f),
					D3DXVECTOR3(0.f, 1.f, 0.f),
					iInstance);

				FillFaceInstance(pVerts, pIndices, 2,
					D3DXVECTOR3(1.f, 0.f, 0.f),
					D3DXVECTOR3(1.f, 0.f, 0.f),
					D3DXVECTOR3(0.f, 1.f, 0.f),
					iInstance);

				FillFaceInstance(pVerts, pIndices, 3,
					D3DXVECTOR3(-1.f, 0.f, 0.f),
					D3DXVECTOR3(-1.f, 0.f, 0.f),
					D3DXVECTOR3(0.f, 1.f, 0.f),
					iInstance);

				FillFaceInstance(pVerts, pIndices, 4,
					D3DXVECTOR3(0.f, 1.f, 0.f),
					D3DXVECTOR3(0.f, 1.f, 0.f),
					D3DXVECTOR3(1.f, 0.f, 0.f),
					iInstance);

				FillFaceInstance(pVerts, pIndices, 5,
					D3DXVECTOR3(0.f, -1.f, 0.f),
					D3DXVECTOR3(0.f, -1.f, 0.f),
					D3DXVECTOR3(1.f, 0.f, 0.f),
					iInstance);
			}

			g_pIBBox->Unlock();
		}
		else
		{
			DXUT_ERR(L"Could not lock box model IB", hr);
		}
		g_pVBBox->Unlock();
	}
	else
	{
		DXUT_ERR(L"Could not lock box model VB", hr);
	}

	int index = 0;
	for (BYTE iZ = 0; iZ < g_sizeZ; iZ++)
		for (BYTE iY = 0; iY < g_sizeY; iY++)
			for (BYTE iX = 0; iX < g_sizeX; iX++)
			{
				if ((index > g_sizeX*g_sizeY*g_warstwa) && (index < g_sizeX*g_sizeY + g_sizeX * g_sizeY*g_warstwa)) {
					g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].x = iX * 16 / 255.0f; // (i%g_sizeX) * 20 / 255.0f;
					g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].y = iY * 16 / 255.0f; // (i / g_sizeX) * 20 / 255.0f;
					g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].z = iZ * 16 / 255.0f; // iY * 20 / 255.0f;
					g_vBoxInstance_Position[index].x += g_sizeX * 16 / 255.0f;
					g_vBoxInstance_Color[index].a = 1.0f;
				}
				else
				{
					//g_vBoxInstance_Color[index] = D3DCOLOR_ARGB(255, 255, 255, 255);
					g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].x = 100.0f; //iX * 16 / 255.0f; // (i%g_sizeX) * 20 / 255.0f;
					g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].y = 100.0f; // iY * 16 / 255.0f; // (i / g_sizeX) * 20 / 255.0f;
					g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].z = 100.0f; // iZ * 16 / 255.0f; // iY * 20 / 255.0f;
					g_vBoxInstance_Color[index].a = 1.0f;
				}
				index++;
				//g_vBoxInstance_Color[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX] = D3DCOLOR_ARGB(255, 255, 255, 255);
				//g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY*g_sizeY + iX].x = iX * 16 / 255.0f;// (i%g_sizeX) * 20 / 255.0f;
				//g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY*g_sizeY + iX].y = iY * 16 / 255.0f; // (i / g_sizeX) * 20 / 255.0f;
				//g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY*g_sizeY + iX].z = iZ * 16 / 255.0f; // iY * 20 / 255.0f;
			}

	//for (unsigned int i = 0; i < (g_sizeX*g_sizeY*g_sizeZ < g_NumBoxes ? g_sizeX*g_sizeY*g_sizeZ : g_NumBoxes); i++)
	//{
	//	g_vBoxInstance_Position[i].x = (i%g_sizeX) * 20 / 255.0f;
	//	g_vBoxInstance_Position[i].y = (i/g_sizeX) * 20 / 255.0f;
	//	g_vBoxInstance_Position[i].z = 0; // iY * 20 / 255.0f;
	//}

	/*
	int nRemainingBoxes = g_NumBoxes;
	for (BYTE iY = 0; iY < g_sizeY; iY++)
		for (BYTE iZ = 0; iZ < g_sizeZ; iZ++)
			for (BYTE iX = 0; iX < g_sizeX && nRemainingBoxes > 0; iX++, nRemainingBoxes--)
			{
				g_vBoxInstance_Color[g_NumBoxes - nRemainingBoxes] = D3DCOLOR_ARGB(255, 0, 0, 255);

				//g_vBoxInstance_Color[g_NumBoxes - nRemainingBoxes] = D3DCOLOR_ARGB( 0xff,
				//0x7f + 0x40 * ( ( g_NumBoxes - nRemainingBoxes + iX ) % 3 ),
				//0x7f + 0x40 * ( ( g_NumBoxes - nRemainingBoxes + iZ + 1 ) % 3 ),
				//0x7f + 0x40 * ( ( g_NumBoxes - nRemainingBoxes + iY + 2 ) % 3 ) );
				
				g_vBoxInstance_Position[g_NumBoxes - nRemainingBoxes].x = iX * 20 / 255.0f;
				g_vBoxInstance_Position[g_NumBoxes - nRemainingBoxes].z = iZ * 20 / 255.0f;
				g_vBoxInstance_Position[g_NumBoxes - nRemainingBoxes].y = iY * 20 / 255.0f;
				//g_vBoxInstance_Position[g_NumBoxes - nRemainingBoxes].w = ((WORD)iX + (WORD)iZ + (WORD)
					//iY) * 8 / 255.0f;
			}
	*/
	return S_OK;
}

void OnRenderHWInstancing(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime)
{
	HRESULT hr;
	UINT iPass, cPasses;

	V(pd3dDevice->SetVertexDeclaration(g_pVertexDeclHardware));

	V(pd3dDevice->SetStreamSource(0, g_pVBBox, 0, sizeof(BOX_VERTEX)));
	V(pd3dDevice->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | g_NumBoxes));

	V(pd3dDevice->SetStreamSource(1, g_pVBInstanceData, 0, sizeof(BOX_INSTANCEDATA_POS)));
	V(pd3dDevice->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1ul));

	V(pd3dDevice->SetIndices(g_pIBBox));

	V(g_pEffect->SetTechnique(g_HandleTechnique));

	V(g_pEffect->Begin(&cPasses, 0));
	for (iPass = 0; iPass < cPasses; iPass++)
	{
		V(g_pEffect->BeginPass(iPass));
		V(g_pEffect->SetTexture(g_HandleTexture, g_pBoxTexture));
		V(g_pEffect->CommitChanges());

		V(pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4 * 6, 0, 6 * 2));

		V(g_pEffect->EndPass());
	}
	V(g_pEffect->End());

	V(pd3dDevice->SetStreamSourceFreq(0, 1));
	V(pd3dDevice->SetStreamSourceFreq(1, 1));
}

void OnRenderShaderInstancing(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime)
{
	HRESULT hr;
	UINT iPass, cPasses;

	V(pd3dDevice->SetVertexDeclaration(g_pVertexDeclShader));

	V(pd3dDevice->SetStreamSource(0, g_pVBBox, 0, sizeof(BOX_VERTEX_INSTANCE)));
	V(pd3dDevice->SetIndices(g_pIBBox));

	V(g_pEffect->SetTechnique(g_HandleTechnique));

	V(g_pEffect->Begin(&cPasses, 0));

	g_timeFrame += clock();
	if ((g_timeFrame / (double)CLOCKS_PER_SEC) > 400)
	{
		float tmp = -1;
		float maxTmp = -1;
		float minTmp = -1;

		if (g_ParameterToShow == 0)
		{
			if (!g_globalNormalize)
			{
				maxTmp = g_Iterations->iteration[g_currentIteration].maxIntensity;
				minTmp = g_Iterations->iteration[g_currentIteration].minIntensity;
			}
			else
			{
				maxTmp = g_Iterations->maxIntensity;
				minTmp = g_Iterations->minIntensity;
			}
			tmp = 255.0f / (maxTmp - minTmp);
			if (maxTmp - minTmp == 0.0f)
				tmp = 255.0f;
		}
		else if (g_ParameterToShow == 1)
		{
			if (!g_globalNormalize)
			{
				maxTmp = g_Iterations->iteration[g_currentIteration].maxFlux;
				minTmp = g_Iterations->iteration[g_currentIteration].minFlux;
			}
			else
			{
				maxTmp = g_Iterations->maxFlux;
				minTmp = g_Iterations->minFlux;
			}
			tmp = 255.0f / (maxTmp - minTmp);
			if (maxTmp - minTmp == 0.0f)
				tmp = 255.0f;
		}

		for (unsigned int i = 0; i < (g_sizeX*g_sizeY*g_sizeZ < g_NumBoxes ? g_sizeX * g_sizeY*g_sizeZ : g_NumBoxes); i++)
		{
			if (g_ParameterToShow == 0)
			{
				g_vBoxInstance_Color[i].g = 0.0f;
				g_vBoxInstance_Color[i].b = 1.0f - (((g_Iterations->iteration[g_currentIteration].point[i].intensity - minTmp)*tmp) / 255.0f);
				g_vBoxInstance_Color[i].r = (g_Iterations->iteration[g_currentIteration].point[i].intensity - minTmp)*tmp / 255.0f;
			}
			else if (g_ParameterToShow == 1)
			{
				g_vBoxInstance_Color[i].g = 0.0f;
				g_vBoxInstance_Color[i].b = 1.0f - (((g_Iterations->iteration[g_currentIteration].point[i].flux - minTmp)*tmp) / 255.0f);
				g_vBoxInstance_Color[i].r = (g_Iterations->iteration[g_currentIteration].point[i].flux - minTmp)*tmp / 255.0f;
			}
		}
		if (g_currentIteration < g_Iterations->IterationNum - 1 && startSimulate)
			g_currentIteration++;
		g_timeFrame = 0.0;
	}

	if (g_warstwa != -1 && g_toTransform)
	{
		DWORD index = 0;
		for (BYTE iZ = 0; iZ < g_sizeZ; iZ++)
			for (BYTE iY = 0; iY < g_sizeY; iY++)
				for (BYTE iX = 0; iX < g_sizeX; iX++)
				{
					if (g_axis == 0)
					{
						if ((index > g_sizeX*g_sizeY*g_warstwa) && (index < g_sizeX*g_sizeY + g_sizeX * g_sizeY*g_warstwa)) {
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].x = iX * 16 / 255.0f; // (i%g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].y = iY * 16 / 255.0f; // (i / g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].z = iZ * 16 / 255.0f; // iY * 20 / 255.0f;
							//g_vBoxInstance_Position[index].x -= g_sizeX * 16 / 255.0f;
							g_vBoxInstance_Color[index].a = 1.0f;
						}
						else
						{
							//g_vBoxInstance_Color[index] = D3DCOLOR_ARGB(255, 255, 255, 255);
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].x = 100.0f; //iX * 16 / 255.0f; // (i%g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].y = 100.0f; // iY * 16 / 255.0f; // (i / g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].z = 100.0f; // iZ * 16 / 255.0f; // iY * 20 / 255.0f;
							g_vBoxInstance_Color[index].a = 0.0f;
						}
					}
					else if (g_axis == 1)
					{
						if (iY == g_warstwa)
						{
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].x = iX * 16 / 255.0f; // (i%g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].y = iY * 16 / 255.0f; // (i / g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].z = iZ * 16 / 255.0f; // iY * 20 / 255.0f;
							//g_vBoxInstance_Position[index].x -= g_sizeX * 16 / 255.0f;
							//g_vBoxInstance_Position[index].y += (g_sizeX * 16 / 255.0f);
							g_vBoxInstance_Color[index].a = 1.0f;
						}
						else
						{
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].x = 100.0f; //iX * 16 / 255.0f; // (i%g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].y = 100.0f; // iY * 16 / 255.0f; // (i / g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].z = 100.0f; // iZ * 16 / 255.0f; // iY * 20 / 255.0f;
							g_vBoxInstance_Color[index].a = 0.0f;
						}
					}
					else if (g_axis == 2)
					{
						if (iX == g_warstwa)
						{
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].x = iX * 16 / 255.0f; // (i%g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].y = iY * 16 / 255.0f; // (i / g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].z = iZ * 16 / 255.0f; // iY * 20 / 255.0f;
							g_vBoxInstance_Color[index].a = 1.0f;
						}
						else
						{
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].x = 100.0f; //iX * 16 / 255.0f; // (i%g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].y = 100.0f; // iY * 16 / 255.0f; // (i / g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].z = 100.0f; // iZ * 16 / 255.0f; // iY * 20 / 255.0f;
							g_vBoxInstance_Color[index].a = 0.0f;
						}
					}
					else if (g_axis == 3)
					{
						if (iY == g_warstwa || iX == g_warstwa || (index > g_sizeX*g_sizeY*g_warstwa) && (index < g_sizeX*g_sizeY + g_sizeX * g_sizeY*g_warstwa))
						{
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].x = iX * 16 / 255.0f; // (i%g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].y = iY * 16 / 255.0f; // (i / g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].z = iZ * 16 / 255.0f; // iY * 20 / 255.0f;
							g_vBoxInstance_Color[index].a = 0.8f;
						}
						else
						{
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].x = 100.0f; //iX * 16 / 255.0f; // (i%g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].y = 100.0f; // iY * 16 / 255.0f; // (i / g_sizeX) * 20 / 255.0f;
							g_vBoxInstance_Position[iZ*(g_sizeX*g_sizeY) + iY * g_sizeY + iX].z = 100.0f; // iZ * 16 / 255.0f; // iY * 20 / 255.0f;
							g_vBoxInstance_Color[index].a = 0.0f;
						}
					}
					index++;
				}

		g_toTransform = false;
		g_endedTransform = true;
	}
	
	for (iPass = 0; iPass < cPasses; iPass++)
	{
		V(g_pEffect->BeginPass(iPass));

		V(g_pEffect->SetTexture(g_HandleTexture, g_pBoxTexture));

		int nRemainingBoxes = g_NumBoxes;
		while (nRemainingBoxes > 0)
		{         
			int nRenderBoxes = min(nRemainingBoxes, g_nNumBatchInstance);

			V(g_pEffect->SetVectorArray(g_HandleBoxInstance_Position, g_vBoxInstance_Position + g_NumBoxes - nRemainingBoxes, nRenderBoxes));
			V(g_pEffect->SetVectorArray(g_HandleBoxInstance_Color, (D3DXVECTOR4*)
				(g_vBoxInstance_Color + g_NumBoxes - nRemainingBoxes), nRenderBoxes));

			V(g_pEffect->CommitChanges());

			V(pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, nRenderBoxes * 4 * 6, 0,
				nRenderBoxes * 6 * 2));
			nRemainingBoxes -= nRenderBoxes;
		}

		V(g_pEffect->EndPass());
	}

	V(g_HUD.OnRender(fElapsedTime));
	V(g_SampleUI.OnRender(fElapsedTime));

	V(g_pEffect->End());
}

void OnRenderConstantsInstancing(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime)
{
	HRESULT hr;
	UINT iPass, cPasses;

	V(pd3dDevice->SetVertexDeclaration(g_pVertexDeclConstants));

	V(pd3dDevice->SetStreamSource(0, g_pVBBox, 0, sizeof(BOX_VERTEX)));
	V(pd3dDevice->SetIndices(g_pIBBox));

	V(g_pEffect->SetTechnique(g_HandleTechnique));

	V(g_pEffect->Begin(&cPasses, 0));
	for (iPass = 0; iPass < cPasses; iPass++)
	{
		V(g_pEffect->BeginPass(iPass));

		V(g_pEffect->SetTexture(g_HandleTexture, g_pBoxTexture));

		for (int nRemainingBoxes = 0; nRemainingBoxes < g_NumBoxes; nRemainingBoxes++)
		{
			V(g_pEffect->SetVector(g_HandleBoxInstance_Position, &g_vBoxInstance_Position[nRemainingBoxes]));
			V(g_pEffect->SetVector(g_HandleBoxInstance_Color,
				(D3DXVECTOR4*)&g_vBoxInstance_Color[nRemainingBoxes]));

			V(g_pEffect->CommitChanges());

			V(pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4 * 6, 0, 6 * 2));
		}

		V(g_pEffect->EndPass());
	}
	V(g_pEffect->End());
}

void OnRenderStreamInstancing(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime)
{
	HRESULT hr;
	UINT iPass, cPasses;

	V(pd3dDevice->SetVertexDeclaration(g_pVertexDeclHardware));

	V(pd3dDevice->SetStreamSource(0, g_pVBBox, 0, sizeof(BOX_VERTEX)));

	V(pd3dDevice->SetIndices(g_pIBBox));

	V(g_pEffect->SetTechnique(g_HandleTechnique));

	V(g_pEffect->Begin(&cPasses, 0));
	for (iPass = 0; iPass < cPasses; iPass++)
	{
		V(g_pEffect->BeginPass(iPass));

		V(g_pEffect->SetTexture(g_HandleTexture, g_pBoxTexture));

		V(g_pEffect->CommitChanges());

		for (int nRemainingBoxes = 0; nRemainingBoxes < g_NumBoxes; nRemainingBoxes++)
		{
			V(pd3dDevice->SetStreamSource(1, g_pVBInstanceData, nRemainingBoxes * sizeof(BOX_INSTANCEDATA_POS),
				0));

			V(pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4 * 6, 0, 6 * 2));
		}

		V(g_pEffect->EndPass());
	}
	V(g_pEffect->End());
}

void OnDestroyBuffers()
{
	SAFE_RELEASE(g_pVBBox);
	SAFE_RELEASE(g_pIBBox);
	SAFE_RELEASE(g_pVBInstanceData);
	SAFE_RELEASE(g_pVertexDeclHardware);
	SAFE_RELEASE(g_pVertexDeclShader);
	SAFE_RELEASE(g_pVertexDeclConstants);
}

void CALLBACK OnD3D9LostDevice(void* pUserContext)
{
	g_DialogResourceManager.OnD3D9LostDevice();
	g_SettingsDlg.OnD3D9LostDevice();
	if (g_pEffect)
		g_pEffect->OnLostDevice();
}

void CALLBACK OnD3D9DestroyDevice(void* pUserContext)
{
	g_DialogResourceManager.OnD3D9DestroyDevice();
	g_SettingsDlg.OnD3D9DestroyDevice();
	SAFE_RELEASE(g_pEffect);
	SAFE_RELEASE(g_pBoxTexture);

	OnDestroyBuffers();
}

LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	bool* pbNoFurtherProcessing, void* pUserContext)
{
	*pbNoFurtherProcessing = g_DialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;
	if (g_SettingsDlg.IsActive())
	{
		g_SettingsDlg.MsgProc(hWnd, uMsg, wParam, lParam);
		return 0;
	}

	if (uMsg == WM_KEYDOWN)
	{
		if (wParam == 85) // U key
		{
			g_currentIteration = 0;
		}
		else if (wParam == 73) // I key
		{
			startSimulate = !startSimulate;
		}
		else if (wParam == 83) // S key
		{
			if (!g_toTransform)
			{
				g_toTransform = true;
				g_warstwa = g_sizeZ / 2;
			}
		}
		//else if (wParam == 82) // R key
		//{
		//	g_warstwa = -1;
		//}
		else if (wParam == 80) // P key
		{
			if ((g_warstwa  < g_sizeX-1 || g_warstwa == -1) && !g_toTransform)
			{
				g_warstwa++;
				g_toTransform = true;
			}
		}
		else if (wParam == 79) // O key
		{
			if (g_warstwa > 0 && !g_toTransform)
			{
				g_warstwa--;
				g_toTransform = true;
			}
		}
		else if (wParam == 49) // 1 num
		{
			g_axis = 0;
			g_toTransform = true;
		}
		else if (wParam == 50) // 2 num
		{
			g_axis = 1;
			g_toTransform = true;
		}
		else if (wParam == 51) // 3 num
		{
			g_axis = 2;
			g_toTransform = true;
		}
		else if (wParam == 52) // 4 num
		{
			g_axis = 3;
			g_toTransform = true;
		}
	}

	*pbNoFurtherProcessing = g_SampleUI.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);

	return 0;
}

void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{
	WCHAR szMessage[100];
	switch (nControlID)
	{
		case IDC_RENDERMETHOD_LIST:
		{
			DXUTComboBoxItem* pCBItem = g_SampleUI.GetComboBox(IDC_RENDERMETHOD_LIST)->GetSelectedItem();
			g_ParameterToShow = wcscmp(pCBItem->strText, L"Intensity") == 0 ? 0 :
				wcscmp(pCBItem->strText, L"Flux") == 0 ? 1 : -1;

			OnDestroyBuffers();
			OnCreateBuffers(DXUTGetD3D9Device());
			break;
		}
	}
}