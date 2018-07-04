/*
* Copyright (c) 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#pragma once

#include "DomainRandomizationDNNPCH.h"
#include "DRUtils.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNVDRUtils, Log, All)

USTRUCT(BlueprintType)
struct DOMAINRANDOMIZATIONDNN_API FRandomRotationData
{
    GENERATED_BODY()

public:
    FRandomRotationData();

    // Get a random rotation from the constrained data
    FRotator GetRandomRotation() const;

    // Get a random rotation related to (the constrained data is applied around) a fixed rotation
    FRotator GetRandomRotationRelative(const FRotator& BaseRotation) const;

    bool ShouldRandomized() const
    {
        return bRandomizeRotationInACone || bRandomizePitch || bRandomizeRoll || bRandomizeYaw;
    }

protected: // Editor properties
    // If true, generate random rotation inside a cone
    UPROPERTY(EditAnywhere, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bRandomizeRotationInACone;

    // Half of the angle (in degree) of the cone where we want to generate the rotation inside
    UPROPERTY(EditAnywhere, meta = (EditCondition = bRandomizeRotationInACone))
    float RandomConeHalfAngle;

    UPROPERTY(EditAnywhere, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bRandomizePitch;

    // The range of pitch angle (in degree)
    UPROPERTY(EditAnywhere, meta = (EditCondition = bRandomizePitch))
    FFloatInterval PitchRange;

    UPROPERTY(EditAnywhere, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bRandomizeRoll;

    // The range of roll angle (in degree)
    UPROPERTY(EditAnywhere, meta = (EditCondition = bRandomizeRoll))
    FFloatInterval RollRange;

    UPROPERTY(EditAnywhere, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bRandomizeYaw;

    // The range of yaw angle (in degree)
    UPROPERTY(EditAnywhere, meta = (EditCondition = bRandomizeYaw))
    FFloatInterval YawRange;
};

USTRUCT(BlueprintType)
struct DOMAINRANDOMIZATIONDNN_API FRandomLocationData
{
    GENERATED_BODY()
public:
    FRandomLocationData();

    // Get a random location from the constrained data
    FVector GetRandomLocation() const;

    // Get a random location related to (the constrained data is applied around) a fixed location
    FVector GetRandomLocationRelative(const FVector& BaseLocation) const;

    // Get a random location in an object's local space
    FVector GetRandomLocationInLocalSpace(const FTransform& ObjectTransform) const;

    bool ShouldRandomized() const
    {
        return bRandomizeXAxis || bRandomizeYAxis || bRandomizeZAxis;
    }

public:
    // If true, the location can be along X axis in world space
    UPROPERTY(EditAnywhere, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bRandomizeXAxis;

    // The range of location in the X axis (in world space)
    UPROPERTY(EditAnywhere, meta = (EditCondition = bRandomizeXAxis))
    FFloatInterval XAxisRange;

    // If true, the location can be along Y axis in world space
    UPROPERTY(EditAnywhere, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bRandomizeYAxis;

    // The range of location in the Y axis (in world space)
    UPROPERTY(EditAnywhere, meta = (EditCondition = bRandomizeYAxis))
    FFloatInterval YAxisRange;

    // If true, the location can be along Z axis in world space
    UPROPERTY(EditAnywhere, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bRandomizeZAxis;

    // The range of location in the Y axis (in world space)
    UPROPERTY(EditAnywhere, meta = (EditCondition = bRandomizeZAxis))
    FFloatInterval ZAxisRange;
};

USTRUCT(BlueprintType)
struct DOMAINRANDOMIZATIONDNN_API FRandomScale3DData
{
    GENERATED_BODY()

public:
    FRandomScale3DData();

    // Get a random 3d scale from the constrained data
    FVector GetRandomScale3D() const;

    bool ShouldRandomized() const
    {
        return bUniformScale || bRandomizeXAxis || bRandomizeYAxis || bRandomizeZAxis;
    }

public:
    // If true, all the axes will use the same scale value
    // Otherwise, the actor can have different scales in different axis
    // NOTE: If this is true, the scale will be chosen in UniformScaleRange and the separated axis scale range will be ignored
    UPROPERTY(EditAnywhere, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bUniformScale;

    UPROPERTY(EditAnywhere, meta = (EditCondition = bUniformScale))
    FFloatInterval UniformScaleRange;

    // If true, the actor can be scaled along the X axis in world space
    UPROPERTY(EditAnywhere, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bRandomizeXAxis;

    // The range of scale in the X axis (in world space)
    // NOTE: Minimum value is set at 0.001
    UPROPERTY(EditAnywhere, meta = (EditCondition = bRandomizeXAxis))
    FFloatInterval XAxisRange;

    // If true, the actor can be scaled along the Y axis in world space
    UPROPERTY(EditAnywhere, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bRandomizeYAxis;

    // The range of scale in the Y axis (in world space)
    // NOTE: Minimum value is set at 0.001
    UPROPERTY(EditAnywhere, meta = (EditCondition = bRandomizeYAxis))
    FFloatInterval YAxisRange;

    // If true, the actor can be scaled along the Z axis in world space
    UPROPERTY(EditAnywhere, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bRandomizeZAxis;

    // The range of scale in the Z axis (in world space)
    // NOTE: Minimum value is set at 0.001
    UPROPERTY(EditAnywhere, meta = (EditCondition = bRandomizeZAxis))
    FFloatInterval ZAxisRange;
};

UENUM()
enum class ERandomColorType : uint8
{
    RandomizeAllColor = 0,
    RandomizeBetweenTwoColors,
    RandomizeAroundAColor,
    RandomColorType_MAX UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct DOMAINRANDOMIZATIONDNN_API FRandomColorData
{
    GENERATED_BODY()

public:
    FRandomColorData();

#if WITH_EDITORONLY_DATA
    void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent);
#endif //WITH_EDITORONLY_DATA

    FLinearColor GetRandomColor() const;

    static FLinearColor GetRandomAnyColor();
    static FLinearColor GetRandomColorInRange(const FLinearColor& Color1, const FLinearColor& Color2, const bool& bRandomizeInHSV);
    static FLinearColor GetRandomColorAround(const FLinearColor& BaseColor, const float& HueDelta, const float& SaturationDelta, const float& ValueDelta);

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ERandomColorType RandomizationType;

    UPROPERTY(EditAnywhere, Category = Randomization, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bRandomizeBetweenTwoColors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "bRandomizeBetweenTwoColors"))
    FLinearColor FirstColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "bRandomizeBetweenTwoColors"))
    FLinearColor SecondColor;

    // If true, the color will be chosen randomly in the HSV value instead of the RGB one
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "bRandomizeBetweenTwoColors"))
    bool bRandomizeInHSV;

    UPROPERTY(EditAnywhere, Category = Randomization, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bRandomizeAroundAColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "bRandomizeAroundAColor"))
    FLinearColor MainColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "bRandomizeAroundAColor"))
    float MaxHueChange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "bRandomizeAroundAColor"))
    float MaxSaturationChange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "bRandomizeAroundAColor"))
    float MaxValueChange;
};

UENUM()
enum class EMaterialSelectionType : uint8
{
    // Modify all the materials in the mesh
    ModifyAllMaterials = 0,

    // Modify all the materials in the MaterialIndexes list
    ModifyMaterialsInIndexesList,

    // Modify all the materials in the MaterialSlotNames list
    ModifyMaterialInSlotNamesList,

    MaterialSelectionType_MAX UMETA(Hidden)
};

// FRandomMaterialSelection represent how to select materials from a mesh to modify
USTRUCT(BlueprintType)
struct DOMAINRANDOMIZATIONDNN_API FRandomMaterialSelection
{
    GENERATED_BODY()

public:
    FRandomMaterialSelection();

#if WITH_EDITORONLY_DATA
    void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent);
#endif //WITH_EDITORONLY_DATA

    // Find the indexes of all the material we want to modify in a mesh
    TArray<int32> GetAffectMaterialIndexes(const class UMeshComponent* MeshComp) const;

public: // Editor properties
    // Decide how to select materials from the owner's mesh to modify
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization)
    EMaterialSelectionType MaterialSelectionType;

    UPROPERTY(EditAnywhere, Category = Randomization, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bSelectMaterialByIndexes;

    UPROPERTY(EditAnywhere, Category = Randomization, meta = (PinHiddenByDefault, InlineEditConditionToggle))
    bool bSelectMaterialBySlotNames;

    // List of indexes of materials in the owner mesh to modify
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "bSelectMaterialByIndexes"))
    TArray<int32> MaterialIndexes;

    // List of names of materials in the owner mesh to modify
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Randomization, meta = (EditCondition = "bSelectMaterialBySlotNames"))
    TArray<FName> MaterialSlotNames;
};

USTRUCT(BlueprintType)
struct DOMAINRANDOMIZATIONDNN_API FRandUtils
{
    GENERATED_BODY()

public:
    static float RandGaussian(const float mean, const float variance);
    static FVector2D RandGaussian2D(const float mean, const float variance);

};

// Utility functions
namespace DRUtils
{
    // Get the first valid mesh component in an actor
    extern UMeshComponent* GetFirstValidMeshComponent(AActor* OwnerActor);

    // Get the valid mesh components in an actor
    extern TArray<UMeshComponent*> GetValidChildMeshComponents(AActor* OwnerActor);
}

// This struct manage a large amount numbers of assets by
USTRUCT(BlueprintType)
struct DOMAINRANDOMIZATIONDNN_API FRandomAssetStreamer
{
    GENERATED_BODY()

public:
    FRandomAssetStreamer();
    FRandomAssetStreamer(const FRandomAssetStreamer& OtherStreamer);
    ~FRandomAssetStreamer();

    FRandomAssetStreamer& operator= (const FRandomAssetStreamer& OtherStreamer);

    void Init(const TArray<FDirectoryPath>& InAssetDirectories, UClass* InAssetClass);
    void ScanPath();

    bool HasAssets() const;
    FSoftObjectPath GetNextAssetReference();

    template <typename AssetClassType>
    AssetClassType* GetNextAsset()
    {
        FSoftObjectPath NextAssetRef = GetNextAssetReference();
        return (NextAssetRef.IsValid()
                ? Cast<AssetClassType>(NextAssetRef.ResolveObject())
                : nullptr);
    }

    bool IsLoadingAssets() const;

protected:
    void LoadNextBatch(bool bAsyncLoad = true);
    void OnAssetBatchLoaded();

private:

    struct FRandomAssetStreamerCallback
    {
    public:
        FRandomAssetStreamer* AssetStreamer;

        void Callback()
        {
            if (AssetStreamer)
            {
                AssetStreamer->OnAssetBatchLoaded();
            }
        }
    };

protected: // Transient properties
    // Path to the directory where we want to get the assets from
    UPROPERTY(Transient)
    TArray<FDirectoryPath> AssetDirectories;

    // Class of assets to get
    UPROPERTY(Transient)
    UClass* ManagedAssetClass;

    // List of all the assets in the managed directory
    UPROPERTY(Transient)
    TArray<FSoftObjectPath> AllAssetReferences;

    // Circular queue of loaded assets
    UPROPERTY(Transient)
    TArray<FSoftObjectPath> LoadedAssetReferences;

    // List of assets is streaming in
    UPROPERTY(Transient)
    TArray<FSoftObjectPath> LoadingAssetReferences;

    UPROPERTY(Transient)
    int32 LastLoadedAssetIndex;

    UPROPERTY(Transient)
    int32 LastUsedAssetIndex;

    UPROPERTY(Transient)
    int32 UnusedAssetCount;

    FStreamableManager AssetStreamer;

    TSharedPtr<FStreamableHandle> StreamableHandlePtr;
    TSharedPtr<FRandomAssetStreamerCallback> StreamerCallbackPtr;
};

// This enum is used by random material components to select which components it should modify material
UENUM()
enum class EAffectedMaterialOwnerComponentType : uint8
{
    OnlyAffectMeshComponents = 0,
    OnlyAffectDecalComponents,
    AffectBothMeshAndDecalComponents,
    AffectedMaterialOwnerComponentType_MAX UMETA(Hidden)
};