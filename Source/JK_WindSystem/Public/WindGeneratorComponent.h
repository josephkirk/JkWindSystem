// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "WindGeneratorComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class JK_WINDSYSTEM_API UWindGeneratorComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UWindGeneratorComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Wind")
    virtual FVector GetWindVelocityAtLocation(const FVector& Location) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
    float Strength = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wind")
    float Radius = 500.0f;

protected:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Wind")
    virtual float GetFalloff(float Distance) const;
};
