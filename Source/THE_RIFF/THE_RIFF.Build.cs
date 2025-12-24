// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class THE_RIFF : ModuleRules
{
    public THE_RIFF(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "HTTP",
                "Json",
                "JsonUtilities"
            }
        );
    }
}

