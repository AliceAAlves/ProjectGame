// Fill out your copyright notice in the Description page of Project Settings.


#include "FightingCharacter.h"
#include "Engine/EngineTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/UnrealMathUtility.h"
#include "RenderCore.h"

#include <vector>
#include <random>

#include "Engine.h"

// Sets default values
AFightingCharacter::AFightingCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 600.0f;
	GetCharacterMovement()->AirControl = 0.2f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Cam2LookAt = GetActorLocation() + FVector(100.0f, 0.0f, 0.0f);
	Cam2Location = Cam2LookAt + GetActorForwardVector().RotateAngleAxis(90, FVector(0.0f, 0.0f, 1.0f)).GetSafeNormal()*Cam2Distance;

	Camera2 = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera2"));
	Camera2->SetWorldLocation(Cam2Location);
	Camera2->SetupAttachment(RootComponent);
	Camera2->bUsePawnControlRotation = true;

	bDefeated = false;
	//IsBlocking = false;

	// Collision boxes
	CollisionBoxesInit();
}

// Called when the game starts or when spawned
void AFightingCharacter::BeginPlay()
{
	Super::BeginPlay(); 

	//AddControllerYawInput(-15);

	FollowCamera->Deactivate();
	Camera2->Activate();

	ControllerRotation = UKismetMathLibrary::FindLookAtRotation(Cam2Location, Cam2LookAt);

	ThisPlayerController = UGameplayStatics::GetPlayerController(this, 0);
	ThisPlayerController->SetControlRotation(ControllerRotation);

	IsPlayableChar = UGameplayStatics::GetPlayerPawn(GetWorld(), 0) == this;

	AttachCollisionBoxesToSockets();

	for (UBoxComponent* weapon : WeaponCollisionBoxes) {
		weapon->OnComponentHit.AddDynamic(this, &AFightingCharacter::OnAttackHit);
		weapon->OnComponentBeginOverlap.AddDynamic(this, &AFightingCharacter::OnAttackOverlapBegin);
		weapon->OnComponentEndOverlap.AddDynamic(this, &AFightingCharacter::OnAttackOverlapEnd);
	}

	VariablesInit();

}

// Called every frame
void AFightingCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateToTarget(DeltaTime);

	//Setting max speed on whether is running or not
	if (bIsRunning) {
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->MaxWalkSpeed = MaxRunSpeed;
	}
	else {
		GetCharacterMovement()->bOrientRotationToMovement = false;
		//If it's not running but current speed is more than the max walk speed, decrease it gradually
		if (GetVelocity().Size() > MaxWalkSpeed) {
			GetCharacterMovement()->MaxWalkSpeed -= GetCharacterMovement()->MaxAcceleration*DeltaTime;
		}
		else GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeed;
	}

	//Reposition on Impact - THIS LOOKS LIKE SHITE
	/*ImpactDirection = ImpactDirection.GetSafeNormal2D();
	FVector newLocation = GetActorLocation() + ImpactDirection * (ImpactVelocity*0.01);
	SetActorLocation(newLocation, true);
	
	//if (ImpactVelocity > 0) GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, FString::Printf(TEXT("ImpactDirection: %f, %f, %f"), ImpactDirection.X, ImpactDirection.Y, ImpactDirection.Z));
	//if (ImpactVelocity > 0) GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Blue, FString::Printf(TEXT("ImpactVelocity: %f"), ImpactVelocity));

	if(ImpactVelocity>0) ImpactVelocity -= ImpactDeceleration * DeltaTime;
	if(ImpactVelocity<0) ImpactVelocity = 0;*/


	/*if (this == UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) {
		if (CanAttack) GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Blue, TEXT("Can attack"));
		else GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Red, TEXT("CanNOT attack"));
	}*/
	

	// Tracking velocity of fists/foots when punching/kicking
	if (bTrackFistsVelocity) {
		FVector currentPos = LeftFistCollisionBox->GetComponentLocation();
		LeftFistVelocity = (currentPos - LeftFistLastPos).Size()/DeltaTime;
		LeftFistLastPos = currentPos;

		currentPos = RightFistCollisionBox->GetComponentLocation();
		RightFistVelocity = (currentPos - RightFistLastPos).Size() / DeltaTime;
		RightFistLastPos = currentPos;

		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Blue, FString::Printf(TEXT("RightFistVelocity: %f"), RightFistVelocity));
		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, FString::Printf(TEXT("LeftFistLastPos: %f"), LeftFistVelocity));

		if (LeftFistVelocity > LeftFistVelocity_max) LeftFistVelocity_max = LeftFistVelocity;
		if (RightFistVelocity > RightFistVelocity_max) RightFistVelocity_max = RightFistVelocity;
	}

	if (bTrackFootsVelocity) {
		FVector currentPos = LeftFootCollisionBox->GetComponentLocation();
		LeftFootVelocity = (currentPos - LeftFootLastPos).Size() / DeltaTime;
		LeftFootLastPos = currentPos;

		currentPos = RightFootCollisionBox->GetComponentLocation();
		RightFootVelocity = (currentPos - RightFootLastPos).Size() / DeltaTime;
		RightFootLastPos = currentPos;

		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Blue, FString::Printf(TEXT("RightFootVelocity: %f"), RightFootVelocity));
		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, FString::Printf(TEXT("LeftFootVelocity: %f"), LeftFootVelocity));

		if (LeftFootVelocity > LeftFootVelocity_max) LeftFootVelocity_max = LeftFootVelocity;
		if (RightFootVelocity > RightFootVelocity_max) RightFootVelocity_max = RightFootVelocity;
	}



	/*for (UBoxComponent* db : DamageCollisionBoxes) {
		GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, FString::Printf(TEXT("%s is overlapping"), *db->GetName()));
	}*/

	if (this == UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) {
		
		
	}

	//Set Camera 2 location and rotation
	if (IsPlayableChar && Camera2->IsActive() && TargetEnemy != NULL && ThisPlayerController != NULL) {
		FVector ToTargetDirection = TargetEnemy->GetActorLocation() - GetActorLocation();
		float distance = ToTargetDirection.Size();
		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, FString::Printf(TEXT("distance: %f"), distance));

		float distanceOffset = 0.0f;
		if (distance > 500.0f) {
			distanceOffset = (distance - 500.0f)*0.5;
		}

		Cam2LookAt = GetActorLocation() + ToTargetDirection/2;
		Cam2Location = Cam2LookAt + ToTargetDirection.RotateAngleAxis(90, FVector(0.0f, 0.0f, 1.0f)).GetSafeNormal() * (Cam2Distance + distanceOffset);
		Cam2LookAt.Z = 250;
		Cam2Location.Z = 250;

		Camera2->SetWorldLocation(Cam2Location);
		ControllerRotation = UKismetMathLibrary::FindLookAtRotation(Cam2Location, Cam2LookAt);
		ThisPlayerController->SetControlRotation(ControllerRotation);
	}
	

	//UE_LOG(LogTemp, Warning, TEXT("speed: %f"), GetVelocity().Size());
}

// Called to bind functionality to input
void AFightingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AFightingCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFightingCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &AFightingCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AFightingCharacter::LookUp);

	PlayerInputComponent->BindAction("ChangeCamera", IE_Pressed, this, &AFightingCharacter::ChangeCamera);

	PlayerInputComponent->BindAction("Run", IE_Pressed, this, &AFightingCharacter::Run);
	PlayerInputComponent->BindAction("Run", IE_Released, this, &AFightingCharacter::StopRunning);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AFightingCharacter::JumpChecking);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Attack1", IE_Pressed, this, &AFightingCharacter::Attack1);
	PlayerInputComponent->BindAction("Attack1", IE_Released, this, &AFightingCharacter::StopAttack1);

	PlayerInputComponent->BindAction("Attack2", IE_Pressed, this, &AFightingCharacter::Attack2);
	PlayerInputComponent->BindAction("Attack2", IE_Released, this, &AFightingCharacter::StopAttack2);

	PlayerInputComponent->BindAction("Block", IE_Pressed, this, &AFightingCharacter::Block);
	PlayerInputComponent->BindAction("Block", IE_Released, this, &AFightingCharacter::StopBlocking);

	PlayerInputComponent->BindAction("Duck", IE_Pressed, this, &AFightingCharacter::Duck);
	PlayerInputComponent->BindAction("Duck", IE_Released, this, &AFightingCharacter::StopDucking);

	PlayerInputComponent->BindAction("MoveMod", IE_Pressed, this, &AFightingCharacter::MoveMod);
	PlayerInputComponent->BindAction("MoveMod", IE_Released, this, &AFightingCharacter::StopMoveMod);

	PlayerInputComponent->BindAction("Taunt", IE_Pressed, this, &AFightingCharacter::Taunt);
	PlayerInputComponent->BindAction("Taunt", IE_Released, this, &AFightingCharacter::StopTaunt);

	
}

void AFightingCharacter::MoveForward(float Axis)
{
	if (!bDefeated && CanMove) {
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Axis);
	}
}

void AFightingCharacter::MoveRight(float Axis)
{
	if (!bDefeated  && CanMove){
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Axis);
	}
}

void AFightingCharacter::Turn(float Val)
{
	if (FollowCamera->IsActive()) {
		AddControllerYawInput(Val);
	}
}

void AFightingCharacter::LookUp(float Val)
{
	if (FollowCamera->IsActive()) {
		AddControllerPitchInput(Val);
	}
}

void AFightingCharacter::Run()
{
	bIsRunning = true;
}

void AFightingCharacter::StopRunning()
{
	bIsRunning = false;
}

void AFightingCharacter::JumpChecking()
{
	if (!bDefeated && CanJump_) {
		Jump();
	}
}

void AFightingCharacter::Attack1()
{
	//if(CanAddNextComboAttack) GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Green, TEXT("attack1"));
	if (!bDefeated && CanAttack && !(GetCharacterMovement()->IsFalling())) {
		if (CanAddNextComboAttack) {
			if (IsDucking) ComboSequenceStr = TEXT("0");
			else if (MoveModPressed && ComboSequenceStr.Equals(TEXT(""))) ComboSequenceStr = TEXT("3");
			else if (TauntPressed) { 
				if( get_random_float() <= 0.50) ComboSequenceStr = TEXT("5");
				else ComboSequenceStr = TEXT("55");
			}
			else ComboSequenceStr += TEXT("1");
			//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, ComboSequenceStr);
			CanAddNextComboAttack = false;
			CanMove = false;
			CanBlock = false;
			CanJump_ = false;
			CanDuck = false;

			Foot_R_Location = GetMesh()->GetSocketLocation("foot_r");
			Foot_L_Location = GetMesh()->GetSocketLocation("foot_l");
		}
		IsAttacking = true;
	}
}

void AFightingCharacter::StopAttack1()
{
	bAttack1 = false;
	IsAttacking = false;
}

void AFightingCharacter::Attack2()
{
	if (!bDefeated && CanAttack && !(GetCharacterMovement()->IsFalling())) {
		if (CanAddNextComboAttack) { 
			if (IsDucking) ComboSequenceStr = TEXT("0");
			else if (MoveModPressed && ComboSequenceStr.Equals(TEXT(""))) ComboSequenceStr = TEXT("4");
			else if (TauntPressed) {
				if (get_random_float() <= 0.80) ComboSequenceStr = TEXT("6");
				else ComboSequenceStr = TEXT("66");
			}
			else ComboSequenceStr += TEXT("2");
			CanAddNextComboAttack = false;
			CanMove = false;
			CanBlock = false;
			CanJump_ = false;
			CanDuck = false;

			Foot_R_Location = GetMesh()->GetSocketLocation("foot_r");
			Foot_L_Location = GetMesh()->GetSocketLocation("foot_l");
		}
		IsAttacking = true;
	}
}

void AFightingCharacter::StopAttack2()
{
	bAttack2 = false;
	IsAttacking = false;
}

void AFightingCharacter::Block()
{
	if (!bDefeated && CanBlock && !(GetCharacterMovement()->IsFalling())) {
		IsBlocking = true;
		CanMove = false;
		CanAttack = false;
		CanJump_ = false;
	}
}

void AFightingCharacter::StopBlocking()
{
	if (IsBlocking) {
		IsBlocking = false;
		CanMove = true;
		CanAttack = true;
		CanJump_ = true;
	}
}

void AFightingCharacter::Duck()
{
	if (!bDefeated && CanDuck && !(GetCharacterMovement()->IsFalling())) {
		IsDucking = true;
		CanMove = false;
		CanJump_ = false;
		CanBlock = false;
	}
}

void AFightingCharacter::StopDucking()
{
	if (IsDucking) {
		IsDucking = false;
		CanMove = true;
		CanJump_ = true;
		CanBlock = true;
	}
}

void AFightingCharacter::MoveMod()
{
	MoveModPressed = true;
}


void AFightingCharacter::StopMoveMod()
{
	MoveModPressed = false;
}

void AFightingCharacter::Taunt()
{
	TauntPressed = true;
}

void AFightingCharacter::StopTaunt()
{
	TauntPressed = false;
}

void AFightingCharacter::ChangeCamera()
{
	if (FollowCamera->IsActive()) {
		Camera2->Activate();
		FollowCamera->Deactivate();
	}
	else {
		FollowCamera->Activate();
		Camera2->Deactivate();
	}
}

void AFightingCharacter::SetTargetEnemy(AFightingCharacter* enemy) {
	TargetEnemy = enemy;
}

AFightingCharacter* AFightingCharacter::GetTargetEnemy() {
	return TargetEnemy;
}

float AFightingCharacter::GetHealthPoints() {
	return HealthPoints;
}

float AFightingCharacter::GetDamagePotential(FString bodyPart) {
	return DamagePotential[bodyPart];
}

float AFightingCharacter::GetWeaponVelocity(UPrimitiveComponent* WeaponComponent) {

	float velocity = 0.0;

	if (WeaponComponent == LeftFistCollisionBox) velocity = LeftFistVelocity;
	else if(WeaponComponent == RightFistCollisionBox) velocity = RightFistVelocity;
	else if (WeaponComponent == LeftFootCollisionBox || WeaponComponent == LeftLegCollisionBox) velocity = LeftFootVelocity;
	else if (WeaponComponent == RightFootCollisionBox || WeaponComponent == RightLegCollisionBox) velocity = RightFootVelocity;

	return velocity;
}


void AFightingCharacter::PunchAttackStart()
{
	if (LeftFistCollisionBox == NULL || RightFistCollisionBox == NULL) return;

	LeftFistCollisionBox->SetCollisionProfileName("Weapon");
	LeftFistCollisionBox->SetNotifyRigidBodyCollision(true);
	LeftFistCollisionBox->SetGenerateOverlapEvents(true);

	RightFistCollisionBox->SetCollisionProfileName("Weapon");
	RightFistCollisionBox->SetNotifyRigidBodyCollision(true);
	RightFistCollisionBox->SetGenerateOverlapEvents(true);

	bTrackFistsVelocity = true;
	LeftFistLastPos = LeftFistCollisionBox->GetComponentLocation();
	RightFistLastPos = RightFistCollisionBox->GetComponentLocation();

	RightFistVelocity_max = LeftFistVelocity_max = 0;
}

void AFightingCharacter::PunchAttackEnd()
{
	LeftFistCollisionBox->SetCollisionProfileName("NoCollision");
	LeftFistCollisionBox->SetNotifyRigidBodyCollision(false);
	LeftFistCollisionBox->SetGenerateOverlapEvents(false);

	RightFistCollisionBox->SetCollisionProfileName("NoCollision");
	RightFistCollisionBox->SetNotifyRigidBodyCollision(false);
	RightFistCollisionBox->SetGenerateOverlapEvents(false);

	bTrackFistsVelocity = false;

	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Cyan, FString::Printf(TEXT("RightFistVelocity_max: %f"), RightFistVelocity_max));
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, FString::Printf(TEXT("LeftFistVelocity_max: %f"), LeftFistVelocity_max));
}

void AFightingCharacter::KickAttackStart()
{
	if (LeftFootCollisionBox == NULL || RightFootCollisionBox == NULL || LeftLegCollisionBox == NULL || RightLegCollisionBox == NULL) return;

	LeftFootCollisionBox->SetCollisionProfileName("Weapon");
	LeftFootCollisionBox->SetNotifyRigidBodyCollision(true);
	LeftFootCollisionBox->SetGenerateOverlapEvents(true);

	RightFootCollisionBox->SetCollisionProfileName("Weapon");
	RightFootCollisionBox->SetNotifyRigidBodyCollision(true);
	RightFootCollisionBox->SetGenerateOverlapEvents(true);

	LeftLegCollisionBox->SetCollisionProfileName("Weapon");
	RightLegCollisionBox->SetCollisionProfileName("Weapon");

	bTrackFootsVelocity = true;
	LeftFootLastPos = LeftFootCollisionBox->GetComponentLocation();
	RightFootLastPos = RightFootCollisionBox->GetComponentLocation();

	RightFootVelocity_max = LeftFootVelocity_max = 0;
}

void AFightingCharacter::KickAttackEnd()
{
	LeftFootCollisionBox->SetCollisionProfileName("NoCollision");
	LeftFootCollisionBox->SetNotifyRigidBodyCollision(false);
	LeftFootCollisionBox->SetGenerateOverlapEvents(false);

	RightFootCollisionBox->SetCollisionProfileName("NoCollision");
	RightFootCollisionBox->SetNotifyRigidBodyCollision(false);
	RightFootCollisionBox->SetGenerateOverlapEvents(false);

	LeftLegCollisionBox->SetCollisionProfileName("DamageBox");
	RightLegCollisionBox->SetCollisionProfileName("DamageBox");

	bTrackFootsVelocity = false;

	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Green, FString::Printf(TEXT("RightFootVelocity_max: %f"), RightFootVelocity_max));
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Orange, FString::Printf(TEXT("LeftFootVelocity_max: %f"), LeftFootVelocity_max));
}

void AFightingCharacter::ReactionStart(AActor* attacker, UPrimitiveComponent* CollisionBox, float ImpactVel, FVector ImpactPoint, FString AttackName) 
{
	Foot_R_Location = GetMesh()->GetSocketLocation("foot_r");
	Foot_L_Location = GetMesh()->GetSocketLocation("foot_l");

	FVector BoxCentre = CollisionBox->GetComponentLocation();
	FString CollisionBoxName = CollisionBox->GetName();
	FString hitArea = DamageCBCategory[CollisionBoxName];

	FVector CentreToImpact = ImpactPoint - BoxCentre;

	float cosForward = GetActorForwardVector().CosineAngle2D(CentreToImpact);
	float cosRight = GetActorRightVector().CosineAngle2D(CentreToImpact);

	bool front, right, left, back, small_hit, medium_hit, big_hit;
	front = cosForward > 0.707;
	back = cosForward < -0.707;
	right = cosRight > 0.707;
	left = cosRight < -0.707;

	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, FString::Printf(TEXT("impact vel: %f"), ImpactVel));
	if (ImpactVel < 400) small_hit = true;
	else if (ImpactVel > 800) big_hit = true;
	else medium_hit = true;

	/*if (hitArea.Equals(TEXT("chest"))) {
		if (ImpactPoint.Z > BoxCentre.Z) hitArea = TEXT("head");
		else hitArea = TEXT("torso");
	}
	if (hitArea.Equals(TEXT("torso"))) {
		if (back) Reaction = ReactType::Back;
		else if (front) {
			if (small_hit) Reaction = ReactType::Torso_FS;
			else if(medium_hit) Reaction = ReactType::Torso_FM;
			else if (big_hit) Reaction = ReactType::Torso_FB;
		} 
		else if (right) {
			if (small_hit) Reaction = ReactType::Torso_RS;
			else if (medium_hit) Reaction = ReactType::Torso_RM;
			else if (big_hit) Reaction = ReactType::Torso_RB;
		}
		else if (left) {
			if (small_hit) Reaction = ReactType::Torso_LS;
			else if (medium_hit) Reaction = ReactType::Torso_LM;
			else if (big_hit) Reaction = ReactType::Torso_LB;
		}
	}
	else if (hitArea.Equals(TEXT("head"))) {
		if (back) Reaction = ReactType::Back;
		else if (front) {
			if (small_hit) Reaction = ReactType::Face_FS;
			else if (medium_hit) Reaction = ReactType::Face_FM;
			else if (big_hit) Reaction = ReactType::Face_FB;
		}
		else if (right) {
			if (small_hit) Reaction = ReactType::Face_RS;
			else if (medium_hit) Reaction = ReactType::Face_RM;
			else if (big_hit) Reaction = ReactType::Face_RB;
		}
		else if (left) {
			if (small_hit) Reaction = ReactType::Face_LS;
			else if (medium_hit) Reaction = ReactType::Face_LM;
			else if (big_hit) Reaction = ReactType::Face_LB;
		}
	}*/

	FVector actorToAttacker = attacker->GetActorLocation() - GetActorLocation();

	float cos = GetActorForwardVector().CosineAngle2D(actorToAttacker);

	bool isAttackerBehindActor = cos < -0.35;

	if (hitArea.Equals(TEXT("head"))) {
		if (isAttackerBehindActor) Reaction = ReactType::Back;
		else if (AttackName.Equals(TEXT("Attack_Duck_Punch")) || AttackName.Equals(TEXT("Attack_Punch_L_quick")) 
			|| AttackName.Equals(TEXT("Attack_Punch_Combo"))) {
			Reaction = ReactType::Face_FS;
		}
		else if ( AttackName.Equals(TEXT("Attack_Punch_R_quick"))) {
			Reaction = ReactType::Face_FM;
		}
		else if (AttackName.Equals(TEXT("Attack_Kick_scissors")) || AttackName.Equals(TEXT("Attack_Punch_L_uppercut")) || AttackName.Equals(TEXT("Attack_Punch_R_uppercut"))) {
			Reaction = ReactType::Face_FB;
		}
		else if (AttackName.Equals(TEXT("Attack_Kick_backwards_round"))) {
			Reaction = ReactType::Face_RB;
		}
		else if (AttackName.Equals(TEXT("Attack_Kick_R_high")) || AttackName.Equals(TEXT("Attack_Kick_R_roundhouse"))) {
			Reaction = ReactType::Face_LM;
		}
		else if (AttackName.Equals(TEXT("Attack_Kick_R_high_round")) || AttackName.Equals(TEXT("Attack_Punch_R_swing"))) {
			Reaction = ReactType::Face_LB;
		}
	}
	else if (hitArea.Equals(TEXT("torso"))) {
		if (isAttackerBehindActor) Reaction = ReactType::Back;
		else if (AttackName.Equals(TEXT("Attack_Kick_R_front")) || AttackName.Equals(TEXT("Attack_Kick_L_front"))) {
			Reaction = ReactType::Torso_FS;
		}
		else if (AttackName.Equals(TEXT("Attack_Kick_R_torso"))) {
			Reaction = ReactType::Torso_FM;
		}
		else if (AttackName.Equals(TEXT("Attack_Punch_L_uppercut")) || AttackName.Equals(TEXT("Attack_Punch_R_uppercut"))) {
			Reaction = ReactType::Torso_FB;
		}
		else if (AttackName.Equals(TEXT("Attack_Punch_R_hook"))) {
			Reaction = ReactType::Torso_LS;
		}
		else if (AttackName.Equals(TEXT("Attack_Kick_air")) //|| AttackName.Equals(TEXT("Attack_Kick_R_roundhouse")) || AttackName.Equals(TEXT("Attack_Kick_scissors"))
			|| AttackName.Equals(TEXT("Attack_Kick_L_roundhouse")) || AttackName.Equals(TEXT("Attack_Kick_R_high"))
			|| AttackName.Equals(TEXT("Attack_Punch_R_hook_momentum")) || AttackName.Equals(TEXT("Attack_Kick_R_mocap"))) {
			Reaction = ReactType::Torso_LM;
		}
		else if (AttackName.Equals(TEXT("Attack_Punch_L_hook"))) {
			Reaction = ReactType::Torso_RM;
		}

	}
	else if (hitArea.Equals(TEXT("chest"))) {
		if (isAttackerBehindActor) Reaction = ReactType::Back;
		else if (AttackName.Equals(TEXT("Attack_Kick_air")) || AttackName.Equals(TEXT("Attack_Punch_Combo")) 
			|| AttackName.Equals(TEXT("Attack_Punch_R_hook")) || AttackName.Equals(TEXT("Attack_Punch_R_hook_momentum"))
			|| AttackName.Equals(TEXT("Attack_Kick_L_roundhouse")) || AttackName.Equals(TEXT("Attack_Kick_R_mocap"))) {
			Reaction = ReactType::Torso_LM;
		}
		else if (AttackName.Equals(TEXT("Attack_Punch_L_hook"))) {
			Reaction = ReactType::Torso_RM;
		}
	}

	CanMove = false;
	CanJump_ = false;
	CanDuck = false;
	CanBlock = false;
	CanAttack = false;
}

void AFightingCharacter::ReactionEnd() {
	Reaction = ReactType::NoReact;
	CanMove = true;
	CanJump_ = true;
	CanDuck = true;
	CanBlock = true;
	CanAttack = true;
}

void AFightingCharacter::InflictDamage(UPrimitiveComponent* CollisionBox, float ImpactVel)
{
	FString hit_area = DamageCBCategory[CollisionBox->GetName()];
	if (hit_area.IsEmpty()) return;
	if (hit_area.Equals(TEXT("chest"))) hit_area = TEXT("torso");
	float current_time = GetWorld()->GetTimeSeconds();
	if (current_time - LastDamageTakenTime[hit_area] > 0.5) {
		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, FString::Printf(TEXT("Hit Area: %s (%s)"), *hit_area, *CollisionBox->GetName()));
		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Cyan, FString::Printf(TEXT("ImpactVel: %f "), ImpactVel)); 
		
		float base_damage = BaseDamage[hit_area];
		float damage_multiplier = DamagePotential[hit_area];
		float damage_taken = base_damage * damage_multiplier * ImpactVel/800;
		HealthPoints -= damage_taken;
		if (HealthPoints < 0) HealthPoints = 0;
		DamagePotential[hit_area] += PotentialIncrement;
		if (DamagePotential[hit_area] > 3) DamagePotential[hit_area] = 3;
		LastDamageTakenTime[hit_area] = current_time;

		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, FString::Printf(TEXT("damage taken: %d, DamagePotential: %f, HP: %d "), (int)(damage_taken * 1000), DamagePotential[hit_area], (int)(HealthPoints * 1000)));
	
		if (hit_area.Equals(TEXT("torso"))) HitTorso = true;
		else if (hit_area.Equals(TEXT("head"))) HitHead = true;
		else if (hit_area.Equals(TEXT("left_arm"))) HitArmL = true;
		else if (hit_area.Equals(TEXT("right_arm"))) HitArmL = true;
		else if (hit_area.Equals(TEXT("left_leg"))) HitLegL = true;
		else if (hit_area.Equals(TEXT("right_Leg"))) HitLegR = true;
	}
}

void AFightingCharacter::OnAttackHit(UPrimitiveComponent * HitComponent, AActor * OtherActor, UPrimitiveComponent * OtherComp, FVector NormalImpulse, const FHitResult & Hit)
{
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, __FUNCTION__);

	FVector impact_point = Hit.ImpactPoint;
	float dist = Hit.Distance;
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, FString::Printf(TEXT("impact_point: %f, %f, %f"), impact_point.X, impact_point.Y, impact_point.Z));
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, FString::Printf(TEXT("dist: %f"), dist));

	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, Hit.GetActor()->GetName());

}

void AFightingCharacter::OnAttackOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Blue, __FUNCTION__);
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, OtherActor->GetName());
	if (OtherActor != this && OtherActor != NULL) {
		if (AFightingCharacter* enemy = Cast<AFightingCharacter>(OtherActor)) {
			FString hitArea = DamageCBCategory[OtherComp->GetName()];

			// SweepResult is unpopulated for OnOverlapBegin - a bug from Unreal. Solution to get the HitResult:
		    // https://answers.unrealengine.com/questions/165523/on-component-begin-overlap-sweep-result-not-popula.html

			TArray<FHitResult> AllResults;

			FHitResult Hit;

			// Get the location of this actor's overlapped component
			auto Start = OverlappedComponent->GetComponentLocation();
			// Get the location of the other component
			auto End = OtherComp->GetComponentLocation();
			// Use a slightly larger radius to ensure we find the same result
			auto CollisionRadius = FVector::Dist(Start, End) * 1.1f;

			// Now do a spherical sweep to find the overlap
			GetWorld()->SweepMultiByObjectType(AllResults, Start, End, FQuat::Identity, 0, FCollisionShape::MakeSphere(CollisionRadius),
				FCollisionQueryParams::FCollisionQueryParams(false) );

			// Finally check which hit result is the one from this event
			for (auto HitResult : AllResults)
			{
				if (OtherComp->GetUniqueID() == HitResult.GetComponent()->GetUniqueID()) {
					// A component with the same UniqueID means we found our overlap!
					Hit = HitResult;
					break;
				}
			}
			/************************************/

			enemy->IsDamageBoxOverlapping[OtherComp->GetName()] = true;
			//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Blue, FString::Printf(TEXT("%s is overlapping"), *OtherComp->GetName()));
			enemy->InflictDamage(OtherComp, GetWeaponVelocity(OverlappedComponent));
			
			enemy->ImpactVelocity = GetWeaponVelocity(OverlappedComponent);
			enemy->ImpactDirection = GetActorForwardVector();

			if(GetCurrentMontage() != NULL) enemy->ReactionStart(this, OtherComp, GetWeaponVelocity(OverlappedComponent), Hit.ImpactPoint, GetCurrentMontage()->GetName());
		}
	}
}


void AFightingCharacter::OnAttackOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, __FUNCTION__);
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, OtherActor->GetName());

	if (OtherActor != this && OtherActor != NULL) {
		if (AFightingCharacter* enemy = Cast<AFightingCharacter>(OtherActor)) {
			enemy->IsDamageBoxOverlapping[OtherComp->GetName()] = false;
		}
	}
}

void AFightingCharacter::RotateToTarget(float DeltaTime) {

	float speed = GetVelocity().Size();
	if (TargetEnemy != NULL && speed != 0) {
		FVector TargetLocation = TargetEnemy->GetActorLocation();
		TargetLocation.Z = GetActorLocation().Z;
		FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), TargetLocation);
		
		FRotator NextRotation = FMath::RInterpTo(GetActorRotation(), LookAt, DeltaTime, 2.0f);

		SetActorRotation(NextRotation);
	}
	
}

float AFightingCharacter::GetSpeedForAnimation(float delta_time)
{
	float max_accel = GetCharacterMovement()->MaxAcceleration + 20;
	float anim_accel = GetCharacterMovement()->MaxAcceleration;
	
	FVector velocity = FVector(GetVelocity().X, GetVelocity().Y, 0.0f);
	float actual_speed = velocity.Size();
	float accel = (actual_speed - speedForAnimation)/delta_time;

	if (abs(accel) > max_accel) {
		if (accel < 0) anim_accel = -anim_accel;

		speedForAnimation += anim_accel * delta_time;
	}
	else
	{
		speedForAnimation = actual_speed;
	}

	float cosAngle = GetActorForwardVector().CosineAngle2D(GetVelocity());

	if (cosAngle < 0) return -speedForAnimation;

	return speedForAnimation;
}

FVector AFightingCharacter::GetTargetSocketLocation(FName SocketName)
{
	FVector TargetLocation(0.0, 0.0, -100.0); // means no target

	// get enemy socket location
	if (TargetEnemy != NULL && !SocketName.IsNone()) {
		
		FVector SocketLocation = TargetEnemy->GetMesh()->GetSocketLocation(SocketName);
		
		FVector VectorToTarget = SocketLocation - GetActorLocation();
		float distance = VectorToTarget.Size();
		
		// check actor is close to enemy
		if (distance < 200.0) {
			float cosAngle = GetActorForwardVector().CosineAngle2D(VectorToTarget);

			// check angle to see if actor is facing enemy
			//if so return socket location
			if (cosAngle > 0.75) {
				TargetLocation = SocketLocation;
			}
		}

		/*if (this == UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) {
			GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, FString::Printf(TEXT("SocketName: %s"), *SocketName.ToString()));
			GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, FString::Printf(TEXT("TargetLocation.x: %f, y: %f, z: %f"), TargetLocation.X, TargetLocation.Y, TargetLocation.Z));
		}*/
	}
	return TargetLocation;
}


FVector AFightingCharacter::GetFootRLocation() {
	return Foot_R_Location;
}

FVector AFightingCharacter::GetFootLLocation() {
	return Foot_L_Location;
}

void AFightingCharacter::ClearComboSequence()
{
	ComboSequenceStr = TEXT("");
	CanAddNextComboAttack = true;
	CanMove = true;
	CanBlock = true;
	CanJump_ = true;
	CanDuck = true;
	CanAttack = true;
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Orange, TEXT("CLEARED"));
}


void AFightingCharacter::CollisionBoxesInit() 
{
	CollisionBoxes = CreateDefaultSubobject<USceneComponent>(TEXT("CollisionBoxes"));
	
		
	/** Weapon Collision Boxes**/

	RightFistCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightFistCollisionBox"));
	WeaponCollisionBoxes.push_back(RightFistCollisionBox);
	WeaponCBCategory[RightFistCollisionBox->GetName()] = "punch";

	LeftFistCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftFistCollisionBox"));
	WeaponCollisionBoxes.push_back(LeftFistCollisionBox);
	WeaponCBCategory[LeftFistCollisionBox->GetName()] = "punch";

	RightFootCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightFootCollisionBox"));
	WeaponCollisionBoxes.push_back(RightFootCollisionBox);
	WeaponCBCategory[RightFootCollisionBox->GetName()] = "kick";

	LeftFootCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftFootCollisionBox"));
	WeaponCollisionBoxes.push_back(LeftFootCollisionBox);
	WeaponCBCategory[LeftFootCollisionBox->GetName()] = "kick";

	/** Damage Collision Boxes**/

	HeadCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HeadCollisionBox"));
	DamageCollisionBoxes.push_back(HeadCollisionBox);
	DamageCBCategory[HeadCollisionBox->GetName()] = "head";

	ChestCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ChestCollisionBox"));
	DamageCollisionBoxes.push_back(ChestCollisionBox);
	DamageCBCategory[ChestCollisionBox->GetName()] = "chest";

	TorsoCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TorsoCollisionBox"));
	DamageCollisionBoxes.push_back(TorsoCollisionBox);
	DamageCBCategory[TorsoCollisionBox->GetName()] = "torso";

	HipsCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HipsCollisionBox"));
	DamageCollisionBoxes.push_back(HipsCollisionBox);
	DamageCBCategory[HipsCollisionBox->GetName()] = "torso";

	RightArmCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightArmCollisionBox"));
	DamageCollisionBoxes.push_back(RightArmCollisionBox);
	DamageCBCategory[RightArmCollisionBox->GetName()] = "right_arm";

	RightForearmCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightForearmCollisionBox"));
	DamageCollisionBoxes.push_back(RightForearmCollisionBox);
	DamageCBCategory[RightForearmCollisionBox->GetName()] = "right_arm";

	LeftArmCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftArmCollisionBox"));
	DamageCollisionBoxes.push_back(LeftArmCollisionBox);
	DamageCBCategory[LeftArmCollisionBox->GetName()] = "left_arm";

	LeftForearmCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftForearmCollisionBox"));
	DamageCollisionBoxes.push_back(LeftForearmCollisionBox);
	DamageCBCategory[LeftForearmCollisionBox->GetName()] = "left_arm";

	RightThighCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightThighCollisionBox"));
	DamageCollisionBoxes.push_back(RightThighCollisionBox);
	DamageCBCategory[RightThighCollisionBox->GetName()] = "right_leg";

	RightLegCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightLegCollisionBox"));
	DamageCollisionBoxes.push_back(RightLegCollisionBox);
	WeaponCollisionBoxes.push_back(RightLegCollisionBox);
	DamageCBCategory[RightLegCollisionBox->GetName()] = "right_leg";
	WeaponCBCategory[RightLegCollisionBox->GetName()] = "kick";

	LeftThighCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftThighCollisionBox"));
	DamageCollisionBoxes.push_back(LeftThighCollisionBox);
	DamageCBCategory[LeftThighCollisionBox->GetName()] = "left_leg";

	LeftLegCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftLegCollisionBox"));
	DamageCollisionBoxes.push_back(LeftLegCollisionBox);
	WeaponCollisionBoxes.push_back(LeftLegCollisionBox);
	DamageCBCategory[LeftLegCollisionBox->GetName()] = "left_leg";
	WeaponCBCategory[LeftLegCollisionBox->GetName()] = "kick";
	

	for (UBoxComponent* element : WeaponCollisionBoxes)
	{
		element->SetupAttachment(CollisionBoxes);
		element->SetCollisionProfileName("NoCollision");
		element->SetNotifyRigidBodyCollision(false);
		element->SetHiddenInGame(false);
	}

	for (UBoxComponent* element : DamageCollisionBoxes)
	{
		element->SetupAttachment(CollisionBoxes);
		element->SetCollisionProfileName("DamageBox");
		element->SetNotifyRigidBodyCollision(true);
		element->SetHiddenInGame(false);
		IsDamageBoxOverlapping[element->GetName()] = false;
	}

}

void AFightingCharacter::AttachCollisionBoxesToSockets()
{
	// attach collision components to sockets based on transformations definitions
	const FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);

	LeftFistCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "fist_l_collision");
	RightFistCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "fist_r_collision");
	RightFootCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "foot_r_collision");
	LeftFootCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "foot_l_collision");
	HeadCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "head_collision");
	ChestCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "chest_collision");
	TorsoCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "torso_collision");
	HipsCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "hips_collision");
	RightArmCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "upperarm_r_collision");
	RightForearmCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "forearm_r_collision");
	LeftArmCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "upperarm_l_collision");
	LeftForearmCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "forearm_l_collision");
	RightThighCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "thigh_r_collision");
	RightLegCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "leg_r_collsion");
	LeftThighCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "thigh_l_collision");
	LeftLegCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "leg_l_collsion");
}

void AFightingCharacter::VariablesInit()
{
	DamageCBCategories.push_back("head");
	DamageCBCategories.push_back("torso");
	DamageCBCategories.push_back("right_arm");
	DamageCBCategories.push_back("left_arm");
	DamageCBCategories.push_back("right_leg");
	DamageCBCategories.push_back("left_leg");
	
	float current_time = GetWorld()->GetTimeSeconds();

	for (FString cat : DamageCBCategories) {
		DamagePotential[cat] = 1.0;
		LastDamageTakenTime[cat] = current_time;
	}

	BaseDamage["head"] = 0.02;
	BaseDamage["torso"] = 0.01;
	BaseDamage["right_arm"] = 0.005;
	BaseDamage["left_arm"] = 0.005;
	BaseDamage["right_leg"] = 0.005;
	BaseDamage["left_leg"] = 0.005;
}

FVector AFightingCharacter::GetEnemyLocation() {
	FVector target_location;
	if(TargetEnemy != NULL) target_location = TargetEnemy->GetActorLocation();
	return target_location;
}

float get_random_float()
{
	static std::default_random_engine e;
	static std::uniform_real_distribution<> dis(0, 1); // rage 0 - 1
	return dis(e);
}
