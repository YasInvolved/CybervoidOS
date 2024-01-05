#include <efi.h>
#include <efilib.h>

#define newline() Print(L"\n");

/*
    Bootloader graphic related functions
*/

EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;

EFI_STATUS InitGop() {
    EFI_STATUS status;

    status = uefi_call_wrapper(
        gBS->LocateProtocol,
        3,
        &GraphicsOutputProtocol,
        NULL,
        (VOID **)&GOP
    );

    if (EFI_ERROR(status)) {
        Print(L"Failed to locate GOP: %r\n", status);
        return status;
    }

    return EFI_SUCCESS;
}

EFI_GRAPHICS_OUTPUT_BLT_PIXEL GraphicsColor;
void SetGraphicscolor(UINT32 color)
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL Gcolor;
    Gcolor.Reserved = color >> 24;
    Gcolor.Red = color >> 16;
    Gcolor.Green = color >> 8;
    Gcolor.Blue = color;
    GraphicsColor = Gcolor;
}
void CreateFilledBox(UINT32 xPos, UINT32 yPos, UINT32 w, UINT32 h) {
    uefi_call_wrapper(GOP->Blt, 9, GOP, &GraphicsColor, EfiBltVideoFill, 0, 0, xPos, yPos, w, h, 0);
}

/*
    Console output related functions
*/

EFI_STATUS SetTextColor(UINTN foreground, UINTN background) {
    EFI_STATUS status;

    status = uefi_call_wrapper(
        ST->ConOut->SetAttribute,
        2,
        ST->ConOut,
        EFI_TEXT_ATTR(foreground, background)
    );

    if (EFI_ERROR(status)) {
        Print(L"Failed to set text color: %r\n", status);
        return status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS ClearSceen() {
    EFI_STATUS status;

    status = uefi_call_wrapper(
        ST->ConOut->ClearScreen,
        1,
        ST->ConOut
    );
}

EFI_STATUS SetCursorPosition(UINT32 col, UINT32 row) {
    return uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, col, row);
}

INT32 GetCursorRow() {
    return ST->ConOut->Mode->CursorRow;
}

INT32 GetCursorCol() {
    return ST->ConOut->Mode->CursorColumn;
}

EFI_STATUS PrintInfo(CHAR16* Message) {
    Print(L"[INFO] %s\n", Message);
    return EFI_SUCCESS;
}

EFI_STATUS PrintError(CHAR16* Message) {
    SetTextColor(EFI_RED, EFI_BACKGROUND_BLACK);
    Print(L"[ERROR] %s\n", Message);
    SetTextColor(EFI_WHITE, EFI_BACKGROUND_BLACK);
    return EFI_SUCCESS;
}

EFI_STATUS PrintWarn(CHAR16* Message) {
    SetTextColor(EFI_YELLOW, EFI_BACKGROUND_BLACK);
    Print(L"[WARN] %s\n", Message);
    SetTextColor(EFI_WHITE, EFI_BACKGROUND_BLACK);
    return EFI_SUCCESS;
}

/*
    Console input related functions
*/

EFI_INPUT_KEY CheckKeystroke;

void ResetKeyboard() { ST->ConIn->Reset(ST->ConIn, TRUE); }

BOOLEAN GetKey(CHAR16 key) {
    if (CheckKeystroke.UnicodeChar == key) return TRUE;
    else return FALSE;
}

EFI_STATUS CheckKey() {
    return uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &CheckKeystroke);
}

/*
    IO related functions
*/

EFI_HANDLE IH;
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;
EFI_STATUS InitializeFS() {
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
    status = uefi_call_wrapper(gBS->HandleProtocol, 3, IH, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage);
    if (EFI_ERROR(status)) {
        SetTextColor(EFI_RED, EFI_BACKGROUND_BLACK);
        PrintError(L"Could not load image.");
        SetTextColor(EFI_WHITE, EFI_BACKGROUND_BLACK);
        return status;
    }
    
    EFI_DEVICE_PATH_PROTOCOL *DevicePath;
    EFI_GUID DevicePathProtocolGUID = EFI_DEVICE_PATH_PROTOCOL_GUID;
    status = uefi_call_wrapper(gBS->HandleProtocol, 3, LoadedImage->DeviceHandle, &DevicePathProtocolGUID, (void**)&DevicePath);
    if (EFI_ERROR(status)) {
        PrintError(L"Could not set up filesystem.");
        return status;
    }

    EFI_GUID SimpleFilesystemProtocolGUID = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    status = uefi_call_wrapper(gBS->HandleProtocol, 3, LoadedImage->DeviceHandle, &SimpleFilesystemProtocolGUID, (void**)&Volume);
    if (EFI_ERROR(status)) {
        PrintError(L"Could not set up filesystem.");
        return status;
    }
    return EFI_SUCCESS;
}
EFI_FILE_PROTOCOL* openFile(CHAR16* FileName) {
    EFI_STATUS status;
    Print(L"[INFO] Opening %s...\n", FileName);
    EFI_FILE_PROTOCOL* RootFS;
    status = uefi_call_wrapper(Volume->OpenVolume, 2, Volume, &RootFS);
    if (EFI_ERROR(status)) {
        CHAR16* fmt;
        SetTextColor(EFI_RED, EFI_BACKGROUND_BLACK);
        StatusToString(fmt, status);
        Print(L"[ERROR] Failed to open file %s: %s\n", FileName, fmt);
        SetTextColor(EFI_WHITE, EFI_BACKGROUND_BLACK);
    }

    EFI_FILE_PROTOCOL* FileHandle = NULL;
    status = RootFS->Open(RootFS, &FileHandle, FileName, 0x0000000000000001, 0);
    if (EFI_ERROR(status)) {
        CHAR16* fmt;
        SetTextColor(EFI_RED, EFI_BACKGROUND_BLACK);
        StatusToString(fmt, status);
        Print(L"[ERROR] Failed to open file %s: %s\n", FileName, fmt);
        SetTextColor(EFI_WHITE, EFI_BACKGROUND_BLACK);
    }

    return FileHandle;
}
EFI_STATUS closeFile(EFI_FILE_PROTOCOL *FileHandle) {
    return uefi_call_wrapper(FileHandle->Close, 1, FileHandle);
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    IH = ImageHandle;
    ClearSceen();
    InitGop();
    SetTextColor(EFI_GREEN, EFI_BACKGROUND_BLACK);
    Print(L"Welcome to CybervoidOS!\n\n");
    SetTextColor(EFI_WHITE, EFI_BACKGROUND_BLACK);
    PrintInfo(L"Initializing fs...");
    EFI_STATUS s = InitializeFS();
    if(EFI_ERROR(s)) {
        CHAR16* fmt;
        StatusToString(fmt, s);
        Print(L"[CRITICAL]: Couldn't initialize file system. Abort.\n", fmt);
        ST->BootServices->Stall(10000);
        ST->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, 0);
    }
    // test (temporary)
    void* OSBuffer_Handle;
    UINT64 fileSize = 0x00100000;
    EFI_FILE_PROTOCOL* myExampleFile = openFile(L"example.txt");
    uefi_call_wrapper(ST->BootServices->AllocatePool, 3, EfiLoaderData, fileSize, (void**)&OSBuffer_Handle);
    uefi_call_wrapper(myExampleFile->Read, 3, myExampleFile, &fileSize, OSBuffer_Handle);
    closeFile(myExampleFile);

    Print(L"Użyj 'S' aby wyłączyć komputer, 'R' aby uruchomić ponownie...\n");
    EFI_INPUT_KEY Key;
    // end of program (temporary)
    while ((uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &Key)) == EFI_NOT_READY);
    if (Key.UnicodeChar == L's') ST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, 0);
    if (Key.UnicodeChar == L'r') ST->RuntimeServices->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, 0);
    // Shutdown system after kernel return 
    ST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, 0);
    return EFI_SUCCESS;
}