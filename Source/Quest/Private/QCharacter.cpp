// Fill out your copyright notice in the Description page of Project Settings.

#include "QCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "QHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "Quest.h"

// Sets default values
AQCharacter::AQCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Component Setup
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(RootComponent);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	HealthComp = CreateDefaultSubobject<UQHealthComponent>(TEXT("HealthComp"));

	// Movement Setup
	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

	// Initialize Variables
	ZoomedFOV = 65.0f;
	ZoomInterpSpeed = 20.0f;
	WeaponSocketName = "WeaponSocket";
}

// Called when the game starts or when spawned
void AQCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultFOV = CameraComp->FieldOfView;

	if (Role == ROLE_Authority)
	{
		// Default Weapon Spawn
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CurrentWeapon = GetWorld()->SpawnActor<AQWeapon>(StarterWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (CurrentWeapon)
		{
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
		}
	}

	HealthComp->OnHealthChanged.AddDynamic(this, &AQCharacter::OnHealthChanged);
}

void AQCharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector() * Value);
}

void AQCharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector() * Value);
}

void AQCharacter::BeginCrouch()
{
	Crouch();
}

void AQCharacter::EndCrouch()
{
	UnCrouch();
}

void AQCharacter::BeginZoom()
{
	bWantsToZoom = true;
}

void AQCharacter::EndZoom()
{
	bWantsToZoom = false;
}

void AQCharacter::StartFire()
{
	if (CurrentWeapon)
		CurrentWeapon->StartFire();
}

void AQCharacter::StopFire()
{
	if (CurrentWeapon)
		CurrentWeapon->StopFire();
}

void AQCharacter::OnHealthChanged(UQHealthComponent* HealthComponent, float Health, float HealthDelta,
	const class UDamageType* DamageType, class AController* IntigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.0f && !isDead)
	{
		isDead = true;
		
		GetMovementComponent()->StopMovementImmediately();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DetachFromControllerPendingDestroy();
		SetLifeSpan(10.0f);

	}
}

// Called every frame
void AQCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float TargetFOV = bWantsToZoom ? ZoomedFOV : DefaultFOV;
	float NewFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);

	CameraComp->SetFieldOfView(NewFOV);

}

// Called to bind functionality to input
void AQCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AQCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AQCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &AQCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &AQCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AQCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AQCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &AQCharacter::BeginZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &AQCharacter::EndZoom);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AQCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &AQCharacter::StopFire);
}

FVector AQCharacter::GetPawnViewLocation() const
{
	if (CameraComp)
		return CameraComp->GetComponentLocation();

	return Super::GetPawnViewLocation();
}

void AQCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AQCharacter, CurrentWeapon);
	DOREPLIFETIME(AQCharacter, isDead);
}