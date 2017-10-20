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

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VRPawn.generated.h"

USTRUCT(BlueprintType)
struct FGravityProperty {


	GENERATED_USTRUCT_BODY()

		// Respond to uneven terrain - Gravity Must be enabled
		UPROPERTY(EditAnywhere, Category = "VR")
			bool respondToUnevenTerrain = false;

		// How fast the VR pawn will fall with gravity
		UPROPERTY(EditAnywhere, Category = "VR")
			float gravityStrength = 3.f;

		// How far should the check for a floor be made
		UPROPERTY(EditAnywhere, Category = "VR")
			float floorTraceRange = 150.f;

		// Minimum Z offset beffore terrain is considered uneven
		UPROPERTY(EditAnywhere, Category = "VR")
			float floorTraceTolerance = 3.f;

		// Direction where this VR pawn will fall
		UPROPERTY(EditAnywhere, Category = "VR")
			FVector gravityDirection = FVector(0.f, 0.f, 0.f);

};

UCLASS()
class TEMPLATEVR_PS_V3_API AVRPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AVRPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	
	
};
