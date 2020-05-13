<img src="img/ws_mngr_arch.svg" alt="Architecture of workspace manager." />

The architecture (visualized in picture above) of the parser library is organized into the following components:

- **Workspace manager API**  
The workspace manager provides API for handling various workspace management (e.g. add new workspace), LSP and DAP requests. It may hold multiple workspaces and calls file manager to handle changes in the workspace files.

- **Workspace representation**  
The representation of workspace deals with the relations between its files (dependencies) upon parse request and propagates the parsing further into analyzer. It also retrieves data from the configuration files and it is used for resolving dependency searches by implementing parse library provider.

- **Processor group representation**  
The representation of a processor group uses the API of libraries to search for their dependencies. Currently, we only support local libraries, which utilize the file manager for their file information retrieval.

- **File manager**  
The file manager is used by multiple components to handle file management and file searches. It also distinguishes and does conversions between regular files and processor files, which may be used for parsing.

- **[[Analyzer]]**  
The analyzer accepts a file along with the information needed for dependency resolution, syntactically and semantically processes it and fills the context tables. The component is further explained in \[chap:analyzer\].

The technical details of each component are further explained in the following sections.

Workspace representation
------------------------

In VSCode, as in many other editors, a grouping of files for a single project is called the *workspace*. This notion simplifies the workflow with the project as all the needed files are concentrated in a single folder. For example, the relative paths to the workspace may be used instead of the absolute ones or custom settings may be applied to the particular project/workspace.

As the parser library follows the LSP, it also incorporates the notion of files organized into workspaces. Therefore, it has its own representation of a workspace.

The representation of workspace is used by the workspace manager to handle various changes in the workspace. The workspace manager propagates LSP requests and notifications coming from the language server to the corresponding workspace and retrieves the results from it via the registered observers.

The workspace component uses the file manager for the file searches, retrieves the values from the configuration files and creates processor groups and is capable of resolving dependencies.

Due to the possibility to include files, the workspace maintains a list of dependants, which are active dependencies of another workspace files. The list of dependants is needed, for example, in case the user changes contents of a macro that is used by multiple open code files, as all of them would have to be reparsed.

The core of the workspace is its `parse_file` method. As addition to the parsing part, it also ensures that the file to be parsed, its dependencies and dependants provide consistent results. The method works as follows:

1.  It checks whether the parsed file is a configuration file. If so, the workspace reloads the configuration values and reparses all dependants in the workspace.

2.  In case the parsed file is not a configuration file, it creates a list of all files to be parsed. This list consists of files depending on the parsed file and the parsed file itself.

3.  The method reparses the files in this list and creates new dependants, based on the dependencies reported from the parsing.

4.  It checks for the files that are no longer in use (former dependencies) and closes them.

The workspace also ensures the correct closure of the file via `didClose` method. It works as follows:

-   If the closed file is a dependency of some other file, it cannot be removed completely from the file manager, as it is still in use. The file manager is rather notified that the file was closed in the editor.

-   If it is not a dependency, the method checks for its dependants and closes them.

File Representation
-------------------

The file manager handles all file-related requests across different workspaces. It distinguishes between regular, non-HLASM files and processable, HLASM files by using different representations.

The representation of a regular file (called *file*) is capable of providing its file names, its contents and changing its state upon file-oriented LSP requests, i.e didChange, didClose and didOpen.

The representation of processor files is defined by *processor_file* class, which derives from both *file* and *processor* abstract classes. The *processor* is an interface which is capable of actual processing (parsing). Its only implementation is processor file.

When the `parse` method is invoked, the processor file initializes new analyzer, uses it for the parsing and rebuilds its dependencies list, closing the unwanted ones. When the parsing is finished, it keeps the instance of the analyzer and provides its parsing results when requested.

Dependency resolution
---------------------

Whenever a code from a different file is to be included, either via `COPY` instruction or a macro call, it is necessary to find the desired file first. During the parsing, the representation of libraries are already created accordingly to the configurations. However, there is also a need for components that would resolve the dependency by finding the the corresponding library and parse it.

The *parse\_lib\_provider* interface exists for this purpose. Whenever a component is to be used for the dependency resolution, it implements this interface.

The name of the needed file, the current context tables and the library data (the currently used type of processing) are passed to the `parse_library` method of the *parse_lib_provider* interface. The method finds the library file (i.e. a macro or COPY file) with the specified name and parses it with the given context.

The workspace is the most important implementation of the `parse_lib_provider` interface. It provides libraries based on the processor groups configuration described in [[Libraries configuration]].

Diagnostics
-----------

A diagnostic is used to indicate a problem with source files, such as a compiler error or a warning. Some diagnostics are created in almost every component of the parser when it finds a problem with a source code. Diagnostics are also used in workspace to indicate problems with configuration files. After each parsing, we need to collect all the diagnostics from all the instances of all the components and pass them to the language server.

The components capable of collecting the diagnostics are organized in a tree where the root is the workspace manager. Starting from the root, each component collects the diagnostics of those children that are again capable of collecting or generating diagnostics.

To enforce this behavior, all of these components implement the *diagnosable* interface. Its functionality is simple, it is used to add diagnostics, show his own and collect them from other diagnosable members. Each component that implements the interface is required to collect diagnostics from diagnosable objects it owns. In the result, one call of `collect_diags` from the root of the tree collects all diagnostics that were created since last such call.

The diagnosable hierarchy of workspace manager component is shown in the following picture:

<img src="img/diagnosable_hierarchy.svg" alt="Hierarchy of diagnostics collection in the workspace manager component" />
