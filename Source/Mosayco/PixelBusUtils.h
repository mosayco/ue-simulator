#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/StaticMeshComponent.h"
#include "PixelBusUtils.generated.h"

UCLASS()
class MOSAYCO_API UPixelBusUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Mosayco")
    static void PaintCubeFromPacket(
        const TArray<uint8>& FullPacket,
        UStaticMeshComponent* TargetMesh,
        UPARAM(ref) UTextureRenderTarget2D*& TargetRT,
        UMaterialInterface* BaseMaterial
    );
};