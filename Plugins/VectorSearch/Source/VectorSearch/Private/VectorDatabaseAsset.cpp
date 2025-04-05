#include "VectorDatabaseAsset.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

UVectorDatabaseAsset::UVectorDatabaseAsset()
{
    CreationDate = FDateTime::Now();
    LastModifiedDate = CreationDate;
    VectorDimension = 0;
}

void UVectorDatabaseAsset::PostInitProperties()
{
    Super::PostInitProperties();
    
    if (CreationDate.GetTicks() == 0)
    {
        CreationDate = FDateTime::Now();
        LastModifiedDate = CreationDate;
    }
}

void UVectorDatabaseAsset::SaveFromVectorDatabase(UVectorDatabase* Database)
{
    if (!Database)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid Database provided to SaveFromVectorDatabase"));
        return;
    }

    Entries.Empty();
    Categories.Empty();
    TArray<FVectorDatabaseEntry> AllEntries = Database->GetAllVectorEntries(TArray<FString>());

    // Update metadata
    LastModifiedDate = FDateTime::Now();
    VectorDimension = AllEntries.Num() > 0 ? AllEntries[0].Vector.Num() : 0;

    // Process all entries
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

            // Add category to our list if it's not empty and not already included
            if (!NewEntry.Entry->Category.IsEmpty() && !Categories.Contains(NewEntry.Entry->Category))
            {
                Categories.Add(NewEntry.Entry->Category);
            }

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

bool UVectorDatabaseAsset::SaveToFile(const FString& FilePath)
{
    // Create a JSON object to store our data
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    
    // Add metadata
    JsonObject->SetStringField(TEXT("DatabaseName"), DatabaseName);
    JsonObject->SetStringField(TEXT("Description"), Description);
    JsonObject->SetStringField(TEXT("CreationDate"), CreationDate.ToString());
    JsonObject->SetStringField(TEXT("LastModifiedDate"), LastModifiedDate.ToString());
    JsonObject->SetNumberField(TEXT("VectorDimension"), VectorDimension);
    
    // Add categories
    TArray<TSharedPtr<FJsonValue>> CategoriesArray;
    for (const FString& Category : Categories)
    {
        CategoriesArray.Add(MakeShared<FJsonValueString>(Category));
    }
    JsonObject->SetArrayField(TEXT("Categories"), CategoriesArray);
    
    // Add entries
    TArray<TSharedPtr<FJsonValue>> EntriesArray;
    for (const FVectorDatabaseEntry& Entry : Entries)
    {
        TSharedPtr<FJsonObject> EntryObject = MakeShared<FJsonObject>();
        
        // Add vector data
        TArray<TSharedPtr<FJsonValue>> VectorArray;
        for (float Value : Entry.Vector)
        {
            VectorArray.Add(MakeShared<FJsonValueNumber>(Value));
        }
        EntryObject->SetArrayField(TEXT("Vector"), VectorArray);
        
        // Add entry wrapper data if it exists
        if (Entry.Entry)
        {
            EntryObject->SetStringField(TEXT("EntryType"), FString::FromInt(static_cast<int32>(Entry.Entry->EntryType)));
            EntryObject->SetStringField(TEXT("Category"), Entry.Entry->Category);
            
            if (Entry.Entry->EntryType == EEntryType::String)
            {
                EntryObject->SetStringField(TEXT("StringValue"), Entry.Entry->StringValue);
            }
            else if (Entry.Entry->EntryType == EEntryType::Struct && Entry.Entry->StructType)
            {
                EntryObject->SetStringField(TEXT("StructTypeName"), Entry.Entry->StructType->GetName());
                
                // Convert struct data to base64 string for storage
                FString Base64StructData = FBase64::Encode(Entry.Entry->StructData.GetData(), Entry.Entry->StructData.Num());
                EntryObject->SetStringField(TEXT("StructData"), Base64StructData);
            }
        }
        
        EntriesArray.Add(MakeShared<FJsonValueObject>(EntryObject));
    }
    JsonObject->SetArrayField(TEXT("Entries"), EntriesArray);
    
    // Convert to string and save to file
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    
    return FFileHelper::SaveStringToFile(OutputString, *FilePath);
}

bool UVectorDatabaseAsset::LoadFromFile(const FString& FilePath)
{
    // Read the file
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *FilePath);
        return false;
    }
    
    // Parse the JSON
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON from file: %s"), *FilePath);
        return false;
    }
    
    // Clear existing data
    Entries.Empty();
    Categories.Empty();
    
    // Load metadata
    DatabaseName = JsonObject->GetStringField(TEXT("DatabaseName"));
    Description = JsonObject->GetStringField(TEXT("Description"));
    
    FString CreationDateStr = JsonObject->GetStringField(TEXT("CreationDate"));
    FDateTime::Parse(CreationDateStr, CreationDate);
    
    FString LastModifiedDateStr = JsonObject->GetStringField(TEXT("LastModifiedDate"));
    FDateTime::Parse(LastModifiedDateStr, LastModifiedDate);
    
    VectorDimension = JsonObject->GetIntegerField(TEXT("VectorDimension"));
    
    // Load categories
    const TArray<TSharedPtr<FJsonValue>>* CategoriesArray;
    if (JsonObject->TryGetArrayField(TEXT("Categories"), CategoriesArray))
    {
        for (const TSharedPtr<FJsonValue>& CategoryValue : *CategoriesArray)
        {
            Categories.Add(CategoryValue->AsString());
        }
    }
    
    // Load entries
    const TArray<TSharedPtr<FJsonValue>>* EntriesArray;
    if (JsonObject->TryGetArrayField(TEXT("Entries"), EntriesArray))
    {
        for (const TSharedPtr<FJsonValue>& EntryValue : *EntriesArray)
        {
            TSharedPtr<FJsonObject> EntryObject = EntryValue->AsObject();
            
            FVectorDatabaseEntry NewEntry;
            
            // Load vector data
            const TArray<TSharedPtr<FJsonValue>>* VectorArray;
            if (EntryObject->TryGetArrayField(TEXT("Vector"), VectorArray))
            {
                for (const TSharedPtr<FJsonValue>& VectorValue : *VectorArray)
                {
                    NewEntry.Vector.Add(VectorValue->AsNumber());
                }
            }
            
            // Load entry wrapper data
            if (EntryObject->HasField(TEXT("EntryType")))
            {
                NewEntry.Entry = NewObject<UVectorEntryWrapper>(this);
                NewEntry.Entry->EntryType = static_cast<EEntryType>(FCString::Atoi(*EntryObject->GetStringField(TEXT("EntryType"))));
                NewEntry.Entry->Category = EntryObject->GetStringField(TEXT("Category"));
                
                if (NewEntry.Entry->EntryType == EEntryType::String)
                {
                    NewEntry.Entry->StringValue = EntryObject->GetStringField(TEXT("StringValue"));
                }
                else if (NewEntry.Entry->EntryType == EEntryType::Struct)
                {
                    FString StructTypeName = EntryObject->GetStringField(TEXT("StructTypeName"));
                    NewEntry.Entry->StructType = FindObject<UScriptStruct>(nullptr, *StructTypeName);
                    
                    if (NewEntry.Entry->StructType)
                    {
                        FString Base64StructData = EntryObject->GetStringField(TEXT("StructData"));
                        TArray<uint8> DecodedData;
                        FBase64::Decode(Base64StructData, DecodedData);
                        NewEntry.Entry->StructData = DecodedData;
                    }
                }
            }
            
            Entries.Add(NewEntry);
        }
    }
    
    // Mark the asset as modified
    MarkPackageDirty();
    
    return true;
}

TArray<FString> UVectorDatabaseAsset::GetUniqueCategories() const
{
    return Categories;
}

int32 UVectorDatabaseAsset::GetEntryCountForCategory(const FString& Category) const
{
    int32 Count = 0;
    for (const FVectorDatabaseEntry& Entry : Entries)
    {
        if (Entry.Entry && Entry.Entry->Category == Category)
        {
            Count++;
        }
    }
    return Count;
}

TArray<FVectorDatabaseEntry> UVectorDatabaseAsset::GetEntriesForCategory(const FString& Category) const
{
    TArray<FVectorDatabaseEntry> Result;
    for (const FVectorDatabaseEntry& Entry : Entries)
    {
        if (Entry.Entry && Entry.Entry->Category == Category)
        {
            Result.Add(Entry);
        }
    }
    return Result;
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

    // Update LastModifiedDate when properties change
    LastModifiedDate = FDateTime::Now();
}
#endif