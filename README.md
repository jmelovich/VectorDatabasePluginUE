# Vector Database Plugin for Unreal Engine 5

This plugin adds a simplified Vector Database type to Unreal Engine 5, designed with blueprints in mind. 
## WARNING: WORK IN PROGRESS

This is still in an early state, so don't be surprised if you experience crashes. I am making this plugin for myself for some particular projects, but I figured it'd be nice to share it. If you do try it out and experience issues, feel free to open a new issue with any log information that could be helpful.

## Installation

This repo includes the entire project (UE 5.4.2) I am using to test & develop this plugin. You do **NOT** need to clone this entire project. To install the plugin, follow these steps:

1. Download the Plugins/VectorSearch folder and place it in your project's Plugins folder (create this if it doesn't exist)
   - At this point in time, you can attempt to open the project. There is a chance it will work already.
2. Right click on your .uproject file and click 'Generate Visual Studio project files'
3. Open the project solution (.sln file) and build the project, ensure there are no compilation errors
   - if you experience any compilation issues, please post the build log in the issues tab
4. Open your project and search for 'Vector Database' in a blueprint graph to see available functions

This installation guide assumes you have a C++ project, as I have not been able to test compiling this plugin in a blueprint project. If you have a blueprint project, you can quickly & easily create a blank C++ project and do the compilation there, I believe copying/pasting from that project would work.

## How does it work?
- Using the 'Create New Vector Database' node, you can initialize a new vector database and then save a reference to it as a variable.
- Using this object as the target, you can use the 'Add Entry to Vector Database' nodes to add an entry (and its corresponding vector-an array of floats) of any data type (the plugin currently supports String, Object, and Struct entries-  Structs can contain most variable types however, the String and Object entries really only exist for simplicity in some use cases)
- To retrieve entries from the database, you can use the 'Get Top N Matches' nodes. These take in the vector database object, a vector (array of floats), and an 'N' value (the amount of matches to return). These nodes then calculate the Top N entries in the database, based off each entry's vector distance from the input vector.
  - The 'Get Top N Struct Matches' has a wildcard output, to get your struct back from it connect a variable-setter or struct-breaker off of a for each loop, then connect the for each loop array input to the 'Get Top N Struct Matches' wildcard output- the pin type should automatically update.
  - There is also a 'Get Detailed Top N Matches' which will return a struct that contains each found entry's vector, distance, and then a wrapper object from where its value can get gotten from supplied pure functions (except structs, which have an impure function that takes in the whole output struct as its input)
  - All of the 'Get Top N Matches' functions will only consider vector database entries with a vector matching the dimensions of the input vector, and with entry types matching the node type. This is because the Vector Databases are data type agnostic- you can put any number or combination of strings, objects, or structs into a database (with vectors of different dimensions too, if you'd like) and it will work- because the get functions only consider entries relevant to them (meaning if you pass in a 700dimension vector, any entries with a vector of different dimensions aren't considered). There is a slight penalty to doing this, in that the getter functions still need to iterate over *every* entry in the array, regardless of whether or not you want to consider them.
- To remove entries from a database, you can use the 'Remove Entry from Vector Database' node which takes in an input vector and removes any matches. If bRemoveAllOccurences is set to true. then any entry with a matching vector will be removed. There is also a 'RemovalRange' float input, which if set to anything above 0, will remove any matches within (or at) that given distance from the input vector. 
- Additionally there are pure nodes to get the entry count in the database (as well as seperate getter nodes for getting string entry count, object entry count, and struct entry counts).
