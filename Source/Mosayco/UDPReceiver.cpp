#include "UDPReceiver.h"
#include "Async/Async.h"

AUDPReceiver::AUDPReceiver()
{
    PrimaryActorTick.bCanEverTick = false; // We don't need to tick, we use events!
}

void AUDPReceiver::BeginPlay()
{
    Super::BeginPlay();

    FIPv4Address Addr;
    FIPv4Address::Parse(MulticastGroup, Addr);
    FIPv4Endpoint Endpoint(FIPv4Address::Any, ListenPort);

    // 1. Build the Socket
    ListenSocket = FUdpSocketBuilder(TEXT("PixelBusSocket"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToEndpoint(Endpoint)
        .WithMulticastLoopback()
        .WithMulticastInterface(FIPv4Address::Any);

    if (ListenSocket)
    {
        // 2. Join Multicast Group (The clean UE5.7 way)
        TSharedRef<FInternetAddr> MulticastAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

        bool bIsValid;
        MulticastAddr->SetIp(*MulticastGroup, bIsValid);
        MulticastAddr->SetPort(ListenPort);

        if (bIsValid)
        {
            ListenSocket->JoinMulticastGroup(*MulticastAddr);
        }

        // 3. Setup the Receiver
        UDPReceiver = new FUdpSocketReceiver(ListenSocket, FTimespan::FromMilliseconds(10), TEXT("UDP_RECEIVER"));
        UDPReceiver->OnDataReceived().BindUObject(this, &AUDPReceiver::HandleDataReceived);
        UDPReceiver->Start();

        UE_LOG(LogTemp, Warning, TEXT("UDP Receiver Started: Port %d, Group %s"), ListenPort, *MulticastGroup);
    }
}

void AUDPReceiver::HandleDataReceived(const FArrayReaderPtr& Data, const FIPv4Endpoint& Endpoint)
{
    // Important: Move data from the network thread to the Game Thread
    TArray<uint8> ReceivedData;
    ReceivedData.Append(Data->GetData(), Data->Num());
    FString SenderIP = Endpoint.Address.ToString();

    AsyncTask(ENamedThreads::GameThread, [this, ReceivedData, SenderIP]()
    {
        OnDataReceived.Broadcast(ReceivedData, SenderIP);
    });
}

void AUDPReceiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    // Clean up to prevent memory leaks or "Port already in use" errors
    if (UDPReceiver)
    {
        UDPReceiver->Stop();
        delete UDPReceiver;
        UDPReceiver = nullptr;
    }

    if (ListenSocket)
    {
        ListenSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
        ListenSocket = nullptr;
    }
}