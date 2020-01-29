# RabbitCall implementation notes

- Implemented in cross-platform C++ with only a few platform-dependent parts. Intended to work on any common development platform.
- Performance is meant to be fast (processing ~100 MB of source code per second) so that builds will be fast and changes to C++ interface will be immediately available in C#, also for auto-complete.
- Some parts of the processing are multi-threaded (e.g. source directory scanning, source file parsing).
- All std::strings are in UTF-8.
- Error messages are formatted in a particular way so that if the user clicks on them in Visual Studio, the corresponding location in the source file will be opened.