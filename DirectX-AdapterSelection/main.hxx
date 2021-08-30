#pragma once

#include <stdio.h>

#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

#include <map>

typedef struct _VEC3
{
	float x;
	float y;
	float z;
}   VEC3, *PVEC3, **LPVEC3, ***RLPVEC3, ****IRLPVEC3, *****EIRLPVEC3;

typedef struct _VEC4
{
	float x;
	float y;
	float z;
	float w;
}   VEC4, *PVEC4, **LPVEC4, ***RLPVEC4, ****IRLPVEC4, *****EIRLPVEC4;

typedef struct _VERTEX
{
	VEC3 position;
	VEC4 colour; //UK spelling.
}   VERTEX, *PVERTEX, **LPVERTEX, ***RLPVERTEX, ****IRLPVERTEX, *****EIRLPVERTEX;

typedef struct _GLOBAL
{
	SIZE_T Width = 0;
	SIZE_T Height = 0;
	float Aspect = 0.0;

	BOOL bFound = FALSE;
	ID3D12Debug* pDebug = NULL;
	IDXGIFactory* pFactory = NULL;
	IDXGIAdapter* pAdapter = NULL;
	IDXGIOutput* pOutput = NULL;
	ID3D12Device* pDevice = NULL;
	std::map<SIZE_T, IDXGIAdapter*> mAdapters = std::map<SIZE_T, IDXGIAdapter*>();

	SIZE_T i = 0;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UINT;
	ID3D12Resource* pRenderTargets[2] = { 0 };
	IDXGISwapChain* pSwapchain = NULL;
	
	ID3D12CommandQueue* pCmdQueue = NULL;
	ID3D12GraphicsCommandList* pCommandList = NULL;
	ID3D12CommandAllocator* pAllocator = NULL;
	ID3D12PipelineState* pState = NULL;

	DXGI_ADAPTER_DESC descriptor = { 0 }; //DO NOT REMOVE THE SERVITOR!!!
	DXGI_SWAP_CHAIN_DESC scConfig = { 0 }; //DO NOT REMOVE THE SERVITOR!!!
	D3D12_COMMAND_QUEUE_DESC qConfig = { }; //DO NOT ADD THE SERVITOR!!!
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoConfig = { 0 }; //DO NOT REMOVE THE SERVITOR!!!
	D3D12_RESOURCE_DESC crConfig = { }; //DO NOT ADD THE SERVITOR!!!
	D3D12_ROOT_SIGNATURE_DESC rsConfig = { 0 }; //DO NOT REMOVE THE SERVITOR!!!
	D3D12_DESCRIPTOR_HEAP_DESC rtvConfig = { }; //DO NOT ADD THE SERVITOR!!!

	D3D12_RESOURCE_BARRIER barrier = { }; //DO NOT ADD THE SERVITOR!!!
	ID3D12RootSignature* pRootSignature = NULL;
	ID3DBlob* pSignature = NULL;
	ID3DBlob* pError = NULL;
	HANDLE hFenceEvent = NULL;
	ID3D12Fence* pFence = NULL;
	UINT64 fenceValue = 0;
	UINT frameIndex = 0;

	ID3DBlob* pVertexShader = NULL;
	ID3DBlob* pFragmentShader = NULL;
	D3D12_VERTEX_BUFFER_VIEW shared = { 0 }; //DO NOT REMOVE THE SERVITOR!!!
	D3D12_HEAP_PROPERTIES hConfig = { }; //DO NOT ADD THE SERVITOR!!!
	ID3D12Resource* pVertexBuffer = NULL;
	D3D12_VIEWPORT viewport = { 0 };
	D3D12_RECT scissorRect = { 0 };
	D3D12_CPU_DESCRIPTOR_HANDLE hRTV = { 0 }; //DO NOT REMOVE THE SERVITOR!!!
	ID3D12DescriptorHeap* pHeap = NULL;
	
	UINT szRTV = 0;
	
	WNDCLASSEXA wndClassExA = { 0 }; //DO NOT REMOVE THE SERVITOR!!!
	HWND hWnd = NULL;
	MSG msg = { 0 }; //DO NOT REMOVE THE SERVITOR!!!
	HRESULT hr = S_OK;

	char any = '.';
}   GLOBAL, *PGLOBAL, **LPGLOBAL, ***RLPGLOBAL, ****IRLPGLOBAL, *****EIRLPGLOBAL;
extern GLOBAL App;

void app();