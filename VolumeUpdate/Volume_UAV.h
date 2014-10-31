#pragma once
#include <D3D11.h>
#include "DXUT.h"
#include "DXUTcamera.h"
#include "header.h"

class Volume_UAV
{
public:

	ID3D11ComputeShader*			m_pCS;
	ID3D11ComputeShader*			m_pCS_r;

	//render to volume necessities
	UINT                            m_uVolumeWidth;
	UINT                            m_uVolumeHeight;
	UINT                            m_uVolumeDepth;

	ID3D11Texture3D*				m_pVolumeTex;
	ID3D11ShaderResourceView*		m_pVolumeSRV;
	ID3D11UnorderedAccessView*		m_pVolumeUAV;

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

		ID3DBlob* pCSBlob = NULL;
		V_RETURN(DXUTCompileFromFile(L"Volume_UAV.fx", nullptr, "CS", "cs_5_0", COMPILE_FLAG, 0, &pCSBlob));
		V_RETURN(pd3dDevice->CreateComputeShader(pCSBlob->GetBufferPointer(),pCSBlob->GetBufferSize(),NULL,&m_pCS));
		V_RETURN(DXUTCompileFromFile(L"Volume_UAV.fx", nullptr, "CS_R", "cs_5_0", COMPILE_FLAG, 0, &pCSBlob));
		V_RETURN(pd3dDevice->CreateComputeShader(pCSBlob->GetBufferPointer(),pCSBlob->GetBufferSize(),NULL,&m_pCS_r));
		pCSBlob->Release();
		
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

		return hr;
	}

	void InitializeVolume( ID3D11DeviceContext* pd3dImmediateContext )
	{
		pd3dImmediateContext->CSSetShader(m_pCS, NULL, 0);
		UINT initCounts = 0;
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &m_pVolumeUAV, &initCounts);
		pd3dImmediateContext->Dispatch(VOLUME_SIZE / THREAD_X, VOLUME_SIZE / THREAD_Y, VOLUME_SIZE / THREAD_Z);

		m_bVolumeInitialized = true;
	}

	void Release()
	{
		SAFE_RELEASE( m_pCS );
		SAFE_RELEASE( m_pCS_r );

		SAFE_RELEASE( m_pVolumeTex );
		SAFE_RELEASE( m_pVolumeSRV );
		SAFE_RELEASE( m_pVolumeUAV );		
	}

	void Render( ID3D11DeviceContext* pd3dImmediateContext )
	{
		if( !m_bVolumeInitialized ) InitializeVolume( pd3dImmediateContext );
		pd3dImmediateContext->CSSetShader( m_pCS_r, NULL, 0 );
		UINT initCounts = 0;
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, &m_pVolumeUAV, &initCounts);
		pd3dImmediateContext->Dispatch(VOLUME_SIZE / THREAD_X, VOLUME_SIZE / THREAD_Y, VOLUME_SIZE / THREAD_Z);
		ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
		pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, ppUAViewNULL, &initCounts);
	}

};