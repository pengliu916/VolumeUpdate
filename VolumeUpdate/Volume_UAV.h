#pragma once
#include <D3D11.h>
#include "DXUT.h"
#include "DXUTcamera.h"
#include "header.h"

class Volume_UAV
{
public:

	D3D11_VIEWPORT					m_cViewport;

	ID3D11VertexShader*				m_pVS;
	ID3D11PixelShader*				m_pPS;
	ID3D11PixelShader*				m_pPS_r;
	ID3D11GeometryShader*			m_pGS;

	ID3D11InputLayout*				m_pVertexLayout;
	ID3D11Buffer*					m_pVertexBuffer;

	//render to volume necessities
	UINT                            m_uVolumeWidth;
	UINT                            m_uVolumeHeight;
	UINT                            m_uVolumeDepth;

	ID3D11Texture3D*				m_pVolumeTex;
	ID3D11ShaderResourceView*		m_pVolumeSRV;
	ID3D11UnorderedAccessView*		m_pVolumeUAV;

	//dummy rendertarget is needed, and its x and y dimension should be the same as the UAV resource
	ID3D11Texture3D*				m_pUAVDescmmyTex;
	ID3D11RenderTargetView*			m_pUAVDescmmyRTV;

	bool							m_bVolumeInitialized;

	Volume_UAV( UINT width, UINT height, UINT depth)
	{
		m_uVolumeWidth = width;
		m_uVolumeHeight = height;
		m_uVolumeDepth = depth;
		m_bVolumeInitialized = false;
	}

	HRESULT CreateResource( ID3D11Device* pd3dDevice )
	{
		HRESULT hr = S_OK;

		ID3DBlob* pVSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(L"Volume_UAV.fx", nullptr,"VS","vs_5_0",COMPILE_FLAG,0,&pVSBlob));
		V_RETURN(pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(),pVSBlob->GetBufferSize(),NULL,&m_pVS));

		ID3DBlob* pPSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(L"Volume_UAV.fx", nullptr, "PS", "ps_5_0", COMPILE_FLAG, 0, &pPSBlob));
		V_RETURN(pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(),pPSBlob->GetBufferSize(),NULL,&m_pPS));
		V_RETURN(DXUTCompileFromFile(L"Volume_UAV.fx", nullptr, "PS_R", "ps_5_0", COMPILE_FLAG, 0, &pPSBlob));
		V_RETURN(pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(),pPSBlob->GetBufferSize(),NULL,&m_pPS_r));
		pPSBlob->Release();

		ID3DBlob* pGSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(L"Volume_UAV.fx", nullptr, "GS", "gs_5_0", COMPILE_FLAG, 0, &pGSBlob));
		V_RETURN(pd3dDevice->CreateGeometryShader(pGSBlob->GetBufferPointer(),pGSBlob->GetBufferSize(),NULL,&m_pGS));
		pGSBlob->Release();

		D3D11_INPUT_ELEMENT_DESC inputLayout[]=
		{{ "POSITION", 0, DXGI_FORMAT_R16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}};
		V_RETURN(pd3dDevice->CreateInputLayout(inputLayout,ARRAYSIZE(inputLayout),pVSBlob->GetBufferPointer(),pVSBlob->GetBufferSize(),&m_pVertexLayout));
		pVSBlob->Release();

		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof(bd) );
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof( short )*m_uVolumeDepth;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		V_RETURN(pd3dDevice->CreateBuffer(&bd,NULL,&m_pVertexBuffer));


		// Create the texture
		D3D11_TEXTURE3D_DESC dstex;
		dstex.Width = m_uVolumeWidth;
		dstex.Height = m_uVolumeHeight;
		dstex.Depth = m_uVolumeDepth;
		dstex.MipLevels = 1;
		dstex.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;// the texture file should have 32bit typeless format
		dstex.Usage = D3D11_USAGE_DEFAULT;
		dstex.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		dstex.CPUAccessFlags = 0;
		dstex.MiscFlags = 0;
		V_RETURN( pd3dDevice->CreateTexture3D( &dstex, NULL, &m_pVolumeTex ) );


		// Create the resource view
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		ZeroMemory( &SRVDesc, sizeof( SRVDesc ) );
		SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;// the srv cannot have typeless formate, the format should match with the conversion func in pixel shader
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		SRVDesc.Texture3D.MostDetailedMip = 0;
		SRVDesc.Texture3D.MipLevels = 1;
		V_RETURN( pd3dDevice->CreateShaderResourceView( m_pVolumeTex, &SRVDesc,&m_pVolumeSRV ) );

		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		ZeroMemory( &UAVDesc, sizeof( UAVDesc ) );
		UAVDesc.Format = DXGI_FORMAT_R32_UINT; // the UAV must have this format to allow simultaneous read and write
		UAVDesc.Texture3D.FirstWSlice = 0;
		UAVDesc.Texture3D.MipSlice = 0;
		UAVDesc.Texture3D.WSize = m_uVolumeDepth;
		UAVDesc.ViewDimension=D3D11_UAV_DIMENSION::D3D11_UAV_DIMENSION_TEXTURE3D;
		V_RETURN( pd3dDevice->CreateUnorderedAccessView(m_pVolumeTex,&UAVDesc,&m_pVolumeUAV));


		dstex.Width = m_uVolumeWidth;
		dstex.Height = m_uVolumeHeight;
		dstex.Depth = 1;// the dummy texture only need the same x y dimension as UAV resource, so set z to 1 to save memory
		dstex.MipLevels = 1;
		dstex.Format = DXGI_FORMAT_A8_UNORM;// choose one that saves memory, but not all format work
		dstex.Usage = D3D11_USAGE_DEFAULT;
		dstex.BindFlags = D3D11_BIND_RENDER_TARGET;
		dstex.CPUAccessFlags = 0;
		dstex.MiscFlags = 0;
		V_RETURN( pd3dDevice->CreateTexture3D( &dstex, NULL, &m_pUAVDescmmyTex ) );

		// Create the render target views
		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = dstex.Format;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
		RTVDesc.Texture3D.MipSlice = 0;
		RTVDesc.Texture3D.FirstWSlice = 0;
		RTVDesc.Texture3D.WSize = 1;
		V_RETURN( pd3dDevice->CreateRenderTargetView( m_pUAVDescmmyTex, &RTVDesc, &m_pUAVDescmmyRTV ) );
	


		m_cViewport.TopLeftX = 0;
		m_cViewport.TopLeftY = 0;
		m_cViewport.Width = (float)m_uVolumeWidth;
		m_cViewport.Height = (float)m_uVolumeHeight;
		m_cViewport.MinDepth = 0.0f;
		m_cViewport.MaxDepth = 1.0f;

		return hr;
	}

	void InitializeVolume( ID3D11DeviceContext* pd3dImmediateContext )
	{
		pd3dImmediateContext->RSSetViewports( 1, &m_cViewport );
		pd3dImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews( 1, &m_pUAVDescmmyRTV, NULL, 1, 1, &m_pVolumeUAV, 0 );
		//pd3dImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews( 0, NULL, NULL, 0, 1, &m_pVolumeUAV, 0 );
		pd3dImmediateContext->IASetInputLayout(m_pVertexLayout);

		UINT stride = sizeof( short );
		UINT offset = 0;
		pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_pVertexBuffer, &stride, &offset );

		pd3dImmediateContext->VSSetShader( m_pVS, NULL, 0 );
		pd3dImmediateContext->GSSetShader( m_pGS ,NULL,0);
		pd3dImmediateContext->PSSetShader( m_pPS, NULL, 0 );
		pd3dImmediateContext->Draw(m_uVolumeDepth, 0);

		m_bVolumeInitialized = true;
	}

	void Release()
	{
		SAFE_RELEASE( m_pVS );
		SAFE_RELEASE( m_pGS );
		SAFE_RELEASE( m_pPS );
		SAFE_RELEASE( m_pPS_r );

		SAFE_RELEASE( m_pVertexLayout );
		SAFE_RELEASE( m_pVertexBuffer );
		
		SAFE_RELEASE( m_pVolumeTex );
		SAFE_RELEASE( m_pVolumeSRV );
		SAFE_RELEASE( m_pVolumeUAV );

		SAFE_RELEASE( m_pUAVDescmmyTex );
		SAFE_RELEASE( m_pUAVDescmmyRTV );
		
	}

	void Render( ID3D11DeviceContext* pd3dImmediateContext )
	{
		if( !m_bVolumeInitialized ) InitializeVolume( pd3dImmediateContext );

		pd3dImmediateContext->RSSetViewports( 1, &m_cViewport );
		pd3dImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews( 1, &m_pUAVDescmmyRTV, NULL, 1, 1, &m_pVolumeUAV, 0 );
		//pd3dImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews( 0, NULL, NULL, 0, 1, &m_pVolumeUAV, 0 );
		pd3dImmediateContext->IASetInputLayout(m_pVertexLayout);

		UINT stride = sizeof( short );
		UINT offset = 0;
		pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_pVertexBuffer, &stride, &offset );

		pd3dImmediateContext->VSSetShader( m_pVS, NULL, 0 );
		pd3dImmediateContext->GSSetShader( m_pGS ,NULL,0);
		pd3dImmediateContext->PSSetShader( m_pPS_r, NULL, 0 );
		pd3dImmediateContext->Draw(m_uVolumeDepth, 0);
	}

};