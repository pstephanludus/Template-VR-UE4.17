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

// Called when the game starts or when spawned
void AVRPawn::BeginPlay()
{
	Super::BeginPlay();
	
	// Adjust pawn spawn target offset based on hmd
	if (GEngine->HMDDevice.isValid()) {

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

}

// Called to bind functionality to input
void AVRPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

