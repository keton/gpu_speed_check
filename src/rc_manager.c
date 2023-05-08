#include "windows.h"
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include "rc_manager.h"
#include "rcdata.h"

static int unpack_rc(WORD wId, LPSTR lpFname)
{
	int ret = 0;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	// unpack file in current executable directory
	char executable_path[MAX_PATH] = { 0 };
	char tgt_full_path[MAX_PATH] = { 0 };
	if(!GetModuleFileName(NULL, executable_path, MAX_PATH)) {
		return 1;
	}
	if(!PathRemoveFileSpec(executable_path)) {
		return 1;
	}

	if(!PathCombine(tgt_full_path, executable_path, lpFname)) {
		return 1;
	}

	// do not extract if file exists
	if(PathFileExistsA(tgt_full_path)) {
		return 0;
	}

	HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(wId), RT_RCDATA);
	if(!hResource) {
		return 1;
	}

	HGLOBAL hGlobal = LoadResource(NULL, hResource);
	if(!hGlobal) {
		return 1;
	}

	LPVOID lpLock = LockResource(hGlobal);
	if(!lpLock) {
		ret = 1;
		goto cleanup;
	}

	DWORD dwSize = SizeofResource(NULL, hResource);
	if(!dwSize) {
		ret = 1;
		goto cleanup;
	}

	hFile = CreateFile(tgt_full_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
					   NULL);
	if(hFile == INVALID_HANDLE_VALUE) {
		ret = 1;
		goto cleanup;
	}

	DWORD dwByteWritten;
	if(!WriteFile(hFile, lpLock, dwSize, &dwByteWritten, NULL) || dwByteWritten == 0) {
		ret = 1;
		goto cleanup;
	}

cleanup:
	if(hGlobal) {
		FreeResource(hGlobal);
	}
	if(hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}

	return ret;
}

int rc_manager_init(void)
{
	if(unpack_rc(RC_DIRECTIO_DLL, TEXT(RC_DIRECTIO_DLL_NAME))) {
		return 1;
	}

	if(unpack_rc(RC_PCIIDS, TEXT(RC_PCIIDS_NAME))) {
		return 1;
	}

	return 0;
}
