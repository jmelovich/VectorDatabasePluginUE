# Vector Database Plugin for Unreal Engine 5

This plugin adds a simplified Vector Database type to Unreal Engine 5, designed with blueprints in mind. It provides functionality for creating, managing, and querying vector databases with support for various data types and distance metrics.

## WARNING: WORK IN PROGRESS

This is still in an early state, so don't be surprised if you experience crashes. I am making this plugin for myself for some particular projects, but I figured it'd be nice to share it. If you do try it out and experience issues, feel free to open a new issue with any log information that could be helpful.

## WARNING: Actor References in Structs

The vector database can store structs that contain actor references. However, since actor references are memory addresses, saving these databases to assets or JSON files will invalidate these references. We strongly recommend:

1. Avoiding storing actor references in structs intended for database persistence
2. If actor references must be used, implement validation checks when retrieving them from the database to prevent engine crashes
3. Consider using alternative identification methods (like unique IDs or names) instead of direct actor references for persistent storage

Improper handling of invalid actor references can lead to engine instability or crashes.

## Installation

This repo includes the entire project (UE 5.4) I am using to test & develop this plugin. You do **NOT** need to clone this entire project. To install the plugin, follow these steps:

1. Download the Plugins/VectorSearch folder and place it in your project's Plugins folder (create this if it doesn't exist)
   - At this point in time, you can attempt to open the project. There is a chance it will work already.
2. Right click on your .uproject file and click 'Generate Visual Studio project files'
3. Open the project solution (.sln file) and build the project, ensure there are no compilation errors
   - If you experience any compilation errors, please post the build log in the issues tab
4. Open your project and search for 'Vector Database' in a blueprint graph to see available functions

This installation guide assumes you have a C++ project, as I have not been able to test compiling this plugin in a blueprint project. If you have a blueprint project, you can quickly & easily create a blank C++ project and do the compilation there, I believe copying/pasting from that project would work.

## Features

### Core Functionality
- Create vector databases using the 'Create Vector Database' node
- Add entries of different types:
  - Strings
  - Objects
  - Structs (with support for most variable types - see actor reference warning above)
- Retrieve entries using various matching methods:
  - Get Top N Matches (for strings and objects)
  - Get Top N Struct Matches (with wildcard output for structs)
  - Get Detailed Top N Matches (returns vectors, distances, and values)
- Remove entries based on vector matches with optional range and multiple occurrence removal
- Database statistics and management functions

### Supported Distance Metrics
- Euclidean Distance
- Cosine Similarity
- Manhattan Distance
- Dot Product

### Vector Generation
- Built-in OpenAI Embedding generation support
  - Configurable API endpoint, model, and API key
  - Returns float arrays compatible with the vector database

### Data Management
- Category support for organizing entries
- Database statistics including entry counts by type
- Vector normalization
- Database persistence through VectorDatabaseAsset
  - Save/load to/from files (note actor reference limitations)
  - Asset-based storage in Unreal Engine (note actor reference limitations)

### Blueprint Integration
- Comprehensive blueprint function library
- Pure functions for state queries
- Custom thunk support for struct handling
- Latent action support for async operations

## How does it work?

### Database Operations
1. Create a new vector database using 'Create New Vector Database'
2. Add entries using type-specific nodes:
   - AddStringEntryToVectorDatabase
   - AddObjectEntryToVectorDatabase
   - AddStructEntryToVectorDatabase (beware of actor references)
3. Query the database:
   - GetTopNStringMatches
   - GetTopNObjectMatches
   - GetTopNStructMatches
   - GetTopNEntriesWithDetails
4. Manage the database:
   - Remove entries with RemoveEntryFromVectorDatabase
   - Clear with ClearVectorDatabase
   - Normalize vectors with NormalizeVectorsInDatabase

### Vector Database Asset
- Save databases to assets with SaveFromVectorDatabase
- Load databases from assets with LoadToVectorDatabase
- File-based persistence with SaveToFile/LoadFromFile
- Note: Validate any actor references after loading due to potential invalidation

### OpenAI Integration
- Generate embeddings using GenerateOpenAIEmbedding
- Configure via FOpenAIConfig struct with endpoint, model, and API key

## Technical Details
- Type-agnostic database implementation
- Supports vectors of different dimensions
- Category-based filtering
- Comprehensive metadata support
- Struct handling through wrapper classes (with actor reference considerations)
- Consistent vector dimension checking

## Usage Notes
- Struct handling requires connecting outputs through For Each loops
- Distance metric can be set per database
- All match functions respect vector dimensions and entry types
- Categories can be used to filter queries
- Database stats provide detailed breakdowns of contents
- Always validate actor references retrieved from persisted databases

## Future Plans
- Additional distance metrics
- Performance optimizations
- More vector generation methods
- Enhanced blueprint documentation
- Better handling of actor references in persistent storage
