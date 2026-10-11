
#pragma once

#include "Modules/ModuleManager.h"

struct FLODReductionEntry
{
	float PercentTriangles{1.0f};
	float ScreenSize{1.0f};
};

struct FLODGeneratorOptions
{
	FName LODGroup{NAME_None};

	TArray<FLODReductionEntry> ReductionEntries
	{
		{
			{1.0f, 1.0f},
			{0.5f, 0.5f},
			{0.25f, 0.25f},
			{0.125f, 0.125f}
		}
	};
};

class FLODGeneratorModule : public IModuleInterface
{
	FLODGeneratorOptions CurrentOptions{};

	TArray<TSharedPtr<FName>> LODGroupOptions{};
	TSharedPtr<FName> SelectedLODGroup{};
	TSharedPtr<SVerticalBox> ReductionRowsBox{};

public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	LODGENERATOR_API void OpenToolWindow();

private:
	void RegisterMenu();
	
	void RefreshLODGroupOptions();
	void RebuildReductionRows();
	void ExecuteLODGeneration() const;

	FString BuildPythonCommand() const;
};
