The parser library approaches the dependency resolution in a way similar to the mainframe. On mainframe, you would have to define the locations of your dependencies in a JCL file (more on JCL [here](https://www.ibm.com/support/knowledgecenter/zosbasics/com.ibm.zos.zjcl/zjclc_basicjclconcepts.htm)). As the user may want to include tens of dependencies for multiple open codes, a source code management tool called [Endevor](https://en.wikipedia.org/wiki/Endevor) groups these dependencies into so-called *processor groups*. Then, the user only has to assign a processor group to the open code and the Endevor does the dependency resolution for him.

To provide similar experience with local files, the parser library simulates this behavior. If the user wants to include dependencies in his project, he has to define 2 configuration files inside his workspace: *pgm\_conf.json* and *proc\_grps.json*. The workspace component of the parser library then processes the configurations, retrieving their values upon initialization. Moreover, each time a save command is issued on any configuration file, the configuration values are reloaded via `load_config` method.

### Processor groups

The proc\_grps configuration file contains a JSON array of possible processor groups, which consist of a name and an array of folder paths (may be relative to the root of the workspace). An example can be found in \[lst:proc\_grps\].

Whenever `load_config` is called, the workspace retrieves these processor groups from the configuration file and creates libraries. The libraries provide information about paths to their dependency files. During the parsing, the workspace retrieves the library corresponding to the provided processor group name and uses it to search for a wanted macro or copy file.

### Program configuration

The pgm\_conf configuration file contains a JSON array of program names (or wildcards \[section:wildcard\]), matched to their processor groups. It serves as a list of the HLASM open code files and states the libraries (in form of processor groups) that contain the dependencies of each open code. An example can be found in \[lst:pgm\_conf\].

From this configuration, the workspace simply remembers the processor group - open code mapping.

    	{
    	  "pgroups": [
    	    {
    	      "name":"GROUP1",
    	      "libs": [
    	        "ASMMAC/",
    	        "C:/SYS.ASMMAC"
    	      ]
    	    },
    	    {
    	      "name":"GROUP2",
    	      "libs": [
    	        "G2MAC/",
    	        "C:/SYS.ASMMAC"
    	      ]
    	    }
    	  ]
    	}
    	

    	{
    	  "pgms": [
    	    {
    	      "program": "source_code",
    	      "pgroup": "GROUP1"
    	    },
    	    {
    	      "program": "second_file",
    	      "pgroup": "GROUP2"
    	    },
    	  ]
    	}
    	