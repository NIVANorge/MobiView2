# MobiView2
GUI for [Mobius 2](https://nivanorge.github.io/Mobius2/)

## Building
We don't have a portable build system for MobiView2 yet. See the [Mobius2 documentation](https://nivanorge.github.io/Mobius2/gettingstarted/gettingstarted.html) for how to get the MobiView2 binaries.


### If you really want to set up the build system on Windows from scratch, follow the below steps. We have not tried to build the GUI on Linux yet:

- Clone https://nivanorge.github.io/Mobius2/ to C:/Data/Mobius2/
- Follow the "Get LLVM" and "Get OpenXLSX" steps from here: https://github.com/NIVANorge/Mobius2/blob/main/HOW_TO_BUILD.md . It is more convenient if these are placed in C:/Data/llvm-project/ and C:/Data/OpenXLSX/ respectively.
- Download [dlib](http://dlib.net/). Extract the archive and copy the "dlib" subfolder (i.e. the folder that directly contains "global_optimization.h") into the Mobius2/third_party/ folder.
- Get the Ultimate++ development environment from here: https://www.ultimatepp.org/www$uppweb$download$en-us.html . Put it in C:/Data/upp/
- Clone this repository (https://github.com/NIVANorge/MobiView2) into upp/MyApps/MobiView2/
- Run upp/theide.exe
- Open the MobiView2 project.
- Check that the build method at the top is set to a variant of MSVS x64 Release, e.g. MSVS26x64 Release (e.g. not Clang or Mingw). If it isn't, you can click the down arrow to change that. Also click the build method name itself, and in the menu that opens make sure that BLITZ is turned off.
- Go to setup->Build methods and put the following into various fields:

Common options: /bigobj -D_CRT_SECURE_NO_WARNINGS /IC:\Data\Mobius2\src /IC:\Data\llvm-project\llvm\include /IC:\Data\llvm-project\build\include /IC:/Data/OpenXLSX/OpenXLSX/ /IC:/Data/OpenXLSX/OpenXLSX/headers /IC:/Data/OpenXLSX/build/OpenXLSX -D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_NONSTDC_NO_WARNINGS -D_SCL_SECURE_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -DMOBIUS_ERROR_STREAMS

Common C++ options: /std:c++17 /w  /EHsc

Common link options: OpenXLSX.lib LLVMWindowsManifest.lib LLVMXRay.lib LLVMLibDriver.lib LLVMDlltoolDriver.lib LLVMTextAPIBinaryReader.lib LLVMCoverage.lib LLVMLineEditor.lib LLVMX86TargetMCA.lib LLVMX86Disassembler.lib LLVMX86AsmParser.lib LLVMX86CodeGen.lib LLVMX86Desc.lib LLVMX86Info.lib LLVMOrcDebugging.lib LLVMOrcJIT.lib LLVMWindowsDriver.lib LLVMMCJIT.lib LLVMJITLink.lib LLVMInterpreter.lib LLVMExecutionEngine.lib LLVMRuntimeDyld.lib LLVMOrcTargetProcess.lib LLVMOrcShared.lib LLVMDWP.lib LLVMDebugInfoLogicalView.lib LLVMDebugInfoGSYM.lib LLVMOption.lib LLVMObjectYAML.lib LLVMObjCopy.lib LLVMMCA.lib LLVMMCDisassembler.lib LLVMLTO.lib LLVMPasses.lib LLVMHipStdPar.lib LLVMCFGuard.lib LLVMCoroutines.lib LLVMipo.lib LLVMVectorize.lib LLVMLinker.lib LLVMInstrumentation.lib LLVMFrontendOpenMP.lib LLVMFrontendOffloading.lib LLVMFrontendOpenACC.lib LLVMFrontendHLSL.lib LLVMFrontendDriver.lib LLVMExtensions.lib LLVMDWARFLinkerParallel.lib LLVMDWARFLinkerClassic.lib LLVMDWARFLinker.lib LLVMGlobalISel.lib LLVMMIRParser.lib LLVMAsmPrinter.lib LLVMSelectionDAG.lib LLVMCodeGen.lib LLVMTarget.lib LLVMObjCARCOpts.lib LLVMCodeGenTypes.lib LLVMIRPrinter.lib LLVMInterfaceStub.lib LLVMFileCheck.lib LLVMFuzzMutate.lib LLVMScalarOpts.lib LLVMInstCombine.lib LLVMAggressiveInstCombine.lib LLVMTransformUtils.lib LLVMBitWriter.lib LLVMAnalysis.lib LLVMProfileData.lib LLVMSymbolize.lib LLVMDebugInfoBTF.lib LLVMDebugInfoPDB.lib LLVMDebugInfoMSF.lib LLVMDebugInfoDWARF.lib LLVMObject.lib LLVMTextAPI.lib LLVMMCParser.lib LLVMIRReader.lib LLVMAsmParser.lib LLVMMC.lib LLVMDebugInfoCodeView.lib LLVMBitReader.lib LLVMFuzzerCLI.lib LLVMCore.lib LLVMRemarks.lib LLVMBitstreamReader.lib LLVMBinaryFormat.lib LLVMTargetParser.lib LLVMTableGen.lib LLVMSupport.lib LLVMDemangle.lib psapi.lib shell32.lib  uuid.lib advapi32.lib ntdll.lib Ws2_32.lib 

Release options: -O2 /MD

Release link options: /STACK:20000000 /LIBPATH:C:\Data\llvm-project\build\Release\lib /LIBPATH:C:\Data\OpenXLSX\build\output\Release

- Finally click the yellow lightning icon to build the program.
- The finished MobiView2.exe will appear in C:\Data\upp\out\MyApps\MSVS26x64.Gui\

## Notes for making a debug build:

- You have to make debug builds of LLVM and OpenXLSX. When OpenXLSX has been compiled, rename OpenXLSX\build\output\Debug\OpenXLSXd.lib to OpenXLSX.lib
- In the Ultimate++ ide, set the build method to a variant of MSVS x64 Debug. Again, it is important to make sure BLITZ is turned off.
- In setup->Build methods:

Debug options: -Od /MDd

Debug link options /STACK:20000000 /LIBPATH:C:\Data\llvm-project\build\Debug\lib /LIBPATH:C:\Data\OpenXLSX\build\output\Debug

- You can use the inbuilt debugger in the Ultimate++ IDE to debug MobiView2.


