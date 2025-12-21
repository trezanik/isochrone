# Design

This has been extremely unclean in how build options have been made configurable over time.

Bear in mind this started as Visual Studio only, with autotools and cmake added afterwards, then meson on top due to how diabolical I find the former.

This can be consolidated and made consistent easily, but I would like to maintain support for those wanting or needing specific uses, no external tools, etc.

## Adding and Removing Option Items

We'll use the application configuration filename as the example here.

In the `app/definitions.h` header (_app_ as this is the project scope it's contained within), at the end of the file, add:
```
#if !defined(TZK_CONFIG_FILENAME)
	// Filename of the application configuration
#	define TZK_CONFIG_FILENAME         "app.cfg"
#endif
```
This is structured so if the define has been provided already - either via the configure.h forced include, manual preprocessor definitions, or via direct command-line - then that value is used.

Only if it was not supplied, do we then have our definition set. This is deemed to be the application default; if you build without anything custom, this is the value used.

Anything deemed an option - something we're happy to make a compile-time override - must be presented here, and have a default value supplied. Including a comment regarding what it controls is highly recommended, and should be deemed mandatory.

With the source now setup, it's now time to expose the option to the build system or compiler in use.

### meson

Open the `meson.options` file. Add a line adhering to the meson format: https://mesonbuild.com/Build-options.html 

The name is free for choice, but consider it's what needs supplying in the command line. It shouldn't be too long but still relay its purpose clearly. In our case we use:

`option('App-ConfigFilename', type : 'string', value : 'app.cfg')`

Unfortunately the value specified is essentially a duplicate of the definitions.h content - ensure it is kept in sync.

You now need to open the `meson.build` file and find the end of the configure args options. These I would like to be optional eventually; not available yet but we're keeping the needed lines present and commented out.

Add the following to link the option with the preprocessor and complete the amendments:
```
#if get_option('App-ConfigFilename')
	add_project_arguments(['-DTZK_CONFIG_FILENAME=' + get_option('App-ConfigFilename').to_string()], language: 'cpp')
#endif
```

### CMake

CMake exposes the singular _ISOCHRONE_FORCE_INCLUDE_CONFIGURE_ in CMakeLists.txt. Run `generate_configure_h.sh` to create the file, where it can be modified as desired.

No further work is required to expose the new option.

This is the ONLY way for CMake builds to not use a default setup.

### Visual Studio (Manual Solution and Projects)

Options are added in the `BuildOptions.props` file, which is automatically included by every project.

The primary purpose of this was to toggle dependencies (the '_UsingXxx_' items) and the toolset/language standards. The only other options added were those I needed mid-development as quick toggles, since the default options can just be modified inline.

I would generally not recommend using this route outside of testing; but if going ahead anyway:

- In the property sheet under *User Macros* you can _Add Macro_ - give it an appropriate name and setting. My convention has been PascalCase, and I'll use _AudioLogTracing_ as an example.
- To expose this to all projects, navigate to *C/C++* > *Preprocessor*, and focus on the *Preprocessor Definitions*
- Append or insert as desired - per C and C++ conventions, macros are all uppercase and we use our own 'TZK_' prefix, resulting in : `TZK_AUDIO_LOG_TRACING=$(AudioLogTracing);`
- Now use `#if TZK_AUDIO_LOG_TRACING` as needed in source

Note that external dependencies are structured in a near identical way.

You can still make use of the configure.h by setting the project to use it as a forced include, just like CMake would do - as long as you can run the shell script!

