// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/ActorComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine.h"
#include "VRTeleporter.generated.h"


UENUM(BlueprintType)
enum class EMoveDirectionEnum :uint8
{
	MOVE_FORWARD UMETA(DisplayName = "Towards player"),
	MOVE_BACKWARD UMETA(DisplayName = "Away from player"),
	MOVE_LEFT UMETA(DisplayName = "Left of player"),
	MOVE_RIGHT UMETA(DisplayName = "Right of player"),
	MOVE_CUSTOM UMETA(DisplayName = "Use a custom rotation for direction")
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TEMPLATEVR_PS_V3_API UVRTeleporter : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVRTeleporter();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Teleport Beam mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR-Teleport Beam Parameters")
		UStaticMesh *teleportBeamMesh = nullptr;

	// Teleport Beam launch velocity Magnitude - higher number increases range of teleport
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR-Teleport Beam Parameters")
		float beamMagnitude;

	// Location offset from the parent mesh origin where the teleport beam will start
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR-Teleport Beam Parameters")
		FVector beamLocationOffset;
	
	// Ensures the length of the beam reaches target location, uses RayScaleState
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR-Teleport Beam Parameters")
		bool rayInstantScale;

	// How much the ray will scale up until it reaches target location
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR-Teleport Beam Parameters")
		float rayScaleRate;

	// The Teleport NavMesh Beam tolerance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR-Teleport Beam Parameters")
		FVector beamHitNavMeshTolerance;

	// Teleport beam's custom gravity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR-Teleport Beam Parameters")
		float arcOverrideGravity;


	// Offset of pawn (internal offsets - steam: 112, rift: 250)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR-Teleport Target Parameters")
		FVector teleportTargetPawnSpawnOffset;

	// 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR-Teleport Target Parameters")
		float floorIsAtZ;
	
	// 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR-Teleport Target Parameters")
		UStaticMesh *teleportTargetMesh;

	//  
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR-Teleport Target Parameters")
		FVector teleportTargetMeshScale;

	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR-Teleport Target Parameters")
		FVector teleportTargetMeshOffset;

};
