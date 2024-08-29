#include "VectorDatabaseAsset.h"

void UVectorDatabaseAsset::SaveFromVectorDatabase(UVectorDatabase* Database)
{
    if (!Database)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid Database provided to SaveFromVectorDatabase"));
        return;
    }

    Entries.Empty();
    TArray<FVectorDatabaseEntry> AllEntries = Database->GetAllVectorEntries(TArray<FString>());

    for (const FVectorDatabaseEntry& Entry : AllEntries)
    {
        FVectorDatabaseEntry NewEntry;
        NewEntry.Distance = Entry.Distance;
        NewEntry.Vector = Entry.Vector;

        if (Entry.Entry)
        {
            NewEntry.Entry = NewObject<UVectorEntryWrapper>(this);
            NewEntry.Entry->StringValue = Entry.Entry->StringValue;
            NewEntry.Entry->ObjectValue = Entry.Entry->ObjectValue;
            NewEntry.Entry->EntryType = Entry.Entry->EntryType;
            NewEntry.Entry->Category = Entry.Entry->Category;

            if (Entry.Entry->EntryType == EEntryType::Struct && Entry.Entry->StructType)
            {
                NewEntry.Entry->StructType = Entry.Entry->StructType;
                int32 StructSize = Entry.Entry->StructType->GetStructureSize();
                NewEntry.Entry->StructData.SetNum(StructSize);
                DeepCopyStruct(Entry.Entry->StructType, NewEntry.Entry->StructData.GetData(), Entry.Entry->StructData.GetData());
            }
        }

        Entries.Add(NewEntry);
    }

    // Mark the asset as modified
    MarkPackageDirty();
}

UVectorDatabase* UVectorDatabaseAsset::LoadToVectorDatabase() const
{
    UVectorDatabase* Database = NewObject<UVectorDatabase>();

    for (const FVectorDatabaseEntry& Entry : Entries)
    {
        if (Entry.Entry)
        {
            UVectorEntryWrapper* NewEntry = NewObject<UVectorEntryWrapper>(Database);
            NewEntry->StringValue = Entry.Entry->StringValue;
            NewEntry->ObjectValue = Entry.Entry->ObjectValue;
            NewEntry->EntryType = Entry.Entry->EntryType;
            NewEntry->Category = Entry.Entry->Category;

            if (Entry.Entry->EntryType == EEntryType::Struct && Entry.Entry->StructType)
            {
                NewEntry->StructType = Entry.Entry->StructType;
                int32 StructSize = Entry.Entry->StructType->GetStructureSize();
                NewEntry->StructData.SetNum(StructSize);
                DeepCopyStruct(Entry.Entry->StructType, NewEntry->StructData.GetData(), Entry.Entry->StructData.GetData());
            }

            Database->AddEntry(Entry.Vector, NewEntry, NewEntry->Category);
        }
    }

    return Database;
}

void UVectorDatabaseAsset::DeepCopyStruct(UScriptStruct* StructType, void* Dest, const void* Src)
{
    if (!StructType || !Dest || !Src)
    {
        UE_LOG(LogTemp, Error, TEXT("DeepCopyStruct: Invalid arguments"));
        return;
    }

    StructType->InitializeStruct(Dest);

    for (TFieldIterator<FProperty> It(StructType); It; ++It)
    {
        FProperty* Property = *It;

        if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
        {
            void* DestNestedStruct = StructProp->ContainerPtrToValuePtr<void>(Dest);
            const void* SrcNestedStruct = StructProp->ContainerPtrToValuePtr<const void>(Src);

            UScriptStruct* NestedStructType = StructProp->Struct;
            if (DestNestedStruct && SrcNestedStruct && NestedStructType)
            {
                DeepCopyStruct(NestedStructType, DestNestedStruct, SrcNestedStruct);
            }
        }
        else
        {
            void* DestValuePtr = Property->ContainerPtrToValuePtr<void>(Dest);
            const void* SrcValuePtr = Property->ContainerPtrToValuePtr<const void>(Src);
            Property->CopyCompleteValue(DestValuePtr, SrcValuePtr);
        }
    }
}

#if WITH_EDITOR
void UVectorDatabaseAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // You can add custom logic here if needed when properties are changed in the editor
}
#endif