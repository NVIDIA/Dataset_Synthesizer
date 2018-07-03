/*
* Copyright © 2018 NVIDIA Corporation.  All rights reserved.        
* This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0
* International License.  (https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode)
*/
#include "NVUtilitiesModule.h"
#include "NVNavigationPath.h"

ANVNavigationPath::ANVNavigationPath(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

ANVWaypoint* ANVNavigationPath::GetRandomWaypoint() const
{
    const int WaypointCount = WaypointList.Num();
    if (WaypointCount <= 0)
    {
        return nullptr;
    }

    return WaypointList[FMath::Rand() % WaypointCount];
}
