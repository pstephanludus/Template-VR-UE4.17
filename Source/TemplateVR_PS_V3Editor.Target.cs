// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class TemplateVR_PS_V3EditorTarget : TargetRules
{
	public TemplateVR_PS_V3EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "TemplateVR_PS_V3" } );
	}
}
