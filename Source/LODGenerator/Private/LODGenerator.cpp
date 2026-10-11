#include "LODGenerator.h"
#include "IPythonScriptPlugin.h"
#include "Widgets/Input/SSpinBox.h"

#define LOCTEXT_NAMESPACE "FLODGeneratorModule"

void FLODGeneratorModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(
			this,
			&FLODGeneratorModule::RegisterMenu)
	);
}

void FLODGeneratorModule::ShutdownModule()
{
	ReductionRowsBox.Reset();
	SelectedLODGroup.Reset();
	LODGroupOptions.Reset();

	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FLODGeneratorModule::OpenToolWindow()
{
	RefreshLODGroupOptions();

	auto MakeLabel = [](const FText& Text)
	{
		return SNew(STextBlock)
			.Text(Text)
			.MinDesiredWidth(130.0f);
	};

	const TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("LODGenerator_WindowTitle", "LOD Generator"))
		.ClientSize(FVector2D(560.0f, 460.0f))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	Window->SetContent(
		SNew(SBorder)
		.Padding(16.0f)
		[
			SNew(SScrollBox)

			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 16.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("LODGenerator_Header", "LOD Generator"))
					.Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						MakeLabel(LOCTEXT("LODGroup_Label", "LOD Group"))
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SComboBox<TSharedPtr<FName>>)
						.OptionsSource(&LODGroupOptions)
						.InitiallySelectedItem(SelectedLODGroup)
						.OnGenerateWidget_Lambda([](const TSharedPtr<FName>& InItem)
						{
							return SNew(STextBlock)
								.Text(InItem.IsValid()
									      ? FText::FromName(*InItem)
									      : FText::GetEmpty());
						})
						.OnSelectionChanged_Lambda([this](
							const TSharedPtr<FName>& NewSelection, ESelectInfo::Type)
							{
								SelectedLODGroup = NewSelection;

								if (SelectedLODGroup.IsValid())
								{
									CurrentOptions.LODGroup = *SelectedLODGroup;
								}
								else
								{
									CurrentOptions.LODGroup = NAME_None;
								}
							})
						[
							SNew(STextBlock)
							.Text_Lambda([this]
							{
								return SelectedLODGroup.IsValid()
									       ? FText::FromName(*SelectedLODGroup)
									       : LOCTEXT("NoLODGroupSelected", "None");
							})
						]
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 16.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ReductionSettingsHeader", "LOD Reduction Map"))
					.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("PercentTriangles_Header", "Percent Triangles"))
						.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ScreenSize_Header", "Screen Size"))
						.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(ReductionRowsBox, SVerticalBox)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("AddReductionRow_Button", "Add Row"))
						.OnClicked_Lambda([this]() -> FReply
						{
							CurrentOptions.ReductionEntries.Add(
								{0.5f, 0.5f});
							RebuildReductionRows();
							return FReply::Handled();
						})
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("RemoveReductionRow_Button", "Remove Last Row"))
						.OnClicked_Lambda([this]() -> FReply
						{
							if (CurrentOptions.ReductionEntries.Num() > 0)
							{
								CurrentOptions.ReductionEntries.RemoveAt(
									CurrentOptions.ReductionEntries.Num() - 1);
								RebuildReductionRows();
							}

							return FReply::Handled();
						})
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 20.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("RunLODGeneration_Button", "Generate LODs From Selection"))
					.OnClicked_Lambda([this]() -> FReply
					{
						ExecuteLODGeneration();
						return FReply::Handled();
					})
				]
			]
		]
	);

	RebuildReductionRows();

	FSlateApplication::Get().AddWindow(Window);
}

void FLODGeneratorModule::RegisterMenu()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(
		"LevelEditor.LevelEditorToolBar.PlayToolBar"
	);

	FToolMenuSection& Section = Menu->FindOrAddSection(
		"LODGenerator",
		LOCTEXT("LODGenerator_Section", "LODGenerator")
	);

	FToolMenuEntry Entry = FToolMenuEntry::InitToolBarButton(
		"LODGenerator_Button",
		FUIAction(
			FExecuteAction::CreateRaw(
				this,
				&FLODGeneratorModule::OpenToolWindow
			)
		),
		LOCTEXT("LODGenerator_Button_Label", "LODGenerator"),
		LOCTEXT("LODGenerator_Button_ToolTip", "Open LODGenerator"),
		FSlateIcon(
			FAppStyle::GetAppStyleSetName(),
			"EditorViewport.LODColorationMode"
		)
	);

	Entry.StyleNameOverride = "CalloutToolbar";
	Section.AddEntry(Entry);
}

void FLODGeneratorModule::RefreshLODGroupOptions()
{
	CurrentOptions = {};
	LODGroupOptions.Reset();
	SelectedLODGroup.Reset();

	TArray<FName> Groups;
	UStaticMesh::GetLODGroups(Groups);

	for (const FName& Group : Groups)
	{
		TSharedPtr<FName> GroupPtr = MakeShared<FName>(Group);
		LODGroupOptions.Add(GroupPtr);

		if (CurrentOptions.LODGroup == Group)
		{
			SelectedLODGroup = GroupPtr;
		}
	}

	if (!SelectedLODGroup.IsValid() && LODGroupOptions.Num() > 0)
	{
		SelectedLODGroup = LODGroupOptions[0];
		CurrentOptions.LODGroup = *SelectedLODGroup;
	}
}

void FLODGeneratorModule::RebuildReductionRows()
{
	if (!ReductionRowsBox.IsValid())
	{
		return;
	}

	ReductionRowsBox->ClearChildren();

	for (int32 Index = 0; Index < CurrentOptions.ReductionEntries.Num(); ++Index)
	{
		ReductionRowsBox->AddSlot()
		                .AutoHeight()
		                .Padding(0.0f, 3.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.01f)
				.MaxValue(1.0f)
				.MinSliderValue(0.01f)
				.MaxSliderValue(1.0f)
				.Value_Lambda([this, Index]
				{
					return CurrentOptions.ReductionEntries.IsValidIndex(Index)
						       ? CurrentOptions.ReductionEntries[Index].PercentTriangles
						       : 1.0f;
				})
				.OnValueChanged_Lambda([this, Index](const float NewValue)
				{
					if (CurrentOptions.ReductionEntries.IsValidIndex(Index))
					{
						CurrentOptions.ReductionEntries[Index].PercentTriangles = NewValue;
					}
				})
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.0f)
				.MaxValue(1.0f)
				.MinSliderValue(0.0f)
				.MaxSliderValue(1.0f)
				.Value_Lambda([this, Index]
				{
					return CurrentOptions.ReductionEntries.IsValidIndex(Index)
						       ? CurrentOptions.ReductionEntries[Index].ScreenSize
						       : 1.0f;
				})
				.OnValueChanged_Lambda([this, Index](const float NewValue)
				{
					if (CurrentOptions.ReductionEntries.IsValidIndex(Index))
					{
						CurrentOptions.ReductionEntries[Index].ScreenSize = NewValue;
					}
				})
			]
		];
	}
}

void FLODGeneratorModule::ExecuteLODGeneration() const
{
	if (!IPythonScriptPlugin::Get())
	{
		return;
	}

	IPythonScriptPlugin::Get()->ExecPythonCommand(*BuildPythonCommand());
}

FString FLODGeneratorModule::BuildPythonCommand() const
{
	FString PythonMap = TEXT("{");

	for (int32 Index = 0; Index < CurrentOptions.ReductionEntries.Num(); ++Index)
	{
		const FLODReductionEntry& Entry = CurrentOptions.ReductionEntries[Index];

		if (Index > 0)
		{
			PythonMap += TEXT(", ");
		}

		PythonMap += FString::Printf(
			TEXT("%s: %s"),
			*FString::SanitizeFloat(Entry.PercentTriangles),
			*FString::SanitizeFloat(Entry.ScreenSize)
		);
	}

	PythonMap += TEXT("}");

	FString LODGroup = CurrentOptions.LODGroup.ToString();
	LODGroup.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
	LODGroup.ReplaceInline(TEXT("'"), TEXT("\\'"));

	return FString::Printf(
		TEXT("import LODGeneratorScript\n")
		TEXT("LODGeneratorScript.level_of_detail_generator_from_selection(lod_group='%s', lod_reduction_map=%s)"),
		*LODGroup,
		*PythonMap
	);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLODGeneratorModule, LODGenerator)
