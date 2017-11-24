// Fill out your copyright notice in the Description page of Project Settings.

#include "VRPawn.h"
#include "IHeadMountedDisplay.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"


// Sets default values
AVRPawn::AVRPawn(const class FObjectInitializer &PCIP):Super(PCIP)
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	enableGravity = false;
	
	oculusLocationOffset = FVector(0.f, 0.f, 150.f);
	
	this->BaseEyeHeight = 0.f;


	// add scene component for headset positioning, set to -11. to ensure headset start at floor
	RootComponent = PCIP.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneRoot"));

	scene = PCIP.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneComponent"));
	scene->SetRelativeLocation(FVector(0.f, 0.f, -110.f));
	scene->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	// add camera
	camera = PCIP.CreateDefaultSubobject<UCameraComponent>(this, TEXT("Camera"));
	camera->AttachToComponent(scene, FAttachmentTransformRules::KeepRelativeTransform);
	camera->SetFieldOfView(110.f);
	
	// add capsule collision, set default VR half height and radius values
	capsuleCollision = PCIP.CreateDefaultSubobject<UCapsuleComponent>(this, TEXT("CapsuleCollision"));
	capsuleCollision->SetCapsuleHalfHeight(96.f);
	capsuleCollision->SetCapsuleRadius(22.f);
	capsuleCollision->SetRelativeLocation(FVector(0.f, 0.f, -110.f));
	capsuleCollision->AttachToComponent(camera, FAttachmentTransformRules::KeepRelativeTransform);

	// add left and right motion controllers
	motionLeftController = PCIP.CreateDefaultSubobject<UMotionControllerComponent>(this, TEXT("motionLeftController"));
	motionLeftController->Hand = EControllerHand::Left;
	motionLeftController->AttachToComponent(scene, FAttachmentTransformRules::KeepRelativeTransform);
	motionLeftController->SetRelativeLocation(FVector(0.f, 0.f, 110.f));

	motionRightController = PCIP.CreateDefaultSubobject<UMotionControllerComponent>(this, TEXT("motionRightController"));
	motionRightController->Hand = EControllerHand::Right;
	motionRightController->AttachToComponent(scene, FAttachmentTransformRules::KeepRelativeTransform);
	motionRightController->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
}

void AVRPawn::overridePawnValues(float PawnBaseEyeHeight, float FOV, float CapsuleHalfHeight, float CapsuleRadius, 
	FVector CapsuleRelativeLocation, FVector SceneLocation, FVector LeftControllerLocation, FVector RightControllerLocation) {
	
	// Set pawn base eye height
	this->BaseEyeHeight = PawnBaseEyeHeight;

	// Set camera field of view
	camera->SetFieldOfView(FOV);

	// Set capsule collision setting
	capsuleCollision->SetCapsuleHalfHeight(CapsuleHalfHeight);
	capsuleCollision->SetCapsuleRadius(CapsuleRadius);
	capsuleCollision->SetRelativeLocation(CapsuleRelativeLocation);

	// Set scene location
	scene->SetRelativeLocation(SceneLocation);

	// Set motion controller location
	motionLeftController->SetRelativeLocation(LeftControllerLocation);
	motionRightController->SetRelativeLocation(RightControllerLocation);

}

// Pawn Rotation - useful for static mouse rotation during development
void AVRPawn::rotatePawn(float RotationRate, float XAxisInput, float YAxisInput) {
	if (XAxisInput != 0.f) {
		this->AddActorLocalRotation(FRotator(0.f, XAxisInput*RotationRate, 0.f));
	}
	if (YAxisInput != 0.f) {
		this->AddActorLocalRotation(FRotator(0.f, YAxisInput*RotationRate, 0.f));
	}

}

bool AVRPawn::isHMDWorn() {
	if (GEngine->HMDDevice.IsValid()) {
		if (GEngine->HMDDevice->GetHMDWornState() == EHMDWornState::Worn) return true;
	}
	return false;
}

void AVRPawn::printDebugMessage(FString Message, bool OverWriteExisting, float Duration, FColor Color)
{
	int32 key;
	if (OverWriteExisting)
		key = 0;
	else
		key = 1;

	GEngine->AddOnScreenDebugMessage(key, Duration, Color, Message);
}


// Called when the game starts or when spawned
void AVRPawn::BeginPlay()
{
	Super::BeginPlay();
	
	// Adjust pawn spawn target offset based on hmd
	if (GEngine->HMDDevice.IsValid()) {

		// Override height offset for Oculus Rift
		switch (GEngine->HMDDevice->GetHMDDeviceType()) {
		case EHMDDeviceType::DT_OculusRift:
				this->SetActorLocation(this->GetActorLocation() + oculusLocationOffset);
				GEngine->HMDDevice->SetTrackingOrigin(EHMDTrackingOrigin::Floor);
				break;
			default:break;
		}

		// set tracking origin (Oculus & Vive)
		GEngine->HMDDevice->SetTrackingOrigin(EHMDTrackingOrigin::Floor);
	}

}

// Called every frame
void AVRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Get current position of the camera
	FVector cameraPosition = camera->GetComponentTransform().GetLocation();

	// Apply gravity if enabled and camera is positioned at head of player
	if (enableGravity&&camera->IsValidLowLevel() && cameraPosition.Z && this->GetActorLocation().Z) {
		
		// Set line trace for gravity variables
		FHitResult RayHit(EForceInit::ForceInit);
		FCollisionQueryParams RayTraceParams(FName(TEXT("GravityRayTrace")), true, this->GetOwner());

		// Initialize Gravity Trace Hit Result var
		RayTraceParams.bTraceComplex = true;
		RayTraceParams.bTraceAsyncScene = true;
		RayTraceParams.bReturnPhysicalMaterial = true;

		HitResult = GetWorld()->LineTraceSingleByChannel(
			RayHit, 
			cameraPosition, 
			cameraPosition + FVector(0.f, 0.f, FMath::Abs(gravityVariable.floorTraceRange)*-1.f), 
			ECollisionChannel::ECC_Visibility, RayTraceParams);

		// Check if we need to float the Pawn over uneven terrain
		if (gravityVariable.respondToUnevenTerrain && HitResult &&
			RayHit.GetComponent()->CanCharacterStepUpOn == ECanBeCharacterBase::ECB_Yes &&
			(RayHit.Distance + gravityVariable.floorTraceRange) < gravityVariable.floorTraceRange) 	this->TeleportTo(this->GetActorLocation() + FVector(0.f, 0.f, gravityVariable.floorTraceRange - RayHit.Distance), this->GetActorRotation());

		
		// Apply gravity
		if(HitResult||RayHit.GetComponent()->CanCharacterStepUpOn!=ECanBeCharacterBase::ECB_Yes)
			this->TeleportTo(this->GetActorLocation() + gravityVariable.gravityDirection*gravityVariable.gravityStrength, this->GetActorRotation());

	}
}

// Called to bind functionality to input
void AVRPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

