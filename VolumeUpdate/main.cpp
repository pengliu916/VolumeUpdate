#include "DXUT.h"
#include "DXUTcamera.h"
#include "SDKmisc.h"
#include <DirectXMath.h>

#include "header.h"
#include "Volume_UAV.h"

using namespace DirectX;
//--------------------------------------------------------------------------------------
//Structures
//--------------------------------------------------------------------------------------
struct CBChangesEveryFrame
{
	XMMATRIX mWorldViewProjection;
	XMFLOAT4 mViewPos;
};
//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CModelViewerCamera              g_Camera;      
XMMATRIX						g_mWorldViewProjection;

D3D11_VIEWPORT					g_mViewport;

ID3D11VertexShader*				g_pVertexShader = NULL;
ID3D11PixelShader*				g_pPixelShader = NULL;
ID3D11GeometryShader*			g_pGeometryShader = NULL;

ID3D11SamplerState*             g_pSamplerLinear;

ID3D11InputLayout*				g_pVertexLayout = NULL;
ID3D11Buffer*					g_pVertexBuffer	= NULL;

ID3D11Buffer*					g_pCBChangesEveryFrame;
CBChangesEveryFrame				g_cbPerFrame;

Volume_UAV						volume(VOLUME_SIZE,VOLUME_SIZE,VOLUME_SIZE);

bool							g_bRender = true;

//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
									  DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
	return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
	return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
									 void* pUserContext )
{
	HRESULT hr=S_OK;

	ID3DBlob* pVSBlob = NULL;
	V_RETURN(DXUTCompileFromFile(L"raymarch.fx", nullptr, "VS", "vs_5_0", COMPILE_FLAG, 0, &pVSBlob));
	V_RETURN(pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(),pVSBlob->GetBufferSize(),NULL,&g_pVertexShader));

	ID3DBlob* pPSBlob = NULL;
	V_RETURN(DXUTCompileFromFile(L"raymarch.fx", nullptr, "PS", "ps_5_0", COMPILE_FLAG, 0, &pPSBlob));
	V_RETURN(pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(),pPSBlob->GetBufferSize(),NULL,&g_pPixelShader));
	pPSBlob->Release();

	ID3DBlob* pGSBlob = NULL;
	V_RETURN(DXUTCompileFromFile(L"raymarch.fx", nullptr, "GS", "gs_5_0", COMPILE_FLAG, 0, &pGSBlob));
	V_RETURN(pd3dDevice->CreateGeometryShader(pGSBlob->GetBufferPointer(),pGSBlob->GetBufferSize(),NULL,&g_pGeometryShader));
	pGSBlob->Release();

	D3D11_INPUT_ELEMENT_DESC inputLayout[]=
	{{ "POSITION", 0, DXGI_FORMAT_R16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}};
	V_RETURN(pd3dDevice->CreateInputLayout(inputLayout,ARRAYSIZE(inputLayout),pVSBlob->GetBufferPointer(),pVSBlob->GetBufferSize(),&g_pVertexLayout));
	pVSBlob->Release();

	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( short );
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	V_RETURN(pd3dDevice->CreateBuffer(&bd,NULL,&g_pVertexBuffer));

	// Create the constant buffers
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0    ;
	bd.ByteWidth = sizeof(CBChangesEveryFrame);
	V_RETURN(pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBChangesEveryFrame ));


	// Create the sample state
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory( &sampDesc, sizeof(sampDesc) );
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP       ;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP       ;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP       ;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	V_RETURN(pd3dDevice->CreateSamplerState( &sampDesc, &g_pSamplerLinear ));


	ID3D11DeviceContext* pd3dImmediateContext=DXUTGetD3D11DeviceContext();
	pd3dImmediateContext->IASetInputLayout(g_pVertexLayout);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	XMVECTORF32 vecEye = { 500.0f, 500.0f, -500.0f };
	XMVECTORF32 vecAt = { 0.0f, 0.0f, 0.0f };
	g_Camera.SetViewParams( vecEye, vecAt );

	volume.CreateResource( pd3dDevice );
	
	return hr;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
										 const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	// Setup the camera's projection parameters
	float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
	g_Camera.SetProjParams(XM_PI / 4, fAspectRatio, 10.0f, 10000.0f);
	g_Camera.SetWindow(pBackBufferSurfaceDesc->Width,pBackBufferSurfaceDesc->Height );
	g_Camera.SetButtonMasks( MOUSE_RIGHT_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );
	g_mViewport.Width = (float)pBackBufferSurfaceDesc->Width ;
	g_mViewport.Height = (float)pBackBufferSurfaceDesc->Height;
	g_mViewport.MinDepth = 0.0f;
	g_mViewport.MaxDepth = 1.0f;
	g_mViewport.TopLeftX = 0;
	g_mViewport.TopLeftY = 0;
	return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	g_Camera.FrameMove(fElapsedTime);
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
								 double fTime, float fElapsedTime, void* pUserContext )
{
	volume.Render( pd3dImmediateContext );

	if (g_bRender){
		// Clear render target and the depth stencil 
		float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
		ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
		pd3dImmediateContext->ClearRenderTargetView(pRTV, ClearColor);
		pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);

		pd3dImmediateContext->RSSetViewports(1, &g_mViewport);
		XMMATRIX m_Proj = g_Camera.GetProjMatrix();
		XMMATRIX m_View = g_Camera.GetViewMatrix();
		XMMATRIX m_World = g_Camera.GetWorldMatrix();
		g_mWorldViewProjection = m_World*m_View*m_Proj;

		g_cbPerFrame.mWorldViewProjection = XMMatrixTranspose(g_mWorldViewProjection);
		XMStoreFloat4(&g_cbPerFrame.mViewPos, g_Camera.GetEyePt());

		pd3dImmediateContext->UpdateSubresource(g_pCBChangesEveryFrame, 0, NULL, &g_cbPerFrame, 0, 0);
		pd3dImmediateContext->IASetInputLayout(g_pVertexLayout);
		UINT stride = sizeof(short);
		UINT offset = 0;
		pd3dImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
		pd3dImmediateContext->OMSetRenderTargets(1, &pRTV, NULL);

		pd3dImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

		pd3dImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
		pd3dImmediateContext->GSSetShader(g_pGeometryShader, NULL, 0);
		pd3dImmediateContext->GSSetConstantBuffers(0, 1, &g_pCBChangesEveryFrame);
		pd3dImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
		pd3dImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBChangesEveryFrame);
		pd3dImmediateContext->PSSetShaderResources(0, 1, &volume.m_pVolumeSRV);
		//pd3dImmediateContext->PSSetShaderResources( 0, 1, &(volume.m_pVolumeTexSRV[0]));
		pd3dImmediateContext->Draw(1, 0);

		ID3D11ShaderResourceView* ppSRVNULL[2] = { NULL, NULL };
		pd3dImmediateContext->PSSetShaderResources(0, 2, ppSRVNULL);
	}
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
	SAFE_RELEASE( g_pVertexShader );
    SAFE_RELEASE( g_pPixelShader );
    SAFE_RELEASE( g_pGeometryShader );
    SAFE_RELEASE( g_pVertexLayout );
    SAFE_RELEASE( g_pVertexBuffer );
    SAFE_RELEASE( g_pCBChangesEveryFrame );
    SAFE_RELEASE( g_pSamplerLinear );
	volume.Release();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
						 bool* pbNoFurtherProcessing, void* pUserContext )
{
	g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );
	switch (uMsg)
	{
	case WM_KEYDOWN:
		int nKey = static_cast<int>(wParam);
		if (nKey == 'R')
		{
			g_bRender = !g_bRender;
		}
		break;
	}
	return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Handle mouse button presses
//--------------------------------------------------------------------------------------
void CALLBACK OnMouse( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
					  bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta,
					  int xPos, int yPos, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved( void* pUserContext )
{
	return true;
}


//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	// DXUT will create and use the best device (either D3D9 or D3D11) 
	// that is available on the system depending on which D3D callbacks are set below

	// Set general DXUT callbacks
	DXUTSetCallbackFrameMove( OnFrameMove );
	DXUTSetCallbackKeyboard( OnKeyboard );
	DXUTSetCallbackMouse( OnMouse );
	DXUTSetCallbackMsgProc( MsgProc );
	DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
	DXUTSetCallbackDeviceRemoved( OnDeviceRemoved );


	// Set the D3D11 DXUT callbacks. Remove these sets if the app doesn't need to support D3D11
	DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
	DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
	DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
	DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
	DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
	DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

	// Perform any application-level initialization here

	DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
	DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
	DXUTCreateWindow( L"VolumeUpdate" );

	// Only require 10-level hardware
	DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 800, 600 );
	DXUTMainLoop(); // Enter into the DXUT render loop

	// Perform any application-level cleanup here

	return DXUTGetExitCode();
}


