// Fill out your copyright notice in the Description page of Project Settings.

#include "QTarget.h"


// Sets default values
AQTarget::AQTarget()
{
	//Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Target"));

	//Mesh->OnComponentHit.AddDynamic(this, &AQTarget::OnHit);

	isHit = false;
}

// Called when the game starts or when spawned
void AQTarget::BeginPlay()
{
	Super::BeginPlay();
	
}

void AQTarget::GotHit()
{
	isHit = true;
}

//void AQTarget::OnHit(AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
//{
//	GLog->Log(*OtherActor->GetName());
//}
