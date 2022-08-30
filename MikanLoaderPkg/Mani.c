#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTable.h>
#include <Library/PrintLib.h>
#include <Protocols/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/BlockIo.h>

// #@@range_begin(struct_memory_map)
struct MemoryMap{
    UINTN buffer_size;
    VOID* buffer;
    UINTN map_size;
    UINTN map_key;
    UINTN descriptor_size;
    UINT32 descriptor_version;
};
// #@@range_end(struct_memory_map)

// #@@range_begin(get_memory_map)
EFI_STATUS GetMemoryMap(struct MemoryMap* map){
    if(map->buffer == NULL){
        return EFI_BUFFER_TOO_SMALL;
    }

    map->map_size = map->buffer_size;
    return gRS->GetMemoryMap(
        &map->map_size,
        (EFI_MEMORY_DESCRIPTOR*)map->buffer,
        &map->map_key,
        &map->descriptor_size,
        &map->descriptor_version);
}
// #@@range_end(get_memory_map)

// #@@range_begin(get_memory_type)
const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type){
    switch (type){
        case EfiReservedMemoryType: return L"EfiReservedMemoryType";
        case EfiLoaderCode: return L"EfiLoaderCode";
        case EfiLoaderData: return L"EfiLoaderData";
        case EfiBootServicesCode: return L"EfiBootServicesCode";
        case EfiBootServicesData: return L"EfiBootServiceData";
        case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
        case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
        case EfiConventionalMemory: return L"EfiConventionalMemory";
        case EfiUnusableMemory: return L"EfiUnusableMemroy";
        case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
        case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
        case EifMemoryMappedIO: return L"EfiMemoryMappedIO";
        case EFIMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPartSpace";
        case EfiPalCode: return L"EfiPalCode";
        case EfiPersistentMemory: return L"EfiPersistentMemory";
        case EfiMaxMemoryType: return L"EfiMaxMemoryType";
        default: return L"InvalidMemoryType";
    }
}
// #@@range_end(get_memory_type)

// #@@range_begin(save_memory_map)
EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL*file){
    CHAR8 buf(256);
    UINTN len;

    CHAR8* header =
        "Index, Type, Type(name), PysicalStart, NumberOfPages, Attribute\n";
    len = AsciiStrLen(header);
    file->Write(file, &len, header);

    Print(L"map->buffer = %08lx, map->map+size = %08lx\n",
        map->buffer, map->map_size);

    EFI_PHYSICAL_ADDRESS iter;
    int i;
    for(iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
        iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
        iter += map->descriptor_size, i++) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;
        len = AsciiPrint(
            buf, sizeof(buf),
            "%u, %x, %-ls, %08lx, %lx, %lx\n",
            i, desc->Type, GetMemoryTypeUnicode(desc->Type),
            desc->PhysicalStart, desc->NumberOpPages,
            desc->Attribute & 0xfffflu);
        file->Write(file, &len,buf);
        }
    
    return EFI_SUCCESS;
}
// #@@range_end(save_memory_map)

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL**root){
    EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

    gBS->OpenProtocol(
        image_handle,
        &gEfiLoadedImageProtocolGuid,
        (VOID**)&loaded_image,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    fs->OpenVolume(fs, root);

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table){
        Print(L"Hello,Mikan World!\n");

        // #@@range_begin(main)
        CHAR8 memmap_buf[4096*4];
        struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0,0, 0, 0};
        GetMemoryMap(&memmap);

        EFI_FILE_PROTOCOL* root_dir;
        OpenRootDir(image_handle, &root_dir);

        EFI_FILE_PROTOCOL* memmap_file;
        root_dir->Open(
            root_dir, &memmap_file, l"\\memmap",
            EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE |
            EFI_FILE_MODE_CREATE, 0);

        SaveMemoryMap(&memmap, memmap_file);
        memmap_file->Close(memmap_file);
        // #@@range_end(main)

        Print(L"All done\n");
        
        while(1);
        return EFI_SUCCESS;
    }