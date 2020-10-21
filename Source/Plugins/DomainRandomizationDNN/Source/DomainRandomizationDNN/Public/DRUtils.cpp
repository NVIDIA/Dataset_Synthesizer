/*
* Copyright (c) 2018 NVIDIA Corporation. All rights reserved.
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/

#include "DomainRandomizationDNNPCH.h"
#include "DRUtils.h"
#include "Engine/AssetManager.h"
#if WITH_EDITORONLY_DATA
#include "AssetRegistryModule.h"
#endif // WITH_EDITORONLY_DATA

DEFINE_LOG_CATEGORY(LogNVDRUtils);
//=================================== FRandomRotationData ===================================
FRandomRotationData::FRandomRotationData()
{
    bRandomizeRotationInACone = true;
    RandomConeHalfAngle = 45.f;

    bRandomizePitch = false;
    PitchRange = FFloatInterval(-180.f, 180.f);

    bRandomizeRoll = false;
    RollRange = FFloatInterval(-180.f, 180.f);

    bRandomizeYaw = true;
    YawRange = FFloatInterval(-180.f, 180.f);
}

FRotator FRandomRotationData::GetRandomRotation() const
{
    FRotator RandomRotation = FRotator::ZeroRotator;

    if (bRandomizeYaw)
    {
        RandomRotation.Yaw = FMath::RandRange(YawRange.Min, YawRange.Max);
    }
    if (bRandomizeRoll)
    {
        RandomRotation.Roll = FMath::RandRange(RollRange.Min, RollRange.Max);
    }
    if (bRandomizePitch)
    {
        RandomRotation.Pitch = FMath::RandRange(PitchRange.Min, PitchRange.Max);
    }

    return RandomRotation;
}

FRotator FRandomRotationData::GetRandomRotationRelative(const FRotator& BaseRotation) const
{
    if (bRandomizeRotationInACone)
    {
        const FVector& BaseDir = BaseRotation.Vector();
        const float ConeHalfAngleRad = FMath::DegreesToRadians(RandomConeHalfAngle);

        FRotator RandomRotation = FMath::VRandCone(BaseDir, ConeHalfAngleRad).Rotation();
        return RandomRotation;
    }

    FRotator RandomRotation = GetRandomRotation();
    return BaseRotation + RandomRotation;
}

//=================================== FRandomLocationData ===================================
FRandomLocationData::FRandomLocationData()
{
    bRandomizeXAxis = false;
    bRandomizeYAxis = false;
    bRandomizeZAxis = false;

    XAxisRange.Min = -100.f;
    XAxisRange.Max = 100.f;
    YAxisRange.Min = -100.f;
    YAxisRange.Max = 100.f;
    ZAxisRange.Min = -100.f;
    ZAxisRange.Max = 100.f;
}

FVector FRandomLocationData::GetRandomLocation() const
{
    FVector RandomLocation = FVector::ZeroVector;

    if (bRandomizeXAxis)
    {
        RandomLocation.X = FMath::RandRange(XAxisRange.Min, XAxisRange.Max);
    }
    if (bRandomizeYAxis)
    {
        RandomLocation.Y = FMath::RandRange(YAxisRange.Min, YAxisRange.Max);
    }
    if (bRandomizeZAxis)
    {
        RandomLocation.Z = FMath::RandRange(ZAxisRange.Min, ZAxisRange.Max);
    }

    return RandomLocation;
}

FVector FRandomLocationData::GetRandomLocationRelative(const FVector& BaseLocation) const
{
    FVector RandomLocation = GetRandomLocation();

    return BaseLocation + RandomLocation;
}

// Get a random location in an object's local space
FVector FRandomLocationData::GetRandomLocationInLocalSpace(const FTransform& ObjectTransform) const
{
    FVector RandomLocation = GetRandomLocation();
    FVector NewLocation = ObjectTransform.TransformPosition(RandomLocation);

    return NewLocation;
}

//=================================== FRandomScale3DData ===================================
FRandomScale3DData::FRandomScale3DData()
{
    bUniformScale = true;
    bRandomizeXAxis = false;
    bRandomizeYAxis = false;
    bRandomizeZAxis = false;

    UniformScaleRange.Min = 0.5f;
    UniformScaleRange.Max = 2.f;
    XAxisRange.Min = 0.5f;
    XAxisRange.Max = 2.f;
    YAxisRange.Min = 0.5f;
    YAxisRange.Max = 2.f;
    ZAxisRange.Min = 0.5f;
    ZAxisRange.Max = 2.f;
}

FVector FRandomScale3DData::GetRandomScale3D() const
{
    static const float MinScale = 0.001f;
    FVector RandomScale = FVector(1.f, 1.f, 1.f);

    if (bUniformScale)
    {
        RandomScale.X = RandomScale.Y = RandomScale.Z = FMath::Max(FMath::RandRange(UniformScaleRange.Min, UniformScaleRange.Max), MinScale);
    }
    else
    {
        if (bRandomizeXAxis)
        {
            RandomScale.X = FMath::Max(FMath::RandRange(XAxisRange.Min, XAxisRange.Max), MinScale);
        }
        if (bRandomizeYAxis)
        {
            RandomScale.Y = FMath::Max(FMath::RandRange(YAxisRange.Min, YAxisRange.Max), MinScale);
        }
        if (bRandomizeZAxis)
        {
            RandomScale.Z = FMath::Max(FMath::RandRange(ZAxisRange.Min, ZAxisRange.Max), MinScale);
        }
    }

    return RandomScale;
}

//=================================== FRandomColorData ===================================
FRandomColorData::FRandomColorData()
{
    RandomizationType = ERandomColorType::RandomizeAllColor;
    bRandomizeBetweenTwoColors = false;
    bRandomizeAroundAColor = false;
}

#if WITH_EDITORONLY_DATA
void FRandomColorData::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    const UProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
    if (PropertyThatChanged)
    {
        switch (RandomizationType)
        {
        default:
        case ERandomColorType::RandomizeAllColor:
        {
            bRandomizeAroundAColor = false;
            bRandomizeBetweenTwoColors = false;
            break;
        }
        case ERandomColorType::RandomizeBetweenTwoColors:
        {
            bRandomizeAroundAColor = false;
            bRandomizeBetweenTwoColors = true;
            break;
        }
        case ERandomColorType::RandomizeAroundAColor:
        {
            bRandomizeAroundAColor = true;
            bRandomizeBetweenTwoColors = false;
            break;
        }
        }
    }
}
#endif // WITH_EDITORONLY_DATA

FLinearColor FRandomColorData::GetRandomColor() const
{
    switch (RandomizationType)
    {
        default:
        case ERandomColorType::RandomizeAllColor:
        {
            return GetRandomAnyColor();
        }
        case ERandomColorType::RandomizeBetweenTwoColors:
        {
            return GetRandomColorInRange(FirstColor, SecondColor, bRandomizeInHSV);
        }
        case ERandomColorType::RandomizeAroundAColor:
        {
            return GetRandomColorAround(MainColor, MaxHueChange, MaxSaturationChange, MaxValueChange);
        }
    }
}

FLinearColor FRandomColorData::GetRandomAnyColor()
{
    FLinearColor RandomColor;
    RandomColor.R = FMath::FRand();
    RandomColor.G = FMath::FRand();
    RandomColor.B = FMath::FRand();

    return RandomColor;
}

FLinearColor FRandomColorData::GetRandomColorInRange(const FLinearColor& Color1, const FLinearColor& Color2, const bool& bRandomizeInHSV)
{
    FLinearColor RandomColor;
    if (bRandomizeInHSV)
    {
        RandomColor = FLinearColor::LerpUsingHSV(Color1, Color2, FMath::FRand());
    }
    else
    {
        RandomColor.R = FMath::RandRange(Color1.R, Color2.R);
        RandomColor.G = FMath::RandRange(Color1.G, Color2.G);
        RandomColor.B = FMath::RandRange(Color1.B, Color2.B);
    }

    return RandomColor;
}

FLinearColor FRandomColorData::GetRandomColorAround(const FLinearColor& BaseColor, const float& HueDelta, const float& SaturationDelta, const float& ValueDelta)
{
    FLinearColor BaseHSV = BaseColor.LinearRGBToHSV();
    // Randomize Hue
    if (HueDelta > 0.f)
    {
        //BaseHSV.R += FMath::RandRange(-HueDelta, HueDelta);
        BaseHSV.R += FRandUtils::RandGaussian(0, HueDelta);
        if (BaseHSV.R < 0.f)
        {
            BaseHSV.R += 360.f;
        }
        else if (BaseHSV.R > 360.f)
        {
            BaseHSV.R -= 360.f;
        }
    }

    // Randomize Saturation
    if (SaturationDelta > 0.f)
    {
        //BaseHSV.G = FMath::Max(FMath::Min(BaseHSV.G + FMath::RandRange(-SaturationDelta, SaturationDelta), 1.f), 0.f);
        BaseHSV.G += FRandUtils::RandGaussian(0, SaturationDelta);
        BaseHSV.G = FMath::Max(FMath::Min(BaseHSV.G, 1.f), 0.f);
    }

    // Randomize Value
    if (ValueDelta > 0.f)
    {
        //BaseHSV.B = FMath::Max(FMath::Min(BaseHSV.G + FMath::RandRange(-ValueDelta, ValueDelta), 1.f), 0.f);
        BaseHSV.B += FRandUtils::RandGaussian(0, ValueDelta);
        BaseHSV.B = FMath::Max(FMath::Min(BaseHSV.B, 1.f), 0.f);
    }

    FLinearColor RandomColor = BaseHSV.HSVToLinearRGB();
    return RandomColor;
}

//=================================== FRandUtils ===================================
// Reference: https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform#Implementation
float FRandUtils::RandGaussian(const float mu, const float sigma)
{
    static const float epsilon = SMALL_NUMBER;
    static const float two_pi = 2.0 * 3.14159265358979323846;

    float u1, u2;
    do
    {
        u1 = FMath::FRand();
        u2 = FMath::FRand();
    }
    while (u1 <= epsilon);

    float z0, z1;
    z0 = FMath::Sqrt(-2.0 * log(u1)) * cos(two_pi * u2);
    z1 = FMath::Sqrt(-2.0 * log(u1)) * sin(two_pi * u2);

    return z0 * sigma + mu;
}

FVector2D FRandUtils::RandGaussian2D(const float mu, const float sigma)
{
    static const float epsilon = SMALL_NUMBER;
    static const float two_pi = 2.0 * 3.14159265358979323846;

    float u1, u2;
    do
    {
        u1 = FMath::FRand();
        u2 = FMath::FRand();
    }
    while (u1 <= epsilon);

    float z0, z1;
    z0 =  FMath::Sqrt(-2.0 * log(u1)) * cos(two_pi * u2);
    z1 = FMath::Sqrt(-2.0 * log(u1)) * sin(two_pi * u2);

    FVector2D v;
    v.X = z0 * sigma + mu;
    v.Y = z1 * sigma + mu;
    return v;
}

//=================================== FRandomMaterialSelection ===================================
FRandomMaterialSelection::FRandomMaterialSelection()
{
    MaterialSelectionType = EMaterialSelectionType::ModifyAllMaterials;
    bSelectMaterialByIndexes = false;
    bSelectMaterialBySlotNames = false;

    // By default, process the first material of the owner's mesh
    MaterialIndexes.Reset(1);
    MaterialIndexes.Add(0);

    MaterialSlotNames.Reset();
}

#if WITH_EDITORONLY_DATA
void FRandomMaterialSelection::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    const UProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
    if (PropertyThatChanged)
    {
        switch (MaterialSelectionType)
        {
            default:
            case EMaterialSelectionType::ModifyAllMaterials:
            {
                bSelectMaterialByIndexes = false;
                bSelectMaterialBySlotNames = false;
                break;
            }
            case EMaterialSelectionType::ModifyMaterialsInIndexesList:
            {
                bSelectMaterialByIndexes = true;
                bSelectMaterialBySlotNames = false;
                break;
            }
            case EMaterialSelectionType::ModifyMaterialInSlotNamesList:
            {
                bSelectMaterialByIndexes = false;
                bSelectMaterialBySlotNames = true;
                break;
            }
        }
    }
}
#endif //WITH_EDITORONLY_DATA

TArray<int32> FRandomMaterialSelection::GetAffectMaterialIndexes(const class UMeshComponent* MeshComp) const
{
    TArray<int32> AffectMaterialIndexes;
    ensure(MeshComp);
    if (!MeshComp)
    {
        UE_LOG(LogNVDRUtils, Warning, TEXT("Invalid argument"));
    }
    else
    {
        int32 TotalMaterialCount = MeshComp->GetNumMaterials();
        AffectMaterialIndexes.Reserve(TotalMaterialCount);

        if (MaterialSelectionType == EMaterialSelectionType::ModifyAllMaterials)
        {
            for (int i = 0; i < TotalMaterialCount; i++)
            {
                AffectMaterialIndexes.Add(i);
            }
        }
        else if (MaterialSelectionType == EMaterialSelectionType::ModifyMaterialsInIndexesList)
        {
            AffectMaterialIndexes = MaterialIndexes;
        }
        else if (MaterialSelectionType == EMaterialSelectionType::ModifyMaterialInSlotNamesList)
        {
            const UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(MeshComp);
            if (StaticMeshComp)
            {
                const UStaticMesh* StaticMesh = StaticMeshComp->GetStaticMesh();
                if (StaticMesh)
                {
                    for (int i = 0; i < StaticMesh->StaticMaterials.Num(); i++)
                    {
                        const FStaticMaterial& StaticMaterial = StaticMesh->StaticMaterials[i];
                        if (MaterialSlotNames.Contains(StaticMaterial.MaterialSlotName))
                        {
                            AffectMaterialIndexes.Add(i);
                        }
                    }
                }
            }
            else
            {
                const USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(MeshComp);
                const USkeletalMesh* SkeletalMesh = SkeletalMeshComp ? SkeletalMeshComp->SkeletalMesh : nullptr;
                if (SkeletalMesh)
                {
                    for (int i = 0; i < SkeletalMesh->Materials.Num(); i++)
                    {
                        const FSkeletalMaterial& SkeletalMaterial = SkeletalMesh->Materials[i];
                        if (MaterialSlotNames.Contains(SkeletalMaterial.MaterialSlotName))
                        {
                            AffectMaterialIndexes.Add(i);
                        }
                    }
                }
            }
        }
    }
    return AffectMaterialIndexes;
}

//=================================== FRandomAssetStreamer ===================================
// TODO: May expose these number as config parameter for the streamer
// Maximum number of loaded textures to keep at a time
const int32 MAX_LOADED_ASSETS_COUNT = 100;
// When the number of unused texture is smaller than this then try to load new textures in
const int32 MAX_LOADED_ASSETS_BUFFER = 20;
// How many texture to be async loaded at a time
const int32 MAX_ASYNC_LOAD_ASSETS_COUNT = 5;

FRandomAssetStreamer::FRandomAssetStreamer()
{
    AssetDirectories.Reset();
    ManagedAssetClass = nullptr;

    StreamerCallbackPtr = TSharedPtr<FRandomAssetStreamerCallback>(new FRandomAssetStreamerCallback());
    if (StreamerCallbackPtr.IsValid())
    {
        StreamerCallbackPtr->AssetStreamer = this;
    }
}

FRandomAssetStreamer::FRandomAssetStreamer(const FRandomAssetStreamer& OtherStreamer)
{
    AssetDirectories = OtherStreamer.AssetDirectories;
    ManagedAssetClass = OtherStreamer.ManagedAssetClass;

    AllAssetReferences = OtherStreamer.AllAssetReferences;
    LoadedAssetReferences = OtherStreamer.LoadedAssetReferences;
    LoadingAssetReferences = OtherStreamer.LoadingAssetReferences;
}

FRandomAssetStreamer::~FRandomAssetStreamer()
{
    if (StreamerCallbackPtr.IsValid())
    {
        StreamerCallbackPtr->AssetStreamer = nullptr;
        StreamerCallbackPtr = nullptr;
    }

    if (StreamableHandlePtr.IsValid())
    {
        StreamableHandlePtr->CancelHandle();
        StreamableHandlePtr->ReleaseHandle();
        StreamableHandlePtr = nullptr;
    }

    AssetDirectories.Reset();
    ManagedAssetClass = nullptr;

    AllAssetReferences.Reset();
    LoadedAssetReferences.Reset();
    LoadingAssetReferences.Reset();
}

FRandomAssetStreamer& FRandomAssetStreamer::operator=(const FRandomAssetStreamer& OtherStreamer)
{
    AssetDirectories = OtherStreamer.AssetDirectories;
    ManagedAssetClass = OtherStreamer.ManagedAssetClass;

    AllAssetReferences = OtherStreamer.AllAssetReferences;
    LoadedAssetReferences = OtherStreamer.LoadedAssetReferences;
    LoadingAssetReferences = OtherStreamer.LoadingAssetReferences;

    return *this;
}

void FRandomAssetStreamer::Init(const TArray<FDirectoryPath>& InAssetDirectories, UClass* InAssetClass)
{
    AssetDirectories = InAssetDirectories;
    ManagedAssetClass = InAssetClass;

    ScanPath();
}

void FRandomAssetStreamer::ScanPath()
{
    AllAssetReferences.Reset();
    LoadedAssetReferences.Reset();
    LoadingAssetReferences.Reset();

    UAssetManager& AssetManager = UAssetManager::Get();
    IAssetRegistry& AssetRegistry = AssetManager.GetAssetRegistry();

    // FIXME: The AssetRegistryModule only work for Editor build
    // May want to use AssetManager since it's available in runtime in UE 4.17
#if WITH_EDITORONLY_DATA
    if (ManagedAssetClass && (AssetDirectories.Num() > 0))
    {
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

        FPrimaryAssetType PrimaryAssetType = ManagedAssetClass->GetFName();
        const bool bForceSynchronousScan = true;
        TArray<FString> AssetDirPaths;
        for (const auto& AssetDirectory : AssetDirectories)
        {
            FString DirPath = AssetDirectory.Path;
            FPaths::NormalizeDirectoryName(DirPath);
            while (!DirPath.IsEmpty() && DirPath.EndsWith(TEXT("/")))
            {
                DirPath = DirPath.Left(DirPath.Len() - 1);
            }
            // NOTE: All the directory must be inside the game's content folder
            const FString& AssetDirPath = FPaths::Combine(TEXT("/Game"), DirPath);
            AssetDirPaths.Add(AssetDirPath);

        }
        int32 AssetCount = AssetManager.ScanPathsForPrimaryAssets(PrimaryAssetType, AssetDirPaths, ManagedAssetClass, false, false, bForceSynchronousScan);

        for (const auto& AssetDirPath : AssetDirPaths)
        {
            TArray<FAssetData> AssetList;

            if (AssetRegistryModule.Get().GetAssetsByPath(*AssetDirPath, AssetList, true))
            {
                for (const FAssetData& AssetData : AssetList)
                {
                    const UClass* CheckAssetClass = AssetData.GetClass();
                    if (CheckAssetClass && CheckAssetClass->IsChildOf(ManagedAssetClass))
                    {
                        const FSoftObjectPath& AssetObjectPath = AssetData.ToSoftObjectPath();

                        // NOTE: When the number of assets are really large this references array can take a lot of memory
                        AllAssetReferences.Add(AssetObjectPath);
                    }
                }
            }
        }

        const int32 TotalAssetCount = AllAssetReferences.Num();
        if (TotalAssetCount <= 0)
        {
            UE_LOG(LogNVDRUtils, Warning, TEXT("FRandomAssetStreamer - There are no asset of type '%s' in directory '%s'"), *ManagedAssetClass->GetName(), *AssetDirectories[0].Path);
        }
        else
        {
            // NOTE: We force load asset in the initial setup, all other follow up load are async
            LoadNextBatch(false);
        }
    }
#endif // WITH_EDITORONLY_DATA
}

int FRandomAssetStreamer::GetAssetsCount() const
{
    return AllAssetReferences.Num();
}

bool FRandomAssetStreamer::HasAssets() const
{
    return (AllAssetReferences.Num() > 0);
}

FSoftObjectPath FRandomAssetStreamer::GetNextAssetReference()
{
    FStringAssetReference NextAssetRef;
    const uint32 TotalLoadedAssetCount = LoadedAssetReferences.Num();
    if (TotalLoadedAssetCount > 0)
    {
        LastUsedAssetIndex = LastUsedAssetIndex % TotalLoadedAssetCount;
        NextAssetRef = LoadedAssetReferences[LastUsedAssetIndex];
        LastUsedAssetIndex++;

        // Reduce the number of unused assets and try to load new ones in if it's less than a threshold
        UnusedAssetCount = FMath::Max(UnusedAssetCount - 1, 0);
        if (UnusedAssetCount < MAX_LOADED_ASSETS_BUFFER)
        {
            LoadNextBatch();
        }
    }
    else
    {
        // FIXME: Need to cached some back up assets?
    }

    return NextAssetRef;
}

bool FRandomAssetStreamer::IsLoadingAssets() const
{
    return (LoadingAssetReferences.Num() > 0);
}

void FRandomAssetStreamer::LoadNextBatch(bool bAsyncLoad/*= true*/)
{
    if (IsLoadingAssets() || !StreamerCallbackPtr.IsValid())
    {
        return;
    }

    LoadingAssetReferences.Reset(MAX_ASYNC_LOAD_ASSETS_COUNT);

    const int32 TotalAssetCount = AllAssetReferences.Num();
    if (TotalAssetCount <= 0)
    {
        return;
    }

    for (int i = 0; i < MAX_ASYNC_LOAD_ASSETS_COUNT; i++)
    {
        int AssetIndex = FMath::Rand() % TotalAssetCount;
        const FSoftObjectPath& StrAssetRef = AllAssetReferences[AssetIndex];
        LoadingAssetReferences.Add(StrAssetRef);
    }

    if (bAsyncLoad)
    {
        StreamableHandlePtr = AssetStreamer.RequestAsyncLoad(LoadingAssetReferences,
                              FStreamableDelegate::CreateSP(StreamerCallbackPtr.ToSharedRef(), &FRandomAssetStreamerCallback::Callback),
                              FStreamableManager::DefaultAsyncLoadPriority, true);
    }
    else
    {
        StreamableHandlePtr = AssetStreamer.RequestSyncLoad(LoadingAssetReferences, true);
        OnAssetBatchLoaded();
    }
}

void FRandomAssetStreamer::OnAssetBatchLoaded()
{
    const int32 NewLoadedAssetCount = LoadingAssetReferences.Num();
    if (NewLoadedAssetCount <= 0)
    {
        return;
    }

    const int32 NumberOfAssetToAdd = FMath::Min(MAX_LOADED_ASSETS_COUNT - LoadedAssetReferences.Num(), NewLoadedAssetCount);

    for (int i = 0; i < NumberOfAssetToAdd; i++)
    {
        LoadedAssetReferences.Add(LoadingAssetReferences[i]);
    }
    const int32 TotalLoadedAssetCount = LoadedAssetReferences.Num();
    if (TotalLoadedAssetCount <= 0)
    {
        return;
    }

    UnusedAssetCount += NewLoadedAssetCount;
    LastLoadedAssetIndex = (LastLoadedAssetIndex + NumberOfAssetToAdd) % TotalLoadedAssetCount;

    // If there are more assets to add then add them to the front of the list
    if (NumberOfAssetToAdd < NewLoadedAssetCount)
    {
        for (int i = 0; i < NewLoadedAssetCount - NumberOfAssetToAdd; i++)
        {
            LoadedAssetReferences[LastLoadedAssetIndex] = LoadingAssetReferences[NumberOfAssetToAdd + i];
            LastLoadedAssetIndex = (LastLoadedAssetIndex + 1) % TotalLoadedAssetCount;
        }
    }

    LoadingAssetReferences.Reset();

    if (LoadedAssetReferences.Num() + MAX_ASYNC_LOAD_ASSETS_COUNT <= MAX_LOADED_ASSETS_COUNT)
    {
        LoadNextBatch();
    }
}

//=================================== Misc ===================================
namespace DRUtils
{
    UMeshComponent* GetFirstValidMeshComponent(AActor* OwnerActor)
    {
        UMeshComponent* FoundMeshComp = nullptr;
        if (!OwnerActor)
        {
            return FoundMeshComp;
        }

		TArray<UMeshComponent*> ChildMeshComps;
		OwnerActor->GetComponents<UMeshComponent>(ChildMeshComps);
		for (auto ChildMeshComp : ChildMeshComps)
        {
            if (ChildMeshComp)
            {
                USkinnedMeshComponent* SkinnedMeshComp = Cast<USkeletalMeshComponent>(ChildMeshComp);
				if (SkinnedMeshComp)
                {
                    if (SkinnedMeshComp->SkeletalMesh)
                    {
                        FoundMeshComp = SkinnedMeshComp;
                        break;
                    }
                }
                else
                {
                    UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(ChildMeshComp);
                    if (StaticMeshComp && StaticMeshComp->GetStaticMesh())
                    {
                        FoundMeshComp = StaticMeshComp;
                        break;
                    }
                }
            }
        }

        return FoundMeshComp;
    }

    extern TArray<UMeshComponent*> GetValidChildMeshComponents(AActor* OwnerActor)
    {
        TArray<UMeshComponent*> ValidMeshComps;
        ValidMeshComps.Reset();

        if (!OwnerActor)
        {
            return ValidMeshComps;
        }

		TArray<UMeshComponent*> ChildMeshComps;
		OwnerActor->GetComponents<UMeshComponent>(ChildMeshComps);
		for (auto ChildMeshComp : ChildMeshComps)
        {
            if (ChildMeshComp)
            {
                USkinnedMeshComponent* SkinnedMeshComp = Cast<USkeletalMeshComponent>(ChildMeshComp);
                if (SkinnedMeshComp)
                {
                    if (SkinnedMeshComp->SkeletalMesh)
                    {
                        ValidMeshComps.Add(SkinnedMeshComp);
                    }
                }
                else
                {
                    UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(ChildMeshComp);
                    if (StaticMeshComp && StaticMeshComp->GetStaticMesh())
                    {
                        ValidMeshComps.Add(StaticMeshComp);
                    }
                }
            }
        }

        return ValidMeshComps;
    }
}
