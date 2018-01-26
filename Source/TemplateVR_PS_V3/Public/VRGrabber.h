// Copyright (C) 2017 Nicolas Lehmann NglStudio

/*Copyright (C) 2017 Nicolas Lehmann NglStudio

Gnu general public license version 3

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "VRGrabber.generated.h"

class UPhysicsHandleComponent;
//class ADirectionalLight;
//class ULightComponent;


UENUM(BlueprintType)
enum class EGrabTypeEnum : uint8
{
	PRECISION_GRAB 	UMETA(DisplayName = "Precision Grab"),
	SNAP_GRAB 		UMETA(DisplayName = "Snap to Mesh Origin Grab"),
	LOCK_GRAB		UMETA(DisplayName = "Locked Rotation Grab"),
	DANGLING_GRAB	UMETA(DisplayName = "Precision Grab and Dangle"),
	PRECISION_LOCK	UMETA(DisplayName = "Precision Grab with Locked Rotation")

};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TEMPLATEVR_PS_V3_API UVRGrabber : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UVRGrabber();

protected:
	// Called when the game starts
	//virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Current Distance of grabbed items from their respective controllers
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
		float distanceFromController;

	// Min Distance for Controller for grabbed objects  - Customize Push & Pull functions mid action 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR")
		float minDistanceFromController;

	// Min & Max Distance for Controller for grabbed objects  - Customize Push & Pull functions mid action 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR")
		float maxDistanceFromController;


	// Grab something within line trace range of controller
	UFUNCTION(BlueprintCallable, Category = "VR")
		AActor* grab(float reach = 5.f, bool scanOnlyWillManuallyAttach = false, EGrabTypeEnum grabMode = EGrabTypeEnum::PRECISION_GRAB, FName tagName = FName(TEXT("")), FRotator rotation_Offset = FRotator::ZeroRotator, bool retainObjectRotation = true, bool retainDistance = false, bool showDebugLine = false);

	// Set distance from controller
	UFUNCTION(BlueprintCallable, Category = "VR")
		void setDistanceFromController(float newDistance, float minDistance, float maxDistance);

	// Release grabbed object
	UFUNCTION(BlueprintCallable, Category = "VR")
		AActor* release();

	// Pull grabbed object 
	UFUNCTION(BlueprintCallable, Category = "VR")
		void pullGrabbedObject(float pullSpeed = 1.f, float minDistance = 1.f, float maxDistance = 20.f);

	// Push grabbed object(s) 
	UFUNCTION(BlueprintCallable, Category = "VR")
		void pushGrabbedObject(float pushSpeed = 1.f, float minDistance = 1.f, float maxDistance = 20.f);

	// Stop Pull
	UFUNCTION(BlueprintCallable, Category = "VR")
		AActor* stopPull();

	// Stop Push
	UFUNCTION(BlueprintCallable, Category = "VR")
		AActor* stopPush();


private :

	// Motion Controller Transform
	FVector controllerLocation;
	FRotator controllerRotation;

	// Temp Variables
	UPhysicsHandleComponent* grabbedObject = nullptr;
	EGrabTypeEnum grabType;
	FVector newGrabbedLocation;
	FRotator standardOffset;
	FRotator rotationOffset;

	// Get Actor hit by line trace
	AActor* getHit(FVector LineTraceStart, FVector LineTraceEnd, bool RetainDistance, bool bShowDebugLine);
	bool manualAttach;

	// Pull-Push Mechanic
	bool isPullingOrPushing;
	float speed;
	// Update Pulled-Pushed Object
	void updatePullPush();

	

		
	
};
