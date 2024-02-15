# Loupe UX Json Server

## Overview

This Loupe UX Json server processes read and write requests from Lux clients* and processes the requests with a PLC. In other words, it acts as a middleman or gateway, translating the highlevel JSON requests into platform-specific data access.

At the present time, this server is capable of connection to Beckhoff PLCs, using TwinCAT 3's built-in [ADS server](https://www.beckhoff.com/en-us/products/automation/twincat/tc1xxx-twincat-3-base/tc1000.html). In the future, additional connection types may be added.



\* for more information on creating a Lux client for user interfaces, see [here](https://loupeteam.github.io/LoupeDocs/libraries/Loupe-UX.html).


## Getting Started

### Building and Running server for communication with Beckhoff TwinCAT 3 PLC 


#### Necessary Installs

* ADS 
    - For Windows
        - There are pre-built binaries available at "src\LuxServer\lib"
        - If you wish to build the library from source, the github repository is a submodule of this repository, and can be found [here](src\LuxServer\depends\ADS). The README in that repository contains instructions for building the library.
    - For Linux
        - you will need to build the ADS library from source. The github repository is a submodule of this repository, and can be found [here](src\LuxServer\depends\ADS). The README in that repository contains instructions for building the library.
        - After building the library, install it to your system.

* [CMake](https://cmake.org/download/) >= V3.27.1
    - Overview: CMake is a system for building C++ projects through whichever compiler you wish.
    - Install Recommendation: Add cmake to PATH so that `cmake` can be called from the command line

* (Optional) Visual Studio Community 2019. If this is not installed, you will need to modify the below commands to select the generator you wish to use.


#### Building server from source with Cmake 


1. Change directory to the following folder within the repo

```CMD
cd <path_to_repo>/src/LuxServer
```

2. Optionally, you can output a list of available generators in cmake. Note which generator you would like to use. Here we will use "Visual Studio 16 2019".

```CMD
cmake --help
```

3. Generate `cmakebuild` files for desired generator. The `-G` flag specifies which generator to use, `-S` specifies where the top level CMakeLists.txt file is located (should be in current directory), `-B` specifes where to put the generator files, and `-A` specifies the architecture of the output (32-bit is necessary here for compatibility with the ADS binaries).
NOTE: This could take a few minutes to complete.
```CMD
cmake -G "Visual Studio 16 2019" -S . -B ./cmakebuild -A win32
```

4. Finally, build the executable
```CMD
cmake --build ./cmakebuild --config Release --target server
```

5. Once building is complete with no errors, the executable is located at:
```CMD
.\cmakebuild\src\server\Release\server.exe
```

#### Configuring and Running the Server

1. Open `configuration.json`. It should be located in the current working directory (`<path_to_repo>/src/LuxServer/`)
2. Modify the JSON contents as necessary, setting netID of your PLC and ADS Port number. NOTE: By default, the Twincat ADS server will listen on port 851.
3. Run the the server with:
```CMD
.\cmakebuild\src\server\Release\server.exe
```
If everything has been set up correctly, your Lux client should now be able to interface with your Beckhoff TwinCat PLC.

### Running the Example project

The repo includes an example Lux client and a TwinCat PLC project that can be used to test the server

#### Get PLC running

1. Open the TwinCAT 3 project, located in `<repo>/example/TwinCat/`

2. Set Target System to `Local` and Activate Configuration.

#### Get the Loupe UX Server running

1. Work through the steps in the Getting Started section of this README.

2. Adapt the `configuration.json` file to point to the PLC (if running locally, netID should be `127.0.0.1.1.1`)

3. Upon running `server.exe`, you should see evidence that connection has occurred. If not, check that the TwinCAT PLC is running locally.

#### Get Lux client running

Follow the instructions in the README file in `<repo>/example/HMI/`


#### Final state
If everything has been set up correctly, the Lux client will be able to read and write variables located on the TwinCAT PLC.


### How to trigger a automatic release through GitHUB
Tagging any branch and pushing that tag to the origin will trigger a release.  The release will have the name 'Release ' + what ever text is in your tag name.  Example:  If you push a tag with the name 'v0.0.1' the release's name will be 'Release v0.0.1'.

A release will include a .zip file named 'luxserver.zip' with the executable for the server and configuration.json file. The source code for Linux and Windows are also included.