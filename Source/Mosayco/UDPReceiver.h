#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Networking.h"
#include "Sockets.h"
#include "UDPReceiver.generated.h"

// This creates the "On Data Received" node you'll see in Blueprints
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUDPReceived, const TArray<uint8>&, Data, const FString&, SENDER_IP);

UCLASS()
class MOSAYCO_API AUDPReceiver : public AActor
{
    GENERATED_BODY()

public:
    AUDPReceiver();

    // Settings you can change inside the Unreal Editor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelBus")
    int32 ListenPort = 6969;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelBus")
    FString MulticastGroup = TEXT("239.0.0.1");

    // This is the output pin for your Blueprint
    UPROPERTY(BlueprintAssignable, Category = "PixelBus")
    FOnUDPReceived OnDataReceived;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    FSocket* ListenSocket;
    FUdpSocketReceiver* UDPReceiver;

    // This function handles the raw data when it hits the network card
    void HandleDataReceived(const FArrayReaderPtr& Data, const FIPv4Endpoint& Endpoint);
};