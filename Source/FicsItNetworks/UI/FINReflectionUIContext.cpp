﻿#include "FINReflectionUIContext.h"

#include "FINReflectionClassHirachyViewer.h"
#include "FINReflectionEntryListViewer.h"
#include "FINReflectionSignatureViewer.h"
#include "Reflection/FINReflection.h"
#include "Network/FINNetworkValues.h"
#include "Reflection/FINArrayProperty.h"
#include "Reflection/FINClassProperty.h"
#include "Reflection/FINObjectProperty.h"
#include "Reflection/FINStructProperty.h"
#include "Reflection/FINTraceProperty.h"

FString GetText(UFINProperty* Prop) {
	if (!Prop) return FString();
	switch (Prop->GetType()) {
	case FIN_ANY:
		return "Any";
	case FIN_NIL:
		return "Nil";
	case FIN_BOOL:
		return "Bool";
	case FIN_INT:
		return "Int";
	case FIN_FLOAT:
		return "Float";
	case FIN_STR:
		return "Str";
	case FIN_OBJ:
		if (Cast<UFINObjectProperty>(Prop)->GetSubclass()) {
			return FString("Obj(") + FFINReflection::Get()->FindClass(Cast<UFINObjectProperty>(Prop)->GetSubclass())->GetDisplayName().ToString() + ")";
		}
		return "Obj";
	case FIN_CLASS:
		if (Cast<UFINClassProperty>(Prop)->GetSubclass()) {
			return FString("Class(") + FFINReflection::Get()->FindClass(Cast<UFINClassProperty>(Prop)->GetSubclass())->GetDisplayName().ToString() + ")";
		}
		return "Class";
	case FIN_STRUCT:
		if (Cast<UFINStructProperty>(Prop)->Struct) {
			return FString("Struct(") + Cast<UFINStructProperty>(Prop)->Struct->GetDisplayNameText().ToString() + ")";
		}
		return "Struct";
	case FIN_TRACE:
		if (Cast<UFINTraceProperty>(Prop)->GetSubclass()) {
			return FString("Trace(") + FFINReflection::Get()->FindClass(Cast<UFINTraceProperty>(Prop)->GetSubclass())->GetDisplayName().ToString() + ")";
		}
		return "Trace";
	case FIN_ARRAY:
		return FString("Array(") + GetText(Cast<UFINArrayProperty>(Prop)->GetInnerType()) + ")";
	default:
		return FString();
	}
}

TSharedRef<SWidget> GenerateDataTypeIcon(UFINProperty* Prop, const FFINReflectionUIStyleStruct* Style) {
	check(Style != nullptr);
	FString Text = GetText(Prop);
	return SNew(STextBlock).Text(FText::FromString(Text));
}

TSharedRef<SWidget> GeneratePropTypeIcon(UFINProperty* Prop, const FFINReflectionUIStyleStruct* Style) {
	check(Style != nullptr);
	TSharedRef<SImage> Image = SNew(SImage).Image(&Style->MemberAttrib);
	if (Prop->GetPropertyFlags() & FIN_Prop_ClassProp) Image->SetImage(&Style->ClassAttrib);
	return SNew(SBox).WidthOverride(15).HeightOverride(15)[
		Image
	];
}

TSharedRef<SWidget> GenerateFuncTypeIcon(UFINFunction* Func, const FFINReflectionUIStyleStruct* Style) {
	check(Style != nullptr);
	TSharedRef<SImage> Image = SNew(SImage).Image(&Style->MemberFunc);
	if (Func->GetFunctionFlags() & FIN_Func_ClassFunc) Image->SetImage(&Style->ClassFunc);
	if (Func->GetFunctionFlags() & FIN_Func_StaticFunc) Image->SetImage(&Style->StaticFunc);
	return SNew(SBox).WidthOverride(15).HeightOverride(15).Content()[
		Image
	];
}

FFINReflectionUIFilter::FFINReflectionUIFilter(FString Filter) {
	FString Token;
	while (Filter.Split(" ", &Filter, &Token)) {
		if (Token.Len() > 0) Tokens.Add(Token);
	}
	if (Filter.Len() > 0) Tokens.Add(Filter);
}

bool FFINReflectionUIFilter::PassesFilter(const FString& String) const {
	for (const FString& Token : Tokens) {
		if (!String.Contains(Token)) return false;
	}
	return true;
}

FFINReflectionUIClass::FFINReflectionUIClass(UFINClass* Class, FFINReflectionUIContext* Context) : FFINReflectionUIEntry(Context), Class(Class) {
	for (UFINProperty* Prop : Class->GetProperties()) {
		Attributes.Add(MakeShared<FFINReflectionUIProperty>(Prop, Context));
	}
	for (UFINFunction* Func : Class->GetFunctions()) {
		Functions.Add(MakeShared<FFINReflectionUIFunction>(Func, Context));
	}
	for (UFINRefSignal* Signal : Class->GetSignals()) {
		Signals.Add(MakeShared<FFINReflectionUISignal>(Signal, Context));
	}
	Filtered = Attributes;
	Filtered.Append(Functions);
	Filtered.Append(Signals);
}

TSharedRef<SWidget> FFINReflectionUIClass::GetDetailsWidget() {
	return SNew(SGridPanel)
	.FillColumn(0, 1)
	.FillRow(1, 1)
	+SGridPanel::Slot(0, 0)[
		GetPreview()
	]
	+SGridPanel::Slot(0,1)[
		SNew(SScrollBox)
		+SScrollBox::Slot()[
			SNew(STextBlock).Text(Class->GetDescription())
		]
		+SScrollBox::Slot()[
			SNew(SFINReflectionEntryListViewer, &Attributes, Context)
		]
		+SScrollBox::Slot()[
			SNew(SFINReflectionEntryListViewer, &Functions, Context)
		]
		+SScrollBox::Slot()[
			SNew(SFINReflectionEntryListViewer, &Signals, Context)
		]
	]
	+SGridPanel::Slot(1, 1)[
		SNew(SBox)
		.MinDesiredWidth(200)
		.Content()[
			SNew(SFINReflectionClassHirachyViewer, SharedThis(this), Context)
			.Style(Context->Style)
		]
	];
}

TSharedRef<SWidget> FFINReflectionUIClass::GetShortPreview() {
	return SNew(SHorizontalBox)
	+SHorizontalBox::Slot().AutoWidth()[
		SNew(STextBlock).Text(Class->GetDisplayName())
		.HighlightText_Lambda([this](){ return FText::FromString(Context->FilterString); })
	]
	+SHorizontalBox::Slot().AutoWidth().Padding(5,0,0,0)[
		SNew(STextBlock).Text( FText::FromString(Class->GetInternalName()))
		.HighlightText_Lambda([this](){ return FText::FromString(Context->FilterString); })
	];
}

TSharedRef<SWidget> FFINReflectionUIClass::GetPreview() {
	return GetShortPreview();
}

bool FFINReflectionUIClass::ApplyFilter(const FFINReflectionUIFilter& Filter) {
	Filtered.Empty();
	TArray<TSharedPtr<FFINReflectionUIEntry>> Entries;
	Entries.Append(Attributes);
	Entries.Append(Functions);
	Entries.Append(Signals);
	for (const TSharedPtr<FFINReflectionUIEntry>& Entry : Entries) {
		if (Entry->ApplyFilter(Filter)) {
			Filtered.Add(Entry);
		}
	}
	return Filtered.Num() > 0 || Filter.PassesFilter(Class->GetDisplayName().ToString() + " " + Class->GetInternalName());
}

TSharedRef<SWidget> FFINReflectionUIProperty::GetDetailsWidget() {
	return SNew(SGridPanel)
	.FillColumn(0, 1)
	.FillRow(1, 1)
	+SGridPanel::Slot(0, 0)[
		GetPreview()
	]
	+SGridPanel::Slot(0,1)[
		SNew(SScrollBox)
		+SScrollBox::Slot()[
			SNew(STextBlock).Text( Property->GetDescription())
		]
	];
}

TSharedRef<SWidget> FFINReflectionUIProperty::GetShortPreview() {
	return SNew(SHorizontalBox)
	+SHorizontalBox::Slot().AutoWidth()[
		GeneratePropTypeIcon(Property, Context->Style.Get())
	]
	+SHorizontalBox::Slot().AutoWidth().Padding(5,0,5,0)[
		GenerateDataTypeIcon(Property, Context->Style.Get())
	]
	+SHorizontalBox::Slot().AutoWidth()[
		SNew(STextBlock).Text(Property->GetDisplayName())
		.HighlightText_Lambda([this](){ return FText::FromString(Context->FilterString); })
	]
	+SHorizontalBox::Slot().AutoWidth().Padding(5,0,0,0)[
		SNew(STextBlock).Text(FText::FromString(Property->GetInternalName()))
		.HighlightText_Lambda([this](){ return FText::FromString(Context->FilterString); })
	];
}
TSharedRef<SWidget> FFINReflectionUIProperty::GetPreview() {
	return GetShortPreview();
}

bool FFINReflectionUIProperty::ApplyFilter(const FFINReflectionUIFilter& Filter) {
	return Filter.PassesFilter(Property->GetDisplayName().ToString() + " " + Property->GetInternalName());
}

TSharedRef<SWidget> FFINReflectionUIFunction::GetDetailsWidget() {
	TSharedRef<SGridPanel> Panel = SNew(SGridPanel)
	.FillColumn(0, 1)
	.FillRow(1, 1)
	+SGridPanel::Slot(0, 0)[
		GetPreview()
	]
	+SGridPanel::Slot(0,1)[
		SNew(SScrollBox)
		+SScrollBox::Slot()[
			SNew(STextBlock).Text(Function->GetDescription())
		]
		+SScrollBox::Slot()[
			SNew(SFINReflectionSignatureViewer, Function->GetParameters(), Context)
			.Style(Context->Style)
		]
	];
	TSharedPtr<FFINReflectionUIClass>* Class = Context->Classes.Find(Cast<UFINClass>(Function->GetOuter()));
	SGridPanel::FSlot& Slot = Panel->AddSlot(1, 0);
	if (Class && Class->IsValid()) Slot[
		Class->Get()->GetShortPreview()
	];
	return Panel;
}

TSharedRef<SWidget> FFINReflectionUIFunction::GetShortPreview() {
	return SNew(SHorizontalBox)
	+SHorizontalBox::Slot().AutoWidth()[
		GenerateFuncTypeIcon(Function, Context->Style.Get())
	]
	+SHorizontalBox::Slot().AutoWidth().Padding(5,0,5,0)[
		SNew(STextBlock).Text(Function->GetDisplayName())
		.HighlightText_Lambda([this](){ return FText::FromString(Context->FilterString); })
	]
	+SHorizontalBox::Slot().AutoWidth()[
		SNew(STextBlock).Text(FText::FromString(Function->GetInternalName()))
		.HighlightText_Lambda([this](){ return FText::FromString(Context->FilterString); })
	];
}
TSharedRef<SWidget> FFINReflectionUIFunction::GetPreview() {
	TSharedPtr<SHorizontalBox> ParamBox;
	TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox)
	+SHorizontalBox::Slot().AutoWidth()[
		GenerateFuncTypeIcon(Function, Context->Style.Get())
	]
	+SHorizontalBox::Slot().AutoWidth().Padding(5,0,5,0)[
		SNew(STextBlock).Text(Function->GetDisplayName())
	]
	+SHorizontalBox::Slot().AutoWidth().Padding(0,0,5,0)[
		SNew(STextBlock).Text(FText::FromString(Function->GetInternalName()))
	]
	+SHorizontalBox::Slot().AutoWidth()[
		SNew(STextBlock).Text(FText::FromString("("))
	]
	+SHorizontalBox::Slot().AutoWidth().Padding(5,0,5,0)[
		SAssignNew(ParamBox, SHorizontalBox)
	]
	+SHorizontalBox::Slot().AutoWidth()[
		SNew(STextBlock).Text(FText::FromString(")"))
	];
	for (UFINProperty* Prop : Function->GetParameters()) {
		if (ParamBox->NumSlots() > 0) {
			ParamBox->AddSlot().AutoWidth().Padding(5,0,5,0)[
				SNew(STextBlock).Text(FText::FromString(","))
			];
		}
		ParamBox->AddSlot().AutoWidth()[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot().AutoWidth()[
				GenerateDataTypeIcon(Prop, Context->Style.Get())
			]
			+SHorizontalBox::Slot().AutoWidth().Padding(5,0,0,0)[
				SNew(STextBlock).Text(Prop->GetDisplayName())
			]
		];
	}
	return Box;
}

bool FFINReflectionUIFunction::ApplyFilter(const FFINReflectionUIFilter& Filter) {
	return Filter.PassesFilter(Function->GetDisplayName().ToString() + " " + Function->GetInternalName());
}

TSharedRef<SWidget> FFINReflectionUISignal::GetDetailsWidget() {
	return SNew(SGridPanel)
	.FillColumn(0, 1)
	.FillRow(1, 1)
	+SGridPanel::Slot(0, 0)[
		GetPreview()
	]
	+SGridPanel::Slot(0,1)[
		SNew(SScrollBox)
		+SScrollBox::Slot()[
			SNew(STextBlock).Text(Signal->GetDescription())
		]
		+SScrollBox::Slot()[
			SNew(SFINReflectionSignatureViewer, Signal->GetParameters(), Context)
			.Style(Context->Style)
		]
	];
}

TSharedRef<SWidget> FFINReflectionUISignal::GetShortPreview() {
	return SNew(SHorizontalBox)
	+SHorizontalBox::Slot().AutoWidth()[
		SNew(STextBlock).Text(Signal->GetDisplayName())
		.HighlightText_Lambda([this](){ return FText::FromString(Context->FilterString); })
	]
	+SHorizontalBox::Slot().AutoWidth().Padding(5,0,0,0)[
		SNew(STextBlock).Text(FText::FromString(Signal->GetInternalName()))
		.HighlightText_Lambda([this](){ return FText::FromString(Context->FilterString); })
	];
}

TSharedRef<SWidget> FFINReflectionUISignal::GetPreview() {
	TSharedPtr<SHorizontalBox> ParamBox;
	TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox)
	+SHorizontalBox::Slot().AutoWidth()[
		SNew(STextBlock).Text(Signal->GetDisplayName())
	]
	+SHorizontalBox::Slot().AutoWidth().Padding(5,0,5,0)[
		SNew(STextBlock).Text(FText::FromString(Signal->GetInternalName()))
	]
	+SHorizontalBox::Slot().AutoWidth()[
		SNew(STextBlock).Text(FText::FromString("("))
	]
	+SHorizontalBox::Slot().AutoWidth().Padding(5,0,5,0)[
		SAssignNew(ParamBox, SHorizontalBox)
	]
	+SHorizontalBox::Slot().AutoWidth()[
		SNew(STextBlock).Text(FText::FromString(")"))
	];
	for (UFINProperty* Prop : Signal->GetParameters()) {
		if (ParamBox->NumSlots() > 0) {
			ParamBox->AddSlot().AutoWidth().Padding(5,0,5,0)[
				SNew(STextBlock).Text(FText::FromString(","))
			];
		}
		ParamBox->AddSlot().AutoWidth()[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot().AutoWidth()[
				GenerateDataTypeIcon(Prop, Context->Style.Get())
			]
			+SHorizontalBox::Slot().AutoWidth().Padding(5,0,0,0)[
				SNew(STextBlock).Text(Prop->GetDisplayName())
			]
		];
	}
	return Box;
}

bool FFINReflectionUISignal::ApplyFilter(const FFINReflectionUIFilter& Filter) {
	return Filter.PassesFilter(Signal->GetDisplayName().ToString() + " " + Signal->GetInternalName());
}

FFINReflectionUIContext::FFINReflectionUIContext() {
	Entries.Empty();
	Classes.Empty();
	for (const TPair<UClass*, UFINClass*>& Class : FFINReflection::Get()->GetClasses()) {
		UE_LOG(LogTemp, Warning, TEXT("%p %s %p %s"), Class.Key, *Class.Key->GetName(), Class.Value, *Class.Value->GetInternalName());
		Classes.Add(Class.Value, MakeShared<FFINReflectionUIClass>(Class.Value, this));
	}
	for (const TPair<UFINClass*, TSharedPtr<FFINReflectionUIClass>>& Class : Classes) Entries.Add(Class.Value);
}