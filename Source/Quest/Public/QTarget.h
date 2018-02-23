// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QTarget.generated.h"

UCLASS()
class QUEST_API AQTarget : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AQTarget();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Target")
	//UStaticMeshComponent* Mesh;

public:	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Target")
	bool isHit;

	void GotHit();

	/*UFUNCTION()
	void RecieveHit(UPrimitiveComponent* MyComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);*/
	
};
