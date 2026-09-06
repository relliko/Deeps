# Build status

This branch contains the memory-safety-hardened Deeps source based on Troy's Ashita 4.30 export port.

The packet-safety tests pass with compiler warnings treated as errors and under AddressSanitizer/UndefinedBehaviorSanitizer. GitHub Actions also builds the production Win32 DLL with Visual Studio/MSVC against the pinned Ashita 4.30 SDK, verifies the PE32/x86 architecture and required exports, and packages the install artifact.

The built DLL has **not** yet completed an in-game HorizonXI soak test. Test load, combat parsing, `/dps report`, unload, and reload in an isolated profile before installing it into a known-good game environment.
