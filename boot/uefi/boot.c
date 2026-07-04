// ZeroOs UEFI Bootloader - using GNU-EFI

#include <efi.h>
#include <efilib.h>

#define KERNEL_LOAD_ADDRESS 0x100000

// Print ASCII string
static void print(CHAR8 *str) {
    Print(L"%a\n\r", str);
}

// Print hex
static void print_hex(UINT64 value) {
    Print(L"0x%lx", value);
}

// Print decimal
static void print_dec(UINT64 value) {
    Print(L"%lu", value);
}

// Main UEFI entry point
EFI_STATUS EFIAPI efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable) {
    InitializeLib(imageHandle, systemTable);

    // Clear screen and print banner
    gConOut->Reset(gConOut, FALSE);
    gConOut->SetAttribute(gConOut, EFI_LIGHTGREEN);
    Print(L"ZeroOs UEFI Bootloader v0.1\n\r");
    gConOut->SetAttribute(gConOut, EFI_WHITE);

    // Find boot device using block IO protocol
    Print(L"Finding boot device...\n\r");
    EFI_GUID blockIoGuid = BLOCK_IO_PROTOCOL;
    EFI_HANDLE *handles = NULL;
    UINTN handleCount = 0;
    EFI_STATUS status;

    status = gBS->LocateHandleBuffer(ByProtocol, &blockIoGuid, NULL, &handleCount, &handles);
    if (status != EFI_SUCCESS || handleCount == 0) {
        gConOut->SetAttribute(gConOut, EFI_RED);
        Print(L"ERROR: No block devices found\n\r");
        while(1);
    }

    EFI_HANDLE device = handles[0];
    gBS->FreePool(handles);
    Print(L"Boot device found\n\r");

    // Open file system
    Print(L"Opening file system...\n\r");
    EFI_GUID fsGuid = SIMPLE_FILE_SYSTEM_PROTOCOL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = NULL;
    status = gBS->HandleProtocol(device, &fsGuid, (void**)&fs);
    if (status != EFI_SUCCESS || !fs) {
        gConOut->SetAttribute(gConOut, EFI_RED);
        Print(L"ERROR: No file system\n\r");
        while(1);
    }

    // Open root directory
    EFI_FILE_PROTOCOL *root = NULL;
    fs->OpenVolume(fs, &root);

    // Open kernel file
    Print(L"Loading kernel...\n\r");
    EFI_FILE_PROTOCOL *file = NULL;
    status = root->Open(root, &file, L"kernel.bin", EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS || !file) {
        gConOut->SetAttribute(gConOut, EFI_RED);
        Print(L"ERROR: kernel.bin not found\n\r");
        while(1);
    }

    // Get file info to determine size
    UINTN infoSize = 0;
    file->GetInfo(file, &gEfiFileInfoGuid, &infoSize, NULL);
    EFI_FILE_INFO *info = NULL;
    gBS->AllocatePool(EfiBootServicesData, infoSize, (void**)&info);
    file->GetInfo(file, &gEfiFileInfoGuid, &infoSize, info);
    UINT64 fileSize = info->FileSize;
    gBS->FreePool(info);

    Print(L"Kernel size: %lu bytes\n\r", fileSize);

    // Read kernel
    void *kernelBuffer = NULL;
    gBS->AllocatePool(EfiBootServicesData, fileSize, &kernelBuffer);
    UINT64 readSize = fileSize;
    file->Read(file, &readSize, kernelBuffer);
    file->Close(file);
    root->Close(root);

    // Copy kernel to 1MB
    Print(L"Copying kernel to 0x100000...\n\r");
    gBS->CopyMem((void*)KERNEL_LOAD_ADDRESS, kernelBuffer, readSize);
    gBS->FreePool(kernelBuffer);

    // Get memory map
    Print(L"Getting memory map...\n\r");
    EFI_MEMORY_DESCRIPTOR *memMap = NULL;
    UINTN mapSize = 0;
    UINTN mapKey = 0;
    UINTN descSize = 0;
    UINT32 descVersion = 0;

    gBS->GetMemoryMap(&mapSize, NULL, &mapKey, &descSize, &descVersion);
    mapSize += 2 * descSize;
    gBS->AllocatePool(EfiBootServicesData, mapSize, (void**)&memMap);
    gBS->GetMemoryMap(&mapSize, memMap, &mapKey, &descSize, &descVersion);

    UINTN mapEntries = mapSize / descSize;
    Print(L"Memory entries: %lu\n\r", mapEntries);

    // Exit boot services
    Print(L"Exiting boot services...\n\r");
    status = gBS->ExitBootServices(imageHandle, mapKey);
    if (status != EFI_SUCCESS) {
        // Retry
        mapSize = 0;
        gBS->GetMemoryMap(&mapSize, NULL, &mapKey, &descSize, &descVersion);
        mapSize += 2 * descSize;
        gBS->FreePool(memMap);
        gBS->AllocatePool(EfiBootServicesData, mapSize, (void**)&memMap);
        gBS->GetMemoryMap(&mapSize, memMap, &mapKey, &descSize, &descVersion);
        gBS->ExitBootServices(imageHandle, mapKey);
    }

    // Jump to kernel
    typedef void (*kernel_entry_t)(void *memMap, UINTN entries);
    kernel_entry_t kernel_entry = (kernel_entry_t)KERNEL_LOAD_ADDRESS;
    kernel_entry(memMap, mapEntries);

    while(1);
}
