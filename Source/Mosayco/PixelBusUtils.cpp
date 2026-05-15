#include "PixelBusUtils.h"
#include "TextureResource.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "Materials/MaterialInstanceDynamic.h"

void UPixelBusUtils::PaintCubeFromPacket(const TArray<uint8>& FullPacket, UStaticMeshComponent* TargetMesh, UTextureRenderTarget2D*& TargetRT, UMaterialInterface* BaseMaterial)
{
    // 1. Safety Checks
    if (FullPacket.Num() < 772 || FullPacket[0] != 0xAB || FullPacket[1] != 0xCD) return;
    if (!TargetMesh) return;

    // 2. Auto-Create Render Target
    if (!TargetRT)
    {
        // We use TargetMesh as the owner instead of a separate Outer variable
        TargetRT = NewObject<UTextureRenderTarget2D>(TargetMesh);
        TargetRT->InitAutoFormat(16, 16);
        TargetRT->Filter = TextureFilter::TF_Nearest;
        TargetRT->UpdateResource();

        UMaterialInstanceDynamic* MID = TargetMesh->CreateDynamicMaterialInstance(0, BaseMaterial);
        if (MID)
        {
            MID->SetTextureParameterValue(FName("LED_Texture"), TargetRT);
        }
    }

    // 3. RGB to RGBA
    TArray<uint8> RGBAData;
    RGBAData.SetNumUninitialized(1024);

    for (int32 i = 0; i < 256; i++)
    {
        int32 Src = 4 + (i * 3);
        int32 Dst = i * 4;
        if (Src + 2 < FullPacket.Num()) {
            RGBAData[Dst] = FullPacket[Src];
            RGBAData[Dst + 1] = FullPacket[Src + 1];
            RGBAData[Dst + 2] = FullPacket[Src + 2];
            RGBAData[Dst + 3] = 255;
        }
    }

    // 4. Update GPU
    FTextureRenderTargetResource* RTResource = TargetRT->GameThread_GetRenderTargetResource();
    if (RTResource)
    {
        FUpdateTextureRegion2D Region(0, 0, 0, 0, 16, 16);
        ENQUEUE_RENDER_COMMAND(UpdatePixelBusTexture)([RTResource, Region, RGBAData](FRHICommandListImmediate& RHICmdList)
            {
                RHIUpdateTexture2D(RTResource->GetTexture2DRHI(), 0, Region, 16 * 4, RGBAData.GetData());
            });
    }
}