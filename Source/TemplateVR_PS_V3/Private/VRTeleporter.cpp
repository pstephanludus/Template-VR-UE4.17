// Fill out your copyright notice in the Description page of Project Settings.

/*
Gnu general public license version 3

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "VRTeleporter.h"
#include "Kismet/KismetMathLibrary.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "IHeadMountedDisplay.h"


// Sets default values for this component's properties
UVRTeleporter::UVRTeleporter()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Auto activate this component
	bAutoActivate = true;

	teleportBeamMesh = nullptr;
	
	beamMagnitude = 500.f;

	beamLocationOffset = FVector::ZeroVector;

	rayInstantScale = true;

	rayScaleRate = 1.f;

	beamHitNavMeshTolerance = FVector(10.f, 10.f, 10.f);

	arcOverrideGravity = 0.f;

	teleportTargetPawnSpawnOffset = FVector(0.f, 0.f, 0.f);

	floorIsAtZ = 0.f;

	teleportTargetMesh = nullptr;

	teleportTargetMeshScale = FVector(1.f, 1.f, 1.f);

	teleportTargetMeshSpawnOffset = FVector(0.f, 0.f, 0.f);

	customMarkerRotation = FRotator::ZeroRotator;

	faceMarkerRotation = false;

	teleportTargetParticle = nullptr;

	teleportTargetParticleScale = FVector(1.f, 1.f, 1.f);

	teleportTargetParticleSpawnOffset = FVector(0.f, 0.f, 0.f);

	isTeleporting = false;

	// Teleport target height offset - defaults to SteamVR
	pawnHeightOffset = FVector(0.f, 0.f, 112.f);

	// Teleport targetting mode
	teleportMode = -1;

	// Teleport Arc spline parameters
	arcSpline = nullptr;
	rayMeshScale = FVector(1.0f, 1.0f, 1.0f);
	rayMeshScale_Max = FVector(1.0f, 1.0f, 1.0f);
	isBeamTypeTeleport = false;
	rayNumOfTimesToScale = 0.f;
	rayNumOfTimesToScale_Actual = 0.f;
	rayDistanceToTarget = 0.f;
	
	// TeleportRay mesh
	rayMesh = nullptr;

	// Teleport target location
	targetLocation = FVector::ZeroVector;
	targetRotation = FRotator::ZeroRotator;
	isTargetLocationValid = false;

	// Spawned visible components for targetting marker
	targetParticleSystemComponent = nullptr;
	targetStaticMeshComponent = nullptr;


	// ...
}


// Called when the game starts
void UVRTeleporter::BeginPlay()
{
	Super::BeginPlay();

	// Ensure target marker is not visible at start
	SetVisibility(false, true);

	// Set object types for the teleport arc to ignore
	arcObjectTypesToIgnore.Add(EObjectTypeQuery::ObjectTypeQuery1); // World static objects

	arcSpline = NewObject<USplineComponent>(GetAttachParent());
	arcSpline->RegisterComponentWithWorld(GetWorld());
	arcSpline->SetMobility(EComponentMobility::Movable);
	arcSpline->AttachToComponent(GetAttachParent(), FAttachmentTransformRules::KeepRelativeTransform);

	// Adjust pawn spawn target offset based on HMD
	static const FName HMDName = GEngine->HMDDevice->GetDeviceName();

	if (GEngine->HMDDevice.IsValid())
	{
		// Override height offset for Oculus Rift
		if (HMDName == FName(TEXT("OculusRift")))
		{
			pawnHeightOffset.Z = 262.f;
		}
	}
	
}


// Called every frame
void UVRTeleporter::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (isTeleporting && isBeamTypeTeleport)
	{
		if (teleportMode == 0)
		{
			drawTeleportArc();
		}
		else if (teleportMode == 1)
		{
			drawTeleportRay();
		}
	}
}

bool UVRTeleporter::showTeleportArc()
{
	if (!isTeleporting)
	{
		teleportMode = 0;
		isTeleporting = true;
		isBeamTypeTeleport = true;
		spawnTargetMarker();
		return true;
	}

	return false;
}

// Show teleport ray in world 
// Set teleportMode, isTeleporting, isBeamTypeTeleport
bool UVRTeleporter::showTeleportRay()
{
	if (!isTeleporting)
	{
		teleportMode = 1;
		isTeleporting = true;
		isBeamTypeTeleport = true;
		spawnTargetMarker();
		rayNumOfTimesToScale_Actual = 0.f;

		return true;
	}

	return false;
}


bool UVRTeleporter::hideTeleportArc()
{
	{
		if (isTeleporting)
		{
			teleportMode = -1;
			isTeleporting = false;
			isBeamTypeTeleport = false;
			clearTeleportArc();

			// Clear Target Marker
			removeTargetMarker();
			return true;
		}

		return false;
	}
}

bool UVRTeleporter::hideTeleportRay()
{
	{
		if (isTeleporting)
		{
			teleportMode = -1;
			isTeleporting = false;
			isBeamTypeTeleport = false;
			clearTeleportRay();
			rayMeshScale = FVector(1.0f, 1.0f, 1.0f);

			// Clear Target Marker
			removeTargetMarker();
			return true;
		}

		return false;
	}
}

bool UVRTeleporter::showMarker()
{
	if (!isTeleporting)
	{
		// Calculate Target Location
		targetLocation = GetAttachParent()->GetComponentLocation() +
			(GetAttachParent()->GetComponentRotation().Vector() * beamMagnitude);

		// Check if target location is within the nav mesh
		FVector tempTargetLocation;
		bool isWithinNavBounds = GetWorld()->GetNavigationSystem()->
			K2_ProjectPointToNavigation(
				this,
				targetLocation,
				tempTargetLocation,
				(ANavigationData*)0, 0,
				beamHitNavMeshTolerance);



		if (isWithinNavBounds)
		{
			// Set Target Marker Visibility
			targetRotation = UKismetMathLibrary::FindLookAtRotation(targetLocation, GetOwner()->GetActorLocation());
			SetTargetMarkerLocationAndRotation(targetLocation, targetRotation);
			setTargetMarkerVisibility(true);
			isTargetLocationValid = true;
		}
		else
		{
			return false;
		}

		// Set teleport parameters
		teleportMode = 2;
		isTeleporting = true;
		isBeamTypeTeleport = false;
		isTargetLocationValid = true;
		// Show target marker
		spawnTargetMarker();
		targetRotation = UKismetMathLibrary::FindLookAtRotation(targetLocation, GetOwner()->GetActorLocation());
		targetLocation.Z = floorIsAtZ;

		// Calculate Rotation of marker to face player and set the new transform
		SetTargetMarkerLocationAndRotation(targetLocation, targetRotation);

		// Make target marker visible
		setTargetMarkerVisibility(true);

		return true;
	}

	return false;
}

bool UVRTeleporter::moveMarker(EMoveDirectionEnum markerDirection, int rate, FRotator customDirection)
{


	// Only move market if it is visible and active
	if (isTeleporting) {
		switch (markerDirection)
		{
		case EMoveDirectionEnum::MOVE_FORWARD:
			// Calculate target location
			targetLocation = FVector(targetLocation + (targetRotation.Vector() * rate));
			targetLocation.Z = floorIsAtZ;
			SetTargetMarkerLocationAndRotation(targetLocation, targetRotation);

		case EMoveDirectionEnum::MOVE_BACKWARD:
			// Calculate target location
			targetLocation = FVector(targetLocation + (targetRotation.Vector() * -rate));
			targetLocation.Z = floorIsAtZ;
			SetTargetMarkerLocationAndRotation(targetLocation, targetRotation);

		case EMoveDirectionEnum::MOVE_LEFT:
			// Tilt original marker location to point Westwards
			customDirection = targetRotation;
			customDirection.Yaw += 90.0f;

			// Calculate target location
			targetLocation = targetLocation + (customDirection.Vector() * rate);
			targetLocation.Z = floorIsAtZ;
			SetTargetMarkerLocationAndRotation(targetLocation, targetRotation);

		case EMoveDirectionEnum::MOVE_RIGHT:
			// Tilt original marker location to point Eastwards
			customDirection = targetRotation;
			customDirection.Yaw += 90.0f;

			// Calculate target location
			targetLocation = targetLocation + (customDirection.Vector() * -rate);
			targetLocation.Z = floorIsAtZ;
			SetTargetMarkerLocationAndRotation(targetLocation, targetRotation);

		case EMoveDirectionEnum::MOVE_CUSTOM:
			targetLocation = targetLocation + (customDirection.Vector() * rate);
			targetLocation.Z = floorIsAtZ;
			SetTargetMarkerLocationAndRotation(targetLocation, targetRotation);

		default:
			break;
		}
	}
	return true;
}

bool UVRTeleporter::hideMarker()
{
	if (isTeleporting) {
		teleportMode = -1;
		isTeleporting = false;
		isBeamTypeTeleport = false;
		isTargetLocationValid = false;

		// Clear Target Marker
		removeTargetMarker();

		return true;
	}
	return false;
}

bool UVRTeleporter::teleportNow()
{
	// Only teleport if targetting is enabled
	if (isTeleporting && isTargetLocationValid) {

		// Get HMD Position & Orientation
		FRotator HMDRotation;
		FVector HMDLocation;
		UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(HMDRotation, HMDLocation);

		// Teleport
		targetLocation = FVector(targetLocation - FVector(HMDLocation.X, HMDLocation.Y, 0.f));
		GetAttachParent()->GetOwner()->SetActorLocation(targetLocation + pawnHeightOffset + teleportTargetPawnSpawnOffset, false, nullptr, ETeleportType::None);

		// Set custom rotation if needed
		if (faceMarkerRotation)
		{
			GetAttachParent()->GetOwner()->SetActorRotation(customMarkerRotation);
		}

		// Remove teleport artifacts
		switch (teleportMode)
		{
		case 0:
			hideTeleportArc();
			break;

		case 1:
			hideTeleportRay();
			hideMarker();
			break;

		default:
			break;
		}

		// Reset Teleport mode
		teleportMode = -1;

		return true;
	}
	return false;
}

void UVRTeleporter::drawTeleportArc()
{
	// Set Teleport Arc Parameters
	FPredictProjectilePathParams params = FPredictProjectilePathParams(
		arcRadius,
		FVector(GetAttachParent()->GetComponentLocation().X + beamLocationOffset.X,
			GetAttachParent()->GetComponentLocation().Y + beamLocationOffset.Y,
			GetAttachParent()->GetComponentLocation().Z + beamLocationOffset.Z),
		GetAttachParent()->GetForwardVector() * beamMagnitude,
		maxSimTime);
	params.bTraceWithCollision = true;
	params.bTraceComplex = false;
	params.DrawDebugType = EDrawDebugTrace::None;
	params.DrawDebugTime = 0.f;
	params.SimFrequency = simFrequency;
	params.ObjectTypes = arcObjectTypesToIgnore;
	params.OverrideGravityZ = arcOverrideGravity;
	params.bTraceWithChannel = false;

	// Do the arc trace
	FPredictProjectilePathResult predictResult;
	bool hitResult = UGameplayStatics::PredictProjectilePath(this, params, predictResult);

	// Show Target Marker (if a valid teleport location)
	if (hitResult) {
		bool isWithinNavBounds = GetWorld()->GetNavigationSystem()->K2_ProjectPointToNavigation(
			this,
			predictResult.HitResult.Location,
			targetLocation,
			(ANavigationData*)0, 0,
			beamHitNavMeshTolerance);

		// Check if arc hit location is within the nav mesh
		if (isWithinNavBounds)
		{
			// Set Marker location
			targetLocation = predictResult.HitResult.Location;

			// Check marker rotation
			if (customMarkerRotation.Equals(FRotator::ZeroRotator))
			{
				targetRotation = customMarkerRotation;
			}
			else
			{
				targetRotation = UKismetMathLibrary::FindLookAtRotation(targetLocation, GetOwner()->GetActorLocation());
			}

			// Apply marker position and orientation
			SetTargetMarkerLocationAndRotation(targetLocation, targetRotation);

			// Set Target MArker Visibility
			setTargetMarkerVisibility(true);
			isTargetLocationValid = true;
		}
		else
		{
			// Set Target Marker Visibility
			setTargetMarkerVisibility(false);
			isTargetLocationValid = false;
		}
	}
	else
	{
		// Set Target Marker Visibility
		setTargetMarkerVisibility(false);
		isTargetLocationValid = false;
	}

	// Set the teleport arc points
	if (arcSpline)
	{
		// Clean-up old Spline
		clearTeleportArc();

		// Set the point type for the curve
		arcSpline->SetSplinePointType(arcPoints.Num() - 1, ESplinePointType::CurveClamped, true);

		for (const FPredictProjectilePathPointData& pathPoint : predictResult.PathData)
		{
			// Add the point to the arc spline
			arcPoints.Add(pathPoint.Location);
			arcSpline->AddSplinePoint(pathPoint.Location,
				ESplineCoordinateSpace::Local, true);
		}
	}

	// Populate arc point with meshes
	if (teleportBeamMesh)
	{
		for (int32 i = 0; i < arcPoints.Num() - 2; i++)
		{
			// Add the arc mesh
			USplineMeshComponent* arcMesh = NewObject<USplineMeshComponent>(arcSpline);
			arcMesh->RegisterComponentWithWorld(GetWorld());
			arcMesh->SetMobility(EComponentMobility::Movable);
			arcMesh->SetStaticMesh(teleportBeamMesh);
			arcSplineMesh.Add(arcMesh);

			// Bend mesh to conform to arc
			arcMesh->SetStartAndEnd(arcPoints[i],
				arcSpline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local),
				arcPoints[i + 1],
				arcSpline->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::Local),
				true);
		}
	}
}

void UVRTeleporter::clearTeleportArc()
{
	// Clear Arc
	arcPoints.Empty();
	arcSpline->ClearSplinePoints();

	for (int32 i = 0; i < arcSplineMesh.Num(); i++)
	{
		if (arcSplineMesh[i])
		{
			arcSplineMesh[i]->DestroyComponent();
		}
	}

	arcSplineMesh.Empty();
}

void UVRTeleporter::drawTeleportRay()
{
	// Setup ray trace
	FCollisionQueryParams ray_TraceParams(FName(TEXT("Ray_Trace")), true, this->GetOwner());
	ray_TraceParams.bTraceComplex = true;
	ray_TraceParams.bTraceAsyncScene = true;
	ray_TraceParams.bReturnPhysicalMaterial = false;

	// Initialize Hit Result var
	FHitResult ray_Hit(ForceInit);

	// Get Target Location
	targetLocation = FVector(GetAttachParent()->GetComponentLocation().X + beamLocationOffset.X,
		GetAttachParent()->GetComponentLocation().Y + beamLocationOffset.Y,
		GetAttachParent()->GetComponentLocation().Z + beamLocationOffset.Z) +
		(GetAttachParent()->GetComponentRotation().Vector() * beamMagnitude);

	// Do the ray trace
	bool hitResult = GetWorld()->LineTraceSingleByObjectType(
		ray_Hit,
		GetAttachParent()->GetComponentLocation(),
		FVector(GetAttachParent()->GetComponentLocation().X + beamLocationOffset.X,
			GetAttachParent()->GetComponentLocation().Y + beamLocationOffset.Y,
			GetAttachParent()->GetComponentLocation().Z + beamLocationOffset.Z) +
			(GetAttachParent()->GetComponentRotation().Vector() * beamMagnitude),
		ECC_WorldStatic,
		ray_TraceParams
	);


	// Reset Target Marker
	setTargetMarkerVisibility(false);
	isTargetLocationValid = false;

	// Check if we hit a possible location to teleport to
	if (hitResult)
	{
		// Check if target location is within the nav mesh
		FVector tempTargetLocation;
		bool isWithinNavBounds = GetWorld()->GetNavigationSystem()->K2_ProjectPointToNavigation(
			this,
			ray_Hit.ImpactPoint,
			tempTargetLocation,
			(ANavigationData*)0, 0,
			beamHitNavMeshTolerance);

		if (isWithinNavBounds)
		{
			// Set Target Marker Visibility
			targetLocation = ray_Hit.ImpactPoint;
			// Check marker rotation
			if (customMarkerRotation.Equals(FRotator::ZeroRotator))
			{
				targetRotation = customMarkerRotation;
			}
			else
			{
				targetRotation = UKismetMathLibrary::FindLookAtRotation(targetLocation, GetOwner()->GetActorLocation());
			}

			SetTargetMarkerLocationAndRotation(targetLocation, targetRotation);
			setTargetMarkerVisibility(true);
			isTargetLocationValid = true;
		}
	}

	// Draw ray mesh
	clearTeleportRay();
	if (teleportBeamMesh)
	{
		// Spawn the beam mesh
		rayMesh = NewObject<UStaticMeshComponent>(GetAttachParent());
		rayMesh->RegisterComponentWithWorld(GetWorld());
		rayMesh->SetMobility(EComponentMobility::Movable);
		rayMesh->AttachToComponent(GetAttachParent(), FAttachmentTransformRules::KeepRelativeTransform);
		rayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		rayMesh->SetStaticMesh(teleportBeamMesh);
		rayMesh->AddLocalOffset(beamLocationOffset);
		rayMesh->SetWorldRotation(UKismetMathLibrary::FindLookAtRotation(FVector(GetAttachParent()->GetComponentLocation().X + beamLocationOffset.X,
			GetAttachParent()->GetComponentLocation().Y + beamLocationOffset.Y,
			GetAttachParent()->GetComponentLocation().Z + beamLocationOffset.Z), targetLocation));

		// Scale the beam mesh
		if (rayInstantScale)
		{
			// Calculate how long the beam should be using RayScaleRate as the base unit
			rayMeshScale = FVector(FVector::Distance(GetComponentLocation(), targetLocation) * rayScaleRate, 1.f, 1.f);
			rayMesh->SetWorldScale3D(rayMeshScale);
		}
		else
		{
			// Scale beam mesh gradually until it reaches the target location
			rayDistanceToTarget = FVector::Distance(GetComponentLocation(), targetLocation);
			rayNumOfTimesToScale = rayDistanceToTarget;
			if (rayNumOfTimesToScale_Actual < rayNumOfTimesToScale)
			{
				// We haven't reached the target location yet, set the mesh scale
				rayMesh->SetWorldScale3D(rayMeshScale);
				rayMeshScale.X = rayMeshScale.X + rayScaleRate;

				// Update temp scale variables
				rayMeshScale_Max = rayMeshScale;
				rayNumOfTimesToScale_Actual += rayScaleRate;
			}
			else
			{
				// Scale mesh to max possible size to hit target location
				rayMesh->SetWorldScale3D(rayMeshScale_Max);
			}
		}
	}
}

void UVRTeleporter::clearTeleportRay()
{
	if (rayMesh)
	{
		// Remove ray mesh component
		rayMesh->DestroyComponent();
		rayMesh = nullptr;
	}
}

void UVRTeleporter::spawnTargetMarker(FVector MarkerLocation, FRotator MarkerRotation)
{
	// Activate Particle System if available
	if (teleportTargetParticle) {
		targetParticleSystemComponent = UGameplayStatics::SpawnEmitterAtLocation(this, teleportTargetParticle, MarkerLocation, MarkerRotation);
		targetParticleSystemComponent->SetWorldScale3D(teleportTargetParticleScale);
		targetParticleSystemComponent->SetVisibility(false);
		targetParticleSystemComponent->SetMobility(EComponentMobility::Movable);
	}

	// Show Static Mesh if available
	if (teleportTargetMesh) {
		// Create new static mesh component and attach to actor
		targetStaticMeshComponent = NewObject<UStaticMeshComponent>(this);
		targetStaticMeshComponent->RegisterComponentWithWorld(GetWorld());
		targetStaticMeshComponent->SetSimulatePhysics(false);
		targetStaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		targetStaticMeshComponent->SetMobility(EComponentMobility::Movable);
		targetStaticMeshComponent->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

		//Set Mesh
		targetStaticMeshComponent->SetVisibility(false);
		targetStaticMeshComponent->SetWorldScale3D(teleportTargetMeshScale);
		targetStaticMeshComponent->SetStaticMesh(teleportTargetMesh);
	}

}

void UVRTeleporter::setTargetMarkerVisibility(bool makeVisible) {

	// Activate Particle System if available
	if (targetParticleSystemComponent) {
		targetParticleSystemComponent->SetVisibility(makeVisible);
	}

	// Show Static Mesh if available
	if (targetStaticMeshComponent) {
		targetStaticMeshComponent->SetVisibility(makeVisible);
	}
}

void UVRTeleporter::removeTargetMarker()
{
	// Destroy Particle System if available
	if (targetParticleSystemComponent) {
		targetParticleSystemComponent->DestroyComponent();
		targetParticleSystemComponent = nullptr;
	}

	// Destroy Static Mesh if available
	if (targetStaticMeshComponent) {
		targetStaticMeshComponent->DestroyComponent();
		targetStaticMeshComponent = nullptr;
	}

	isTargetLocationValid = false;
	customMarkerRotation = FRotator::ZeroRotator;
}

void UVRTeleporter::SetTargetMarkerLocationAndRotation(FVector MarkerLocation, FRotator MarkerRotation)
{
	// Activate Particle System if available
	if (targetParticleSystemComponent) {
		targetParticleSystemComponent->SetWorldLocation(MarkerLocation + teleportTargetParticleSpawnOffset);
		targetParticleSystemComponent->SetWorldRotation(MarkerRotation);
	}

	// Show Static Mesh if available
	if (targetStaticMeshComponent) {
		targetStaticMeshComponent->SetWorldLocation(MarkerLocation + teleportTargetMeshSpawnOffset);
		targetStaticMeshComponent->SetWorldRotation(MarkerRotation);
	}
}
