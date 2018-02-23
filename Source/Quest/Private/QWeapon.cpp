// Fill out your copyright notice in the Description page of Project Settings.

#include "QWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "TimerManager.h"
#include "Quest.h"
#include "Net/UnrealNetwork.h"
#include "QTarget.h"


// Sets default values
AQWeapon::AQWeapon()
{
	NetUpdateFrequency = 70.0f;
	MinNetUpdateFrequency = 35.0f;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	WeaponDamage = 20.0f;
	RateOfFire = 600.0f;
	MuzzleSocketName = "MuzzleSocket";
	TraceTargetName = "Target";
	
	SetReplicates(true);
}

void AQWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60.0f / RateOfFire;
}

void AQWeapon::Fire()
{

	if (Role < ROLE_Authority)
	{
		ServerFire();
	}

	// Trace the world, from pawn to crosshair location
	AActor* Owner = GetOwner();
	if (Owner)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, TEXT("OWNERSHIP"));
		FVector EyeLocation;
		FRotator EyeRotation;
		Owner->GetActorEyesViewPoint(EyeLocation, EyeRotation);
		
		FVector ShotDirection = EyeRotation.Vector();
		FVector TraceEnd = EyeLocation + (ShotDirection * 10000);
		
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Owner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		FVector TraceEndPoint = TraceEnd;
		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams))
		{
			// Hit Confirmed, Process Damage
			AActor* HitActor = Hit.GetActor();

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			if (Cast<AQTarget>(HitActor))
			{
				Cast<AQTarget>(HitActor)->GotHit();
			}

			float NewDamage = WeaponDamage;
			if (SurfaceType == SURFACE_FLESHVULNERABLE)
				NewDamage *= 3.0f;

			UGameplayStatics::ApplyPointDamage(HitActor, NewDamage, ShotDirection, Hit, Owner->GetInstigatorController(), this, DamageType);

			PlayImpactEffects(SurfaceType, Hit.ImpactPoint);

			TraceEndPoint = Hit.ImpactPoint;
		}

		PlayFireEffects(TraceEndPoint);
		LastTimeFired = GetWorld()->TimeSeconds;

		if (Role == ROLE_Authority)
		{
			HitScanTrace.TraceEnd = TraceEndPoint;
			HitScanTrace.SurfaceType = SurfaceType;
		}
	}
}

void AQWeapon::OnRep_HitScanTrace()
{
	PlayFireEffects(HitScanTrace.TraceEnd);
	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceEnd);
}

void AQWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AQWeapon, HitScanTrace, COND_SkipOwner);
}

void AQWeapon::ServerFire_Implementation()
{
	Fire();
}

bool AQWeapon::ServerFire_Validate()
{
	return true;
}

void AQWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastTimeFired + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &AQWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}

void AQWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

void AQWeapon::PlayFireEffects(FVector TraceEndPoint)
{

	APawn* Owner = Cast<APawn>(GetOwner());
	if (Owner)
	{
		APlayerController* PC = Cast<APlayerController>(Owner->GetController());
		if (PC)
			PC->ClientPlayCameraShake(FireCameraShake);
	}

	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	if (TraceEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		UParticleSystemComponent* TraceComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TraceEffect, MuzzleLocation);
		if (TraceComp)
			TraceComp->SetVectorParameter(TraceTargetName, TraceEndPoint);
	}
}

void AQWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector PointOfImpact)
{
	UParticleSystem* SelectedEffect = nullptr;

	switch (SurfaceType)
	{
	case SURFACE_FLESHDEFAULT:
	case SURFACE_FLESHVULNERABLE:
		SelectedEffect = FleshImpactEffect;
		break;
	case SURFACE_TARGET:
	case SURFACE_TARGETVULNERABLE:
	default:
		SelectedEffect = DefaultImpactEffect;
		break;
	}

	if (SelectedEffect)
	{
		FVector Direction = PointOfImpact - MeshComp->GetSocketLocation(MuzzleSocketName);
		Direction.Normalize();
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, PointOfImpact, Direction.Rotation());
	}
}

