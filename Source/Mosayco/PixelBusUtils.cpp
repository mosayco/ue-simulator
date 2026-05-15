#include "PixelBusUtils.h"
#include "TextureResource.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "Materials/MaterialInstanceDynamic.h"

void UPixelBusUtils::PaintCubeFromPacket(const TArray<uint8>& FullPacket, UStaticMeshComponent* TargetMesh, UTextureRenderTarget2D*& TargetRT, UMaterialInterface* BaseMaterial)
{
    // 1. Safety Checks & Debugging
    if (FullPacket.Num() < 772)
    {
        UE_LOG(LogTemp, Warning, TEXT("PixelBus Error: Packet too small! Expected 772, got %d"), FullPacket.Num());
        return;
    }

    if (FullPacket[0] != 0xAB || FullPacket[1] != 0xCD)
    {
        UE_LOG(LogTemp, Warning, TEXT("PixelBus Error: Magic number mismatch! Expected AB CD, got %X %X"), FullPacket[0], FullPacket[1]);
        return;
    }

    if (!TargetMesh) return;

    // 2. Render Target Setup (FIXED: Forced to 8-bit BGRA to match TD data)
    if (!TargetRT)
    {
        UE_LOG(LogTemp, Log, TEXT("PixelBus: Creating 8-bit BGRA RenderTarget..."));
        TargetRT = NewObject<UTextureRenderTarget2D>(TargetMesh);
        // PF_B8G8R8A8 is the standard 8-bit format Unreal uses for the GPU
        TargetRT->InitCustomFormat(16, 16, PF_B8G8R8A8, false);
        TargetRT->Filter = TextureFilter::TF_Nearest;
        TargetRT->UpdateResource();
    }

    // 3. Material Setup
    UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(TargetMesh->GetMaterial(0));
    if (!MID && BaseMaterial)
    {
        MID = TargetMesh->CreateDynamicMaterialInstance(0, BaseMaterial);
    }

    if (MID)
    {
        MID->SetTextureParameterValue(FName("LED_Texture"), TargetRT);
    }

    // 4. RGB to BGRA Conversion (FIXED: Swizzled order and alignment)
    TArray<uint8> BGRAData;
    BGRAData.SetNumUninitialized(1024); // 16x16 * 4 channels

    for (int32 i = 0; i < 256; i++)
    {
        // Src skips 4 byte header, then 3 bytes per pixel (TD sends RGB)
        int32 Src = 4 + (i * 3);
        int32 Dst = i * 4;

        if (Src + 2 < FullPacket.Num())
        {
            // Unreal's 8-bit format (PF_B8G8R8A8) expects Blue first!
            BGRAData[Dst] = FullPacket[Src + 2]; // B
            BGRAData[Dst + 1] = FullPacket[Src + 1]; // G
            BGRAData[Dst + 2] = FullPacket[Src];     // R
            BGRAData[Dst + 3] = 255;                 // A
        }
    }

    // 5. Update GPU
    FTextureRenderTargetResource* RTResource = TargetRT->GameThread_GetRenderTargetResource();
    if (RTResource)
    {
        FUpdateTextureRegion2D Region(0, 0, 0, 0, 16, 16);

        // This sends the BGRAData to the Render Thread safely
        ENQUEUE_RENDER_COMMAND(UpdatePixelBusTexture)([RTResource, Region, BGRAData](FRHICommandListImmediate& RHICmdList)
            {
                RHIUpdateTexture2D(RTResource->GetTexture2DRHI(), 0, Region, 16 * 4, BGRAData.GetData());
            });
    }
}