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
                
                // Use our safe DeepCopyStruct method to handle actor references
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
                
                // Use our safe DeepCopyStruct method to handle actor references
                DeepCopyStruct(Entry.Entry->StructType, NewEntry->StructData.GetData(), Entry.Entry->StructData.GetData());
            }

            Database->AddEntry(Entry.Vector, NewEntry, NewEntry->Category);
        }
    }

    return Database;
}

// Helper function to safely serialize struct data to JSON
void SerializeStructToJson(UScriptStruct* StructType, const void* StructData, TSharedPtr<FJsonObject>& JsonObject)
{
    if (!StructType || !StructData || !JsonObject.IsValid())
    {
        return;
    }

    for (TFieldIterator<FProperty> It(StructType); It; ++It)
    {
        FProperty* Property = *It;
        FString PropertyName = Property->GetName();

        if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
        {
            const void* NestedStructData = StructProp->ContainerPtrToValuePtr<const void>(StructData);
            UScriptStruct* NestedStructType = StructProp->Struct;
            
            if (NestedStructData && NestedStructType)
            {
                TSharedPtr<FJsonObject> NestedJsonObject = MakeShared<FJsonObject>();
                SerializeStructToJson(NestedStructType, NestedStructData, NestedJsonObject);
                JsonObject->SetObjectField(PropertyName, NestedJsonObject);
            }
        }
        else if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
        {
            // Check if this is an actor reference
            if (ObjectProp->PropertyClass && ObjectProp->PropertyClass->IsChildOf(AActor::StaticClass()))
            {
                // Get the actor reference
                UObject* ObjRef = ObjectProp->GetPropertyValue_InContainer(StructData);
                AActor* ActorRef = Cast<AActor>(ObjRef);
                
                // If there's an actor reference, store a safe representation
                if (ActorRef)
                {
                    // Store actor name and path as a string for reference
                    FString ActorPath = ActorRef->GetPathName();
                    JsonObject->SetStringField(PropertyName + TEXT("_ActorPath"), ActorPath);
                    
                    // Set the actual property to null in the JSON
                    JsonObject->SetField(PropertyName, MakeShared<FJsonValueNull>());
                    
                    UE_LOG(LogTemp, Warning, TEXT("Actor reference detected in struct '%s' property '%s'. Storing actor path instead: %s"), 
                           *StructType->GetName(), *PropertyName, *ActorPath);
                }
                else
                {
                    // If the reference is already null, store null
                    JsonObject->SetField(PropertyName, MakeShared<FJsonValueNull>());
                }
            }
            else
            {
                // For non-actor object references, try to serialize normally
                UObject* ObjectRef = ObjectProp->GetPropertyValue_InContainer(StructData);
                if (ObjectRef)
                {
                    JsonObject->SetStringField(PropertyName, ObjectRef->GetPathName());
                }
                else
                {
                    JsonObject->SetField(PropertyName, MakeShared<FJsonValueNull>());
                }
            }
        }
        else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
        {
            bool BoolValue = BoolProp->GetPropertyValue_InContainer(StructData);
            JsonObject->SetBoolField(PropertyName, BoolValue);
        }
        else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
        {
            int32 IntValue = IntProp->GetPropertyValue_InContainer(StructData);
            JsonObject->SetNumberField(PropertyName, IntValue);
        }
        else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
        {
            float FloatValue = FloatProp->GetPropertyValue_InContainer(StructData);
            JsonObject->SetNumberField(PropertyName, FloatValue);
        }
        else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
        {
            double DoubleValue = DoubleProp->GetPropertyValue_InContainer(StructData);
            JsonObject->SetNumberField(PropertyName, DoubleValue);
        }
        else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
        {
            FString StrValue = StrProp->GetPropertyValue_InContainer(StructData);
            JsonObject->SetStringField(PropertyName, StrValue);
        }
        else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
        {
            FName NameValue = NameProp->GetPropertyValue_InContainer(StructData);
            JsonObject->SetStringField(PropertyName, NameValue.ToString());
        }
        else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
        {
            // Handle arrays (simplified - only handles basic types)
            TArray<TSharedPtr<FJsonValue>> JsonArray;
            
            if (FIntProperty* InnerIntProp = CastField<FIntProperty>(ArrayProp->Inner))
            {
                TArray<int32> IntArray;
                FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(StructData));
                for (int32 i = 0; i < ArrayHelper.Num(); ++i)
                {
                    int32 Value = InnerIntProp->GetPropertyValue(ArrayHelper.GetRawPtr(i));
                    JsonArray.Add(MakeShared<FJsonValueNumber>(Value));
                }
            }
            else if (FFloatProperty* InnerFloatProp = CastField<FFloatProperty>(ArrayProp->Inner))
            {
                TArray<float> FloatArray;
                FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(StructData));
                for (int32 i = 0; i < ArrayHelper.Num(); ++i)
                {
                    float Value = InnerFloatProp->GetPropertyValue(ArrayHelper.GetRawPtr(i));
                    JsonArray.Add(MakeShared<FJsonValueNumber>(Value));
                }
            }
            else if (FStrProperty* InnerStrProp = CastField<FStrProperty>(ArrayProp->Inner))
            {
                TArray<FString> StrArray;
                FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(StructData));
                for (int32 i = 0; i < ArrayHelper.Num(); ++i)
                {
                    FString Value = InnerStrProp->GetPropertyValue(ArrayHelper.GetRawPtr(i));
                    JsonArray.Add(MakeShared<FJsonValueString>(Value));
                }
            }
            
            JsonObject->SetArrayField(PropertyName, JsonArray);
        }
    }
}

// Helper function to safely deserialize JSON to struct data
void DeserializeJsonToStruct(UScriptStruct* StructType, void* StructData, const TSharedPtr<FJsonObject>& JsonObject)
{
    if (!StructType || !StructData || !JsonObject.IsValid())
    {
        return;
    }

    for (TFieldIterator<FProperty> It(StructType); It; ++It)
    {
        FProperty* Property = *It;
        FString PropertyName = Property->GetName();

        if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
        {
            void* NestedStructData = StructProp->ContainerPtrToValuePtr<void>(StructData);
            UScriptStruct* NestedStructType = StructProp->Struct;
            
            if (NestedStructData && NestedStructType)
            {
                const TSharedPtr<FJsonObject>* NestedJsonObject;
                if (JsonObject->TryGetObjectField(PropertyName, NestedJsonObject))
                {
                    DeserializeJsonToStruct(NestedStructType, NestedStructData, *NestedJsonObject);
                }
            }
        }
        else if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
        {
            // Check if this is an actor reference
            if (ObjectProp->PropertyClass && ObjectProp->PropertyClass->IsChildOf(AActor::StaticClass()))
            {
                // For actor references, we'll set them to null
                ObjectProp->SetPropertyValue_InContainer(StructData, nullptr);
                
                // Log a warning about the actor reference
                UE_LOG(LogTemp, Warning, TEXT("Actor reference in struct '%s' property '%s' was set to null during deserialization."), 
                       *StructType->GetName(), *PropertyName);
            }
            else
            {
                // For non-actor object references, try to deserialize normally
                FString ObjectPath;
                if (JsonObject->TryGetStringField(PropertyName, ObjectPath))
                {
                    // Try to find the object by path
                    UObject* ObjectRef = FindObject<UObject>(nullptr, *ObjectPath);
                    if (ObjectRef)
                    {
                        ObjectProp->SetPropertyValue_InContainer(StructData, ObjectRef);
                    }
                    else
                    {
                        ObjectProp->SetPropertyValue_InContainer(StructData, nullptr);
                    }
                }
                else
                {
                    ObjectProp->SetPropertyValue_InContainer(StructData, nullptr);
                }
            }
        }
        else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
        {
            bool BoolValue;
            if (JsonObject->TryGetBoolField(PropertyName, BoolValue))
            {
                BoolProp->SetPropertyValue_InContainer(StructData, BoolValue);
            }
        }
        else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
        {
            int32 IntValue;
            if (JsonObject->TryGetNumberField(PropertyName, IntValue))
            {
                IntProp->SetPropertyValue_InContainer(StructData, IntValue);
            }
        }
        else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
        {
            float FloatValue;
            if (JsonObject->TryGetNumberField(PropertyName, FloatValue))
            {
                FloatProp->SetPropertyValue_InContainer(StructData, FloatValue);
            }
        }
        else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
        {
            double DoubleValue;
            if (JsonObject->TryGetNumberField(PropertyName, DoubleValue))
            {
                DoubleProp->SetPropertyValue_InContainer(StructData, DoubleValue);
            }
        }
        else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
        {
            FString StrValue;
            if (JsonObject->TryGetStringField(PropertyName, StrValue))
            {
                StrProp->SetPropertyValue_InContainer(StructData, StrValue);
            }
        }
        else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
        {
            FString NameValue;
            if (JsonObject->TryGetStringField(PropertyName, NameValue))
            {
                NameProp->SetPropertyValue_InContainer(StructData, FName(*NameValue));
            }
        }
        else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
        {
            // Handle arrays (simplified - only handles basic types)
            const TArray<TSharedPtr<FJsonValue>>* JsonArray;
            if (JsonObject->TryGetArrayField(PropertyName, JsonArray))
            {
                if (FIntProperty* InnerIntProp = CastField<FIntProperty>(ArrayProp->Inner))
                {
                    FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(StructData));
                    ArrayHelper.EmptyAndAddUninitializedValues(JsonArray->Num());
                    
                    for (int32 i = 0; i < JsonArray->Num(); ++i)
                    {
                        if ((*JsonArray)[i].IsValid() && (*JsonArray)[i]->Type == EJson::Number)
                        {
                            InnerIntProp->SetPropertyValue(ArrayHelper.GetRawPtr(i), (*JsonArray)[i]->AsNumber());
                        }
                    }
                }
                else if (FFloatProperty* InnerFloatProp = CastField<FFloatProperty>(ArrayProp->Inner))
                {
                    FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(StructData));
                    ArrayHelper.EmptyAndAddUninitializedValues(JsonArray->Num());
                    
                    for (int32 i = 0; i < JsonArray->Num(); ++i)
                    {
                        if ((*JsonArray)[i].IsValid() && (*JsonArray)[i]->Type == EJson::Number)
                        {
                            InnerFloatProp->SetPropertyValue(ArrayHelper.GetRawPtr(i), (*JsonArray)[i]->AsNumber());
                        }
                    }
                }
                else if (FStrProperty* InnerStrProp = CastField<FStrProperty>(ArrayProp->Inner))
                {
                    FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(StructData));
                    ArrayHelper.EmptyAndAddUninitializedValues(JsonArray->Num());
                    
                    for (int32 i = 0; i < JsonArray->Num(); ++i)
                    {
                        if ((*JsonArray)[i].IsValid() && (*JsonArray)[i]->Type == EJson::String)
                        {
                            InnerStrProp->SetPropertyValue(ArrayHelper.GetRawPtr(i), (*JsonArray)[i]->AsString());
                        }
                    }
                }
            }
        }
    }
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
                
                // Use our safe serialization function for struct data
                TSharedPtr<FJsonObject> StructJsonObject = MakeShared<FJsonObject>();
                SerializeStructToJson(Entry.Entry->StructType, Entry.Entry->StructData.GetData(), StructJsonObject);
                
                // Convert struct JSON to string
                FString StructJsonString;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&StructJsonString);
                FJsonSerializer::Serialize(StructJsonObject.ToSharedRef(), Writer);
                
                // Store the struct JSON as a string
                EntryObject->SetStringField(TEXT("StructData"), StructJsonString);
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
                        // Get the struct data as a JSON string
                        FString StructJsonString = EntryObject->GetStringField(TEXT("StructData"));
                        
                        // Parse the struct JSON
                        TSharedPtr<FJsonObject> StructJsonObject;
                        TSharedRef<TJsonReader<>> StructReader = TJsonReaderFactory<>::Create(StructJsonString);
                        if (FJsonSerializer::Deserialize(StructReader, StructJsonObject))
                        {
                            // Allocate memory for the struct
                            int32 StructSize = NewEntry.Entry->StructType->GetStructureSize();
                            NewEntry.Entry->StructData.SetNum(StructSize);
                            
                            // Initialize the struct
                            NewEntry.Entry->StructType->InitializeStruct(NewEntry.Entry->StructData.GetData());
                            
                            // Deserialize the JSON to the struct
                            DeserializeJsonToStruct(NewEntry.Entry->StructType, NewEntry.Entry->StructData.GetData(), StructJsonObject);
                        }
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
        else if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
        {
            // Check if this is an actor reference
            if (ObjectProp->PropertyClass && ObjectProp->PropertyClass->IsChildOf(AActor::StaticClass()))
            {
                // Get the actor reference
                UObject* ObjRef = ObjectProp->GetPropertyValue_InContainer(Src);
                AActor* ActorRef = Cast<AActor>(ObjRef);
                
                // If there's an actor reference, log a warning and skip it
                if (ActorRef)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Actor reference detected in struct '%s' property '%s'. Actor references cannot be safely serialized and will be skipped."), 
                           *StructType->GetName(), *Property->GetName());
                    
                    // Set the destination to null to avoid crashes
                    ObjectProp->SetPropertyValue_InContainer(Dest, nullptr);
                }
                else
                {
                    // If the reference is already null, just copy it
                    void* DestValuePtr = Property->ContainerPtrToValuePtr<void>(Dest);
                    const void* SrcValuePtr = Property->ContainerPtrToValuePtr<const void>(Src);
                    Property->CopyCompleteValue(DestValuePtr, SrcValuePtr);
                }
            }
            else
            {
                // For non-actor object references, copy normally
                void* DestValuePtr = Property->ContainerPtrToValuePtr<void>(Dest);
                const void* SrcValuePtr = Property->ContainerPtrToValuePtr<const void>(Src);
                Property->CopyCompleteValue(DestValuePtr, SrcValuePtr);
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