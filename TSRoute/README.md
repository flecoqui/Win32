

# TSROUTE

TSRoute is a tool which supports the following MPEG2-TS related features:
- TSRoute could be used to route MPEG2-TS Stream over UDP 
- TSRoute could be used to stream MPEG2-TS files over UDP (Unicast or Multicast)
- TSRoute could be used to capture MPEG2-TS traffic over UDP in a Transport Stream file
- TSRoute could be used to parse a MPEG2-TS file or stream.


System requirements
-----------------------------

**Client:** Windows XP, Windows 7, Windows 8.X, Windows 10, Windows Server 2003, Windows Server 2008, Windows Server 2012


Latest Build
------------

The latest build is available there in the [releases section] : (https://github.com/flecoqui/Win32/tree/master/TSRoute/Releases/LatestRelease.zip)
The latest build with TS Files is available there in the [releases section] : (https://github.com/flecoqui/Win32/tree/master/TSRoute/Releases/LatestReleaseWithTSFiles.zip)

How to use TSRoute
------------------

**Installing TSRoute:** Unzip the zip file with or without TS files on your machine running Windows in the folder where you want to install the application. The zip file contains the application TSRoute.Exe

**Using TSRoute to get help:** On your machine running Windows, press WINDOWS+R, the dialog box Run appears enter "cmd" in the Open edit field and click on the "OK" button. Enter command "cd <Installation Directory>" to change directory. Then enter command "tsrooute. exe -help"

**Using TSRoute to parse a TS file:** On your machine running Windows, in the command shell in the folder where TSRoute is installed enter command "tsroute.exe -parse -file <TS file path>"
 
**Using TSRoute to stream a TS file:** On your machine running Windows, in the command shell in the folder where TSRoute is installed enter command "tsroute.exe -stream -address <IPAddress>:<UDPPort> -interfaceaddress <NICCardIPAddress> -loop -updatetimestamps -file <TS file path>"

**Using TSRoute to capture a TS file:** On your machine running Windows, in the command shell in the folder where TSRoute is installed enter command "tsroute.exe -receive -address <IPAddress>:<UDPPort> -file <TS file path>"

**Using TSRoute to route a TS Stream:** On your machine running Windows, in the command shell in the folder where TSRoute is installed enter command "tsroute.exe -stream  -inputaddress <InputIPAddress>:<UDPPort> -address <IPAddress>:<UDPPort> -interfaceaddress <NICCardIPAddress> "

**Using TSRoute to install the TSRoute Service :** On your machine running Windows, in the command shell in the folder where TSRoute is installed enter command "tsroute.exe -install -xmlfile <XML configuration file>"

**Using TSRoute to start the TSRoute Service :** On your machine running Windows, in the command shell in the folder where TSRoute is installed enter command "tsroute.exe -start"

**Using TSRoute to stop the TSRoute Service :** On your machine running Windows, in the command shell in the folder where TSRoute is installed enter command "tsroute.exe -stop"


Build the sample
----------------

1. If you download the Win32 ZIP, be sure to unzip the entire archive, not just the folder with the sample you want to build. 
2. Start Microsoft Visual Studio 2015 and select **File** \> **Open** \> **Project/Solution**.
3. Starting in the folder where you unzipped the samples, go to the Win32 subfolder, then the subfolder TSRoute for this specific project. Double-click the Visual Studio 2015 Solution (.sln) file.
4. Press Ctrl+Shift+B, or select **Build** \> **Build Solution**.

Run the sample
--------------

The next steps depend on whether you just want to deploy the sample or you want to both deploy and run it.

**Deploying the sample**
1.  Select **Build** \> **Run**.

**Running the sample**
1.  To debug the sample and then run it, press F5 or select **Debug** \> **Start Debugging**. To run the sample without debugging, press Ctrl+F5 or select**Debug** \> **Start Without Debugging**.




