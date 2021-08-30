#include "main.hxx"

GLOBAL App;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_DESTROY) { PostQuitMessage(0); }
	return DefWindowProcA(hWnd, msg, wParam, lParam);
}

int main()
{
	printf("[INFO]: %s\n", "Initialising DirectX...");

	App.hr = D3D12GetDebugInterface(IID_PPV_ARGS(&App.pDebug));
	if (SUCCEEDED(App.hr)) { App.pDebug->EnableDebugLayer(); }
	else { printf("[WARN]: %s Failed with Status 0x%08lX\n", "D3D12GetDebugInterface()", App.hr); }

	App.hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&App.pFactory);
	if (FAILED(App.hr)) { printf("[WARN]: %s Failed with Status 0x%08lX\n", "CreateDXGIFactory()", App.hr); goto end; }

	for (App.i = 0; App.pFactory->EnumAdapters(App.i, &App.pAdapter) != DXGI_ERROR_NOT_FOUND; ++App.i)
	{
		App.mAdapters[App.i] = App.pAdapter;
		App.hr = App.pAdapter->GetDesc(&App.descriptor);
		if (FAILED(App.hr)) { continue; }
		
		printf("[INFO]: Device %llu:\n", App.i);
		printf("\tDescription: %ls\n", App.descriptor.Description);
		printf("\tVendorId: %u\n", App.descriptor.VendorId);
		printf("\tDeviceId: %u\n", App.descriptor.DeviceId);
		printf("\tSubSysId: %u\n", App.descriptor.SubSysId);
		printf("\tRevision: %u\n", App.descriptor.Revision);
		printf("\tDedicatedVideoMemory: 0x%016llX\n", App.descriptor.DedicatedVideoMemory);
		printf("\tDedicatedSystemMemory: 0x%016llX\n", App.descriptor.DedicatedSystemMemory);
		printf("\tSharedSystemMemory: 0x%016llX\n", App.descriptor.SharedSystemMemory);
		printf("\tLUID: 0x%08lX%08lX\n", App.descriptor.AdapterLuid.HighPart, App.descriptor.AdapterLuid.LowPart);
	}

end:
	if (App.pFactory) { App.pFactory->Release(); }
	printf("Press Any Key to Continue...\n");
	App.any = getc(stdin);
	return 0;
}