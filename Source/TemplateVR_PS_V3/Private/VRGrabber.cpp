// Copyright (C) 2017 Nicolas Lehmann NglStudio

#include "VRGrabber.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "DrawDebugHelpers.h"
//#include "Engine/DirectionalLight.h"
#include "Kismet/KismetMathLibrary.h"
#include "Misc/OutputDeviceNull.h"

// Sets default values for this component's properties
UVRGrabber::UVRGrabber()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;


	distanceFromController = 10.0f;
	minDistanceFromController = 1.0f;
	maxDistanceFromController = 20.0f;

	FVector controllerLocation = FVector::ZeroVector;
	FRotator controllerRotation = FRotator::ZeroRotator;

	EGrabTypeEnum grabType = EGrabTypeEnum::PRECISION_GRAB;
	FVector newGrabbedLocation = FVector::ZeroVector;
	FRotator standardOffset = FRotator::ZeroRotator;
	FRotator rotationOffset = FRotator::ZeroRotator;

	manualAttach = false;

	isPullingOrPushing = false;
	speed = 1.f;

	// ...
}





// Called every frame
void UVRGrabber::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Pull-Push Mechanic
	if (grabbedObject && isPullingOrPushing)
	{
		// Update controller location & rotation
		controllerLocation = GetAttachParent()->GetComponentLocation();
		controllerRotation = GetAttachParent()->GetComponentRotation();

		updatePullPush();
	}
	// Update grabbed object location & rotation (if any)
	else if (grabbedObject && !manualAttach) {

		// Update controller location & rotation
		controllerLocation = GetAttachParent()->GetComponentLocation();
		controllerRotation = GetAttachParent()->GetComponentRotation();

		switch (grabType)
		{
		case EGrabTypeEnum::PRECISION_GRAB:
		case EGrabTypeEnum::SNAP_GRAB:

			// Add controller rotation offsets
			controllerRotation.Add(standardOffset.Pitch, standardOffset.Yaw, standardOffset.Roll);
			if (rotationOffset != FRotator::ZeroRotator)
			{
				controllerRotation.Add(rotationOffset.Pitch, rotationOffset.Yaw, rotationOffset.Roll);
			}

			// Set grabbed object rotation
			grabbedObject->SetTargetLocation(controllerLocation + (controllerRotation.Vector() * distanceFromController));
			grabbedObject->SetTargetRotation(controllerRotation);
			break;

		case EGrabTypeEnum::LOCK_GRAB:
		case EGrabTypeEnum::DANGLING_GRAB:
		case EGrabTypeEnum::PRECISION_LOCK:
			grabbedObject->SetTargetLocation(controllerLocation + (controllerRotation.Vector() * distanceFromController));
			break;

		default:
			break;
		}
	}




}


AActor* UVRGrabber::grab(float reach, bool scanOnlyWillManuallyAttach, EGrabTypeEnum grabMode, FName tagName, FRotator rotation_Offset, bool retainObjectRotation, bool retainDistance, bool showDebugLine) {


	// Set component vars
	grabType = grabMode;
	distanceFromController = reach;
	rotationOffset = rotation_Offset;
	manualAttach = scanOnlyWillManuallyAttach;

	// Update controller location & rotation
	controllerLocation = GetAttachParent()->GetComponentLocation();
	controllerRotation = GetAttachParent()->GetComponentRotation();

	// Calculate Standard Offset - invert Roll to ensure rotation of grabbed objects are retained
	if (controllerRotation.Roll < 0)
	{
		standardOffset = FRotator(0.f, 0.f, FMath::Abs(controllerRotation.Roll));
	}
	else if (controllerRotation.Roll > 0)
	{
		standardOffset = FRotator(0.f, 0.f, controllerRotation.Roll * -1.f);
	}


	// Show Debug line (helpful for a visual indicator during testing)
	if (showDebugLine) {
		// Draw Debug Line Trace
		DrawDebugLine(
			GetWorld(),
			controllerLocation,
			controllerLocation + (controllerRotation.Vector() * reach),
			FColor(255, 0, 0),
			false, -1, 0,
			12.0f
		);
	}

	// Line trace
	AActor* actorHit = getHit(controllerLocation, controllerLocation + (controllerRotation.Vector() * reach), retainDistance, showDebugLine);

	// Check if there's a valid object to grab
	if (actorHit)
	{
		// Only grab an object with a Physics Handle
		grabbedObject = actorHit->FindComponentByClass<UPhysicsHandleComponent>();
		//UE_LOG(LogTemp, Warning, TEXT("GRABBER - I grabbed : %s"), *ActorHit->GetName());

		// Automatic Attachment - Attach to Physics Handle
		if (grabbedObject)
		{
			// Check for actor tag
			if (!tagName.IsNone())
			{
				if (!grabbedObject->ComponentHasTag(tagName))
				{
					//UE_LOG(LogTemp, Warning, TEXT("GRABBER - Couldn't find %s tag in this physics handle."), *TagName.ToString());
					return nullptr;
				}
			}

			// Do a Physics Handle Grab if automatic attachment is selected
			if (!manualAttach)
			{
				// Physics Handle found! Attempt to Grab Object
				UPrimitiveComponent* ComponentToGrab = Cast<UPrimitiveComponent>(actorHit->GetRootComponent());

				// Make object face controller
				//FRotator TempRotator = UKismetMathLibrary::FindLookAtRotation(GrabbedObject->GetOwner()->GetActorLocation(), GetAttachParent()->GetComponentLocation());
				//GrabbedObject->SetTargetRotation(TempRotator);

				// Check for precision grab
				if (grabType == EGrabTypeEnum::PRECISION_GRAB)
				{
					// Grab
					grabbedObject->GrabComponentAtLocationWithRotation(
						ComponentToGrab,
						NAME_None,
						newGrabbedLocation, // NewGrabbedLocation holds the impact point of the line trace
						retainObjectRotation ? actorHit->GetActorRotation() : controllerRotation
					);

					// Set transform
					grabbedObject->SetTargetLocation(controllerLocation + (controllerRotation.Vector() * distanceFromController));
					grabbedObject->SetTargetRotation(controllerRotation);

				}
				else if (grabType == EGrabTypeEnum::DANGLING_GRAB)
				{
					// Grab
					grabbedObject->GrabComponentAtLocation(
						ComponentToGrab,
						NAME_None,
						newGrabbedLocation // NewGrabbedLocation holds the impact point of the line trace
					);

					// Set transform
					grabbedObject->SetTargetLocation(controllerLocation + (controllerRotation.Vector() * distanceFromController));
					grabbedObject->SetTargetRotation(controllerRotation);

				}
				else if (grabType == EGrabTypeEnum::PRECISION_LOCK)
				{
					// Grab
					grabbedObject->GrabComponentAtLocationWithRotation(
						ComponentToGrab,
						NAME_None,
						newGrabbedLocation, // NewGrabbedLocation holds the impact point of the line trace
						retainObjectRotation ? actorHit->GetActorRotation() : controllerRotation
					);

					// Set transform
					grabbedObject->SetTargetLocation(controllerLocation + (controllerRotation.Vector() * distanceFromController));
				}
				else
				{
					// Grab
					grabbedObject->GrabComponentAtLocationWithRotation(
						ComponentToGrab,
						NAME_None,
						actorHit->GetActorLocation(),
						retainObjectRotation ? actorHit->GetActorRotation() : FRotator::ZeroRotator
					);

					// Set transform
					grabbedObject->SetTargetLocation(controllerLocation + (controllerRotation.Vector() * distanceFromController));
					if (grabType == EGrabTypeEnum::SNAP_GRAB)
					{
						grabbedObject->SetTargetRotation(controllerRotation);
					}

				}
			}

			// UE_LOG(LogTemp, Warning, TEXT("GRABBER - Returning Actor %s."), *ActorHit->GetName());
			return actorHit;
		}

	}

	return nullptr;

}


void UVRGrabber::setDistanceFromController(float newDistance, float minDistance, float maxDistance) {

	// Set specified bounds
	minDistanceFromController = minDistance;
	maxDistanceFromController = maxDistance;

	//UE_LOG(LogTemp, Warning, TEXT("GRABBER - MinDistance: %f   MaxDistance: %f"), MinDistanceFromController, MaxDistanceFromController);
	//UE_LOG(LogTemp, Warning, TEXT("GRABBER - CurrentDistance: %f   Speed: %f"), DistanceFromController, Speed);

	// Check if we're pulling
	if (speed < 0.f)
	{
		if (newDistance > minDistanceFromController) {
			distanceFromController = newDistance;
			if (grabbedObject)
			{
				// Update controller location & rotation
				//UE_LOG(LogTemp, Warning, TEXT("GRABBER - PULLING..."));
				controllerLocation = GetAttachParent()->GetComponentLocation();
				controllerRotation = GetAttachParent()->GetComponentRotation();
				grabbedObject->SetTargetLocation(controllerLocation + (controllerRotation.Vector() * distanceFromController));

			}
		}
	}
	// Check if we're pushing
	else if (speed > 0.f)
	{
		if (newDistance < maxDistanceFromController) {
			distanceFromController = newDistance;
			if (grabbedObject)
			{
				// Update controller location & rotation
				//UE_LOG(LogTemp, Warning, TEXT("GRABBER - PUSHING..."));
				controllerLocation = GetAttachParent()->GetComponentLocation();
				controllerRotation = GetAttachParent()->GetComponentRotation();
				grabbedObject->SetTargetLocation(controllerLocation + (controllerRotation.Vector() * distanceFromController));
			}
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("GRABBER - NewDistance: %f   Speed: %f"),NewDistance, Speed);



}

AActor* UVRGrabber::release() {


	//isGrabbingSun = false;

	if (grabbedObject) {


		// Check if we're currently pulling or pushing the grabbed object
		if (isPullingOrPushing)
		{
			stopPull();
			stopPush();
		}

		// Save the currently attached object
		AActor* currentlyGrabbed = grabbedObject->GetOwner();

		if (!manualAttach)
		{
			// Player has latched on to something, release it
			grabbedObject->ReleaseComponent();
		}

		grabbedObject = nullptr;
		return currentlyGrabbed;
	}

	return nullptr;
}


void UVRGrabber::pullGrabbedObject(float pullSpeed, float minDistance, float maxDistance) {

	// Update controller location & rotation
	controllerLocation = GetAttachParent()->GetComponentLocation();
	controllerRotation = GetAttachParent()->GetComponentRotation();

	if (grabbedObject) {
		// Set variables and begin pull
		speed = FMath::Abs(pullSpeed) * -1.f;
		minDistanceFromController = minDistance;
		maxDistanceFromController = maxDistance;
		isPullingOrPushing = true;
	}

}


void UVRGrabber::pushGrabbedObject(float pushSpeed, float minDistance, float maxDistance) {

	// Update controller location & rotation
	controllerLocation = GetAttachParent()->GetComponentLocation();
	controllerRotation = GetAttachParent()->GetComponentRotation();

	if (grabbedObject) {
		// Set variables and begin pull
		speed = FMath::Abs(pushSpeed);
		minDistanceFromController = minDistance;
		maxDistanceFromController = maxDistance;
		isPullingOrPushing = true;
	}

}

AActor* UVRGrabber::stopPull() {

	if (speed < 0.f)
	{
		// Stop Pull
		isPullingOrPushing = false;
	}
	else
	{
		return nullptr;
	}

	if (grabbedObject)
	{
		// Save the currently attached object
		AActor* currentlyGrabbed = grabbedObject->GetOwner();
		return currentlyGrabbed;
	}

	return nullptr;
}

AActor* UVRGrabber::stopPush() {


	if (speed > 0.f)
	{
		// Stop Push
		isPullingOrPushing = false;
	}
	else
	{
		return nullptr;
	}

	if (grabbedObject)
	{
		// Save the currently attached object
		AActor* currentlyGrabbed = grabbedObject->GetOwner();
		return currentlyGrabbed;
	}

	return nullptr;
}




AActor* UVRGrabber::getHit(FVector lineTraceStart, FVector lineTraceEnd, bool retainDistance, bool showDebugLine) {

	// Do line trace / ray-cast
	FHitResult	hit;
	FCollisionQueryParams traceParameters(FName(TEXT("")), false, GetOwner());
	GetWorld()->LineTraceSingleByObjectType(
		hit,
		lineTraceStart,
		lineTraceEnd,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_PhysicsBody),
		traceParameters
	);

	// See what we hit
	auto actorHit = hit.GetActor();

	// Return any hits
	if (actorHit) {

		// Update Distance with hit distance
		if (!retainDistance)
		{
			distanceFromController = hit.Distance;
		}

		// Set Grabbed transform for precision grabs
		newGrabbedLocation = hit.ImpactPoint;

		// Send back actor that was hit by the line trace
		return actorHit;
	}
	else {
		return nullptr;
	}
}

void UVRGrabber::updatePullPush() {

	// Get the distance from the controller
	distanceFromController = FVector::Distance(controllerLocation, grabbedObject->GetOwner()->GetActorLocation());

	// Try to set the new distance
	setDistanceFromController(distanceFromController + speed, minDistanceFromController, maxDistanceFromController);


}

