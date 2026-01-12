# Klish 3

## Overview

The klish program is designed to provide a command-line interface where the operator only has access to a strictly defined set of commands. This distinguishes klish from standard shell interpreters, where the operator can use any commands existing in the system, and the ability to execute them depends only on access rights. The set of available commands in klish is determined during configuration and can be specified, in general, in various ways, the main one being XML configuration files. Commands in klish are not system commands but are fully defined in configuration files, with all possible parameters, syntax, and actions they perform.

The primary use of klish is in embedded systems, where the operator only has access to specific commands particular to the device, rather than an arbitrary set of commands as in general-purpose systems. In such embedded systems, klish can replace the shell interpreter, which is inaccessible to the operator.

An example of embedded systems is managed network equipment. Historically, two main approaches to organizing the command-line interface have developed in this segment. They can be conditionally called the "Cisco" approach and the "Juniper" approach. Both Cisco and Juniper have two modes of operation - command mode and configuration mode. In command mode, entered commands are executed immediately but do not affect the system configuration. These are commands for viewing status, device management commands, but not for changing its configuration. In configuration mode, equipment and services are configured. In Cisco, configuration commands are also executed immediately, changing the system configuration. In Juniper, the configuration can be roughly imagined as a text file, where changes are made using standard editing commands. During editing, changes are not applied to the system. And only upon a special command, the entire accumulated set of changes is applied at once, ensuring the consistency of changes. In the Cisco approach, similar behavior can also be emulated by designing commands in a certain way, but for Cisco, this is a less natural way.

Which approach is better and simpler in a particular case is determined by the task. Klish is primarily designed for the Cisco approach, i.e., for immediately executed commands. However, the project has a plugin system, which allows extending its capabilities. For example, the klish-plugin-sysrepo plugin, implemented as a separate project, working based on the sysrepo storage, allows organizing the Juniper approach.

## Basics

![Klish Client-Server Model](/klish-client-server.en.png "Klish Client-Server Model")

The klish project uses a client-server model. The listening server klishd loads the command configuration and waits for client requests on a UNIX socket (1). When a client connects, the listening server klishd forks a separate process (2) that will handle one specific client. The forked process is called a "handling klishd server". The listening server klishd continues to wait for new client connections. Interaction between clients and the handling server occurs over UNIX sockets using the specially designed KTP protocol (Klish Transfer Protocol) (3).

The client's task is to transmit operator input to the server and receive the result from it to show the operator. The client does not know what commands exist or how to execute them. Execution is handled by the server side. Since the client has relatively simple code, it is not difficult to implement alternative client programs, for example, a graphical client or a client for automated management. Currently, only the klish command-line text client exists.

![Klish Libraries](/klish-libs.en.png "Klish Libraries")

The foundation of the klish project is the libklish library. The klish client and klishd server are built on it. The library implements all the basic mechanisms of operation and interaction. The library is part of the klish project.

The text client uses another built-in library - libtinyrl (Tiny ReadLine). The library provides interaction with the user when working in the command line.

All executable files of the klish project, as well as all its built-in libraries, use the libfaux (Library of Auxiliary Functions) library in their work - a library of auxiliary functions. In previous versions of the klish project, the library was part of the klish source code; now it is separated into a separate project and includes functions that facilitate working with strings, files, networks, standard data structures, etc. The libfaux library is a mandatory external dependency for the klish project.

Additionally, klish plugins, which will be discussed below, may use other libraries for their work.

Klish has two types of plugins. Plugins for loading command configuration (let's call them database plugins) and plugins that implement actions for user commands (let's call them function plugins). Klish does not have built-in commands and configuration loading means (except for the special ischeme case). All this functionality is implemented in plugins. Several plugins with basic functionality are included in the klish source code and are located in the "dbs/" and "plugins/" directories.

All necessary plugins are loaded by the listening server klishd at the start of its operation. It learns which plugins for loading user command configuration to use from its base configuration file (a special file in INI format). Then, using a database plugin, the klishd server loads the user command configuration (for example, from XML files). From this configuration, it learns the list of necessary function plugins and also loads these plugins into memory. However, functions from function plugins will only be used by the handling (not listening) klishd server upon user requests.

To configure the klishd server parameters, the configuration file `/etc/klish/klishd.conf` is used. An alternative configuration file can be specified when starting the klishd server.

To configure the client parameters, the configuration file `/etc/klish/klish.conf` is used. An alternative configuration file can be specified when starting the klish client.

## Command Configuration Loading

![Command Configuration Loading](/klish-plugin-db.en.png "Command Configuration Loading")

The internal representation of command configuration in klish is kscheme. Kscheme is a set of C structures representing the entire tree of available user commands, scopes, parameters, etc. All internal work is performed based on these structures - command search, auto-completion, generating hints when interacting with the user.

In klish, there is an intermediate representation of command configuration in the form of C structures, called ischeme. This representation is generally similar to kscheme but differs in that all configuration fields are represented in text form, all references to other objects are also textual names of the objects they refer to, rather than pointers as in kscheme. There are other differences as well. Thus, ischeme can be called an "uncompiled scheme," and kscheme a "compiled" one. "Compilation," i.e., the transformation of ischeme into kscheme, is performed by klish's internal mechanisms. Ischeme can be manually defined by the user as C structures and compiled as a separate database plugin. In this case, other database plugins will not be needed; the entire configuration is ready for transformation into kscheme.

The following database plugins for loading command configuration, i.e., the scheme, are included in klish (see dbs/):

* expat - Uses the expat library to load configuration from XML.
* libxml2 - Uses the libxml2 library to load configuration from XML.
* roxml - Uses the roxml library to load configuration from XML.
* ischeme - Uses configuration built into the C code (Internal Scheme).

All database plugins translate the external configuration, obtained for example from XML files, into ischeme. In the case of ischeme, an additional transformation step is not required because ischeme is already ready.

Installed dbs plugins are located in `/usr/lib` (if configured with --prefix=/usr). Their names are `libklish-db-<name>.so`, for example `/usr/lib/libklish-db-libxml2.so`.

## Executable Function Plugins

Each klish command performs some action or several actions at once. These actions must be described somehow. Looking at the implementation, klish can only execute compiled code from a plugin. Plugins contain so-called symbols, which essentially represent functions with a unified fixed API. Commands in klish can reference these symbols. In turn, a symbol can execute complex code, for example, launch a shell interpreter with a script defined when describing the klish command in the configuration file. Or another symbol can execute a Lua script.

Klish can only obtain symbols from plugins. Standard symbols are implemented in plugins included in the klish project. The following plugins are included in klish:

* klish - Basic plugin. Navigation, data types (for parameters), auxiliary functions.
* lua - Execution of Lua scripts. The mechanism is built into the plugin and does not use an external program for interpretation.
* script - Execution of scripts. Considers the shebang to determine which interpreter to use. Thus, not only shell scripts but also scripts in other interpreted languages can be executed. By default, the shell interpreter is used.

Users can write their own plugins and use custom symbols in klish commands. Installed plugins are located in `/usr/lib` (if configured with --prefix=/usr). Their names are `libklish-plugin-<name>.so`, for example `/usr/lib/libklish-plugin-script.so`.

![Command Execution](/klish-exec.en.png "Command Execution")

Symbols can be "synchronous" and "asynchronous". Synchronous symbols are executed in the klishd address space; for asynchronous ones, a separate process is forked.

Asynchronous symbols can accept user input (stdin) received by the handling klishd server from the client via the KTP protocol and forwarded to the forked process, within which the asynchronous symbol is executed. In turn, an asynchronous symbol can write to stdout and stderr streams. Data from these streams is received by the handling klishd server and forwarded to the client via the KTP protocol.

Synchronous symbols (with output to stdout, stderr) cannot receive data through the input stream stdin because they are executed within the handling klishd server, and for the duration of the symbol code execution, other server functions are temporarily halted. Data reception from the client stops. The handling klishd server is a single-threaded program, so synchronous symbols must be used with caution. If a symbol takes too long to execute, the server will hang for that time. Additionally, synchronous symbols are executed in the server's address space. This means that executing an untested symbol containing errors can lead to the handling server crashing. Although a synchronous symbol does not have an input stream stdin, it can use output streams stdout and stderr. To obtain data from these streams and subsequently send them to the client, the handling server spawns a helper process that receives and temporarily stores data coming from the symbol. When the symbol execution completes, the helper process forwards the stored data back to the handling server, which, in turn, sends them to the client. Synchronous symbols are recommended only when the symbol changes the state of the handling server and therefore cannot be executed within another process. For example, navigation through sections (see Scopes) is implemented by a synchronous symbol because the user's position in the section tree is stored in the handling server.

Lightweight synchronous symbols do not have input and output data streams stdin, stdout, stderr. Due to this, executing such a symbol does not require forking new processes, as is done in the case of a regular synchronous symbol or an asynchronous symbol. Thus, a lightweight synchronous symbol executes as quickly as possible. Although a lightweight synchronous symbol does not have a stdout stream, it can form an output text string. The string is formed manually and, using a special function, is passed to the handling server, which can send it to the client as stdout. In other respects, the lightweight synchronous symbol has the same features as a regular synchronous symbol. Lightweight synchronous symbols are recommended when maximum execution speed is required. An example of such a task is generating the user prompt. This is a service function that executes every time the user presses the enter key. Another use case is validation functions to check if the argument entered by the user is valid (see PTYPE), as well as functions for generating auto-completion and hints.

A symbol, or rather a klish command, since a command can consist of several sequentially executed symbols, can be regular or a filter. For simplicity, let's assume that a command executes only one symbol, and therefore we will talk about symbols, not commands. A regular symbol performs some useful action and often produces some text output. A filter receives the result of work, i.e., in this case, the text output of a regular symbol, and processes it. To indicate that a filter should be applied to a regular command, the user uses command chains separated by the "|" character. This is largely analogous to chains in the standard user shell.

![Filters](/klish-filters.en.png "Filters")

Synchronous symbols do not have an input stream stdin, i.e., they do not receive any input data and therefore cannot be full-fledged filters; they can only be used as the first command in a command chain. Lightweight synchronous symbols do not have an output stream stdout either, so they cannot be used either as filters or as a command in the first place in a command chain because the filter cannot receive any data from them. The typical use of lightweight synchronous symbols is internal service functions that do not interact directly with the user and are not used in command chains.

The first command in a command chain receives data from the user via the stdin stream if required. Let's call the stdin stream from the user the global stdin stream. Then the stdout stream of the previous symbol in the chain is fed into the stdin stream of the subsequent symbol. The output stdout stream of the last symbol in the chain is considered global and is sent to the handling server and then to the user.

Any symbol in the chain can write to the stderr stream. The entire chain has a unified stderr stream, i.e., one for all symbols. This stream is considered the global stderr and is sent to the handling server and then to the user.

The number of commands in a command chain is unlimited. A klish command must be declared as a filter to be used after the "|" sign.

## KTP Protocol

The KTP (Klish Transfer Protocol) is used for interaction between the klish client and the handling klishd server. The main task is to transfer commands and user input from the client to the server, and to transfer the text output of the executed command and the command execution status in the opposite direction. Additionally, the protocol provides means for the client to receive auto-completion options for incomplete commands and hints for commands and parameters from the server.

The KTP protocol is implemented using means from the libfaux library, which simplifies the creation of a specialized network protocol based on a generalized network protocol template. The protocol is built on top of a stream socket. Currently, the klish project uses UNIX sockets for client-server interaction.

A KTP protocol packet consists of a fixed-length header and a set of "parameters". A special field in the header stores the number of parameters. Each parameter has its own small header, which specifies the parameter type and its length. Thus, at the beginning of the KTP packet, there is a KTP header, followed by the header of the first parameter, the data of the first parameter, the header of the second parameter, the data of the second parameter, etc. All fields in the headers are transmitted using network (big-endian) byte order.

KTP header:

| Size, bytes | Field Name                     | Field Value |
|-------------|--------------------------------|-------------|
| 4           | Magic number                   | 0x4b545020  |
| 1           | Protocol version (major)       | 0x01        |
| 1           | Protocol version (minor)       | 0x00        |
| 2           | Command code                   |             |
| 4           | Status                         |             |
| 4           | Unused                         |             |
| 4           | Number of parameters           |             |
| 4           | Total packet length (with header)|             |

Command codes:

| Code | Name           | Direction | Description                                       |
|------|----------------|-----------|---------------------------------------------------|
| 'i'  | STDIN          | ->        | stdin user input (PARAM_LINE)                     |
| 'o'  | STDOUT         | <-        | stdout command output (PARAM_LINE)                |
| 'e'  | STDERR         | <-        | stderr command output (PARAM_LINE)                |
| 'c'  | CMD            | ->        | Command to execute (PARAM_LINE)                   |
| 'C'  | CMD_ACK        | <-        | Command execution result                          |
| 'v'  | COMPLETION     | ->        | Auto-completion request (PARAM_LINE)              |
| 'V'  | COMPLETION_ACK | <-        | Possible completions (PARAM_PREFIX, PARAM_LINE)   |
| 'h'  | HELP           | ->        | Help request (PARAM_LINE)                         |
| 'H'  | HELP_ACK       | <-        | Help hint (PARAM_PREFIX, PARAM_LINE)              |
| 'n'  | NOTIFICATION   | <->       | Asynchronous message (PARAM_LINE)                 |
| 'a'  | AUTH           | ->        | Authentication request                            |
| 'A'  | AUTH_ACK       | <-        | Authentication confirmation                       |
| 'I'  | STDIN_CLOSE    | ->        | stdin was closed                                  |
| 'O'  | STDOUT_CLOSE   | ->        | stdout was closed                                 |
| 'E'  | STDERR_CLOSE   | ->        | stderr was closed                                 |

The "Direction" column shows the direction of command transmission. A right arrow indicates transmission from the client to the server, a left arrow from the server to the client. In the "Description" column, the names of the parameters in which data is transmitted are given in parentheses.

The status field in the KTP header is a 32-bit field. Bitmask values of the status field:

| Bit Mask   | Name              | Value                                       |
|------------|-------------------|---------------------------------------------|
| 0x00000001 | STATUS_ERROR      | Error                                       |
| 0x00000002 | STATUS_INCOMPLETED| Command execution not completed             |
| 0x00000100 | STATUS_TTY_STDIN  | Client stdin is a terminal                  |
| 0x00000200 | STATUS_TTY_STDOUT | Client stdout is a terminal                 |
| 0x00000400 | STATUS_TTY_STDERR | Client stderr is a terminal                 |
| 0x00001000 | STATUS_NEED_STDIN | Command accepts user input                  |
| 0x00002000 | STATUS_INTERACTIVE| Command output is intended for a terminal   |
| 0x00010000 | STATUS_DRY_RUN    | Dry run. Command execution not required     |
| 0x80000000 | STATUS_EXIT       | Session termination                         |

Parameter header:

| Size, bytes | Field Name                          |
|-------------|-------------------------------------|
| 2           | Parameter type                      |
| 2           | Unused                              |
| 4           | Parameter data length (without header) |

Parameter types:

| Code | Name          | Direction | Description                                     |
|------|---------------|-----------|-------------------------------------------------|
| 'L'  | PARAM_LINE    | <->       | String. Multi-functional                        |
| 'P'  | PARAM_PREFIX  | <-        | String. For auto-completion and hints           |
| '$'  | PARAM_PROMPT  | <-        | String. User prompt                             |
| 'H'  | PARAM_HOTKEY  | <-        | Function key and its value                      |
| 'W'  | PARAM_WINCH   | ->        | User window size. On change                     |
| 'E'  | PARAM_ERROR   | <-        | String. Error message                           |
| 'R'  | PARAM_RETCODE | <-        | Return code of the executed command             |

Additional parameters can be transmitted from the server to the client along with the command and its corresponding parameters. For example, with the CMD_ACK command, which reports the completion of a user command execution, a PARAM_PROMPT parameter can be sent, informing the client that the user prompt has changed.

## XML Configuration Structure

The main way to describe klish commands today is XML files. All examples in this section will be based on XML elements.

### Scopes

Commands are organized into "scopes" called VIEW. That is, each command belongs to some VIEW and is defined within it. When working in klish, there is always a "current path". By default, when klish starts, the current path is set to the VIEW named "main". The current path determines which commands are visible to the operator at that moment. The current path can be changed by [navigation commands](#navigation). For example, moving to an "adjacent" VIEW will make that adjacent VIEW the current path, displacing the old one. Another possibility is to "go deeper," i.e., enter a nested VIEW. Then the current path becomes two-level, similar to entering a nested directory in a file system. When the current path has more than one level, the operator has access to the commands of the "deepest" VIEW, as well as the commands of all higher levels. Navigation commands can be used to exit nested levels. When changing the current path, the navigation command used determines whether the current path's VIEW will be replaced at the same nesting level by another VIEW, or whether the new VIEW will become nested and another level will be added to the current path. How VIEWs are defined in XML files does not affect whether a VIEW can be nested.

When defining VIEWs in XML files, some VIEWs can be nested within others. This nesting should not be confused with nesting when forming the current path. A VIEW defined inside another VIEW adds its commands to the commands of the parent VIEW but can also be addressed separately from the parent. Additionally, there are references to VIEWs. By declaring such a reference inside a VIEW, we add the commands of the VIEW pointed to by the reference to the commands of the current VIEW. You can define a VIEW with "standard" commands and include a reference to this VIEW in other VIEWs where these commands are needed, without redefining them again.

In XML configuration files, the `VIEW` tag is used to declare a VIEW.

### Commands and Parameters

Commands (`COMMAND` tag) can have parameters (`PARAM` tag). A command is defined inside some VIEW and belongs to it. Parameters are defined inside a command and, in turn, belong to it. A command can consist of only one word, unlike commands in previous versions of klish. If you need to define a multi-word command, then nested commands are created. That is, inside a command identified by the first word of the multi-word command, a command identified by the second word of the multi-word command is created, etc.

Strictly speaking, a command differs from a parameter only in that its value can only be a predefined word, while a parameter's value can be anything. Only the parameter type determines its possible values. Thus, a command can be viewed as a parameter with a type that indicates that the allowed value is the parameter name itself. The internal implementation of commands is exactly like that.

In general, a parameter can be defined directly in a VIEW, not inside a command, but this is rather an atypical case.

Like VIEWs, commands and parameters can be references. In this case, the reference can be considered simply as a substitution of the object it points to.

Parameters can be mandatory, optional, or represent a mandatory choice among several candidate parameters. Thus, when entering a command, the operator may specify some parameters and not others. When parsing the command line, a sequence of selected parameters is compiled.

### Parameter Type

The parameter type determines the allowed values for that parameter. Usually, types are defined separately using the `PTYPE` tag, and a parameter references a defined type by its name. Also, a type can be defined directly inside a parameter, but this is not typical, as standard types cover most needs.

The PTYPE type, like a command, performs a specific action specified by the `ACTION` tag. The action specified for the type checks the value entered by the operator for the parameter and returns a result - success or error.

### Action

Each command must define an action performed when the operator enters that command. There can be one action or several actions for one command. The action is declared by the `ACTION` tag inside the command. The ACTION specifies a reference to a symbol (function) from a function plugin that will be executed in this case. All data inside the ACTION tag is available to the symbol. The symbol can use this information at its discretion. For example, as data, a script can be specified that will be executed by the symbol.

The result of executing an action is a "return code". It determines the success or failure of the command as a whole. If more than one action is defined for a single command, calculating the return code becomes a more complex task. Each action has a flag determining whether the return code of the current action affects the overall return code. Also, actions have settings determining whether the action will be executed if the previous action ended with an error. If several actions are executed sequentially that have the flag to influence the overall return code, then the overall return code will be the return code of the last such action.

### Filters

Filters are commands that process the output of other commands. A filter can be specified in the command line after the main command and the `|` sign. A filter cannot be a standalone command and cannot be used without a main command. A typical example of a filter is an analog of the UNIX utility `grep`. Filters can be used one after another, separated by `|`, as is done in the shell.

If a command is not a filter, it cannot be used after the `|` symbol.

A filter is defined in configuration files using the `FILTER` tag.

### Parameter Containers

The SWITCH and SEQ containers are used to aggregate parameters nested within them and define the rules by which the command line is parsed relative to these parameters.

By default, it is assumed that if several parameters are defined sequentially inside a command, then all these parameters must also be present sequentially in the command line. However, sometimes it is necessary for the operator to enter only one parameter chosen from a set of possible parameters. In this case, the SWITCH container can be used. If a SWITCH container is encountered inside a command during command line parsing, then only one parameter from the parameters nested in SWITCH should be selected to match the next word entered by the operator. That is, branching of the parse occurs using the SWITCH container.

The SEQ container defines a sequence of parameters. All of them must be sequentially matched with the command line. Although, as noted earlier, parameters nested in a command are parsed sequentially, the container can be useful in more complex constructs. For example, the SEQ container itself can be an element of the SWITCH container.

### Command Line Prompts

The `PROMPT` construct is used to form the command line prompt that the operator sees. The PROMPT tag must be nested inside a VIEW. Different VIEWs can have different prompts. That is, depending on the current path, the operator sees a different prompt. The prompt can be dynamic and generated by `ACTION` actions specified inside PROMPT.

### Auto-completion and Hints

For the operator's convenience, hints and auto-completion can be implemented for commands and parameters. Hints (help) explaining the purpose and format of possible parameters are displayed in the klish client by pressing the `?` key. A list of possible completions is printed when the operator presses the `Tab` key.

The `HELP` and `COMPL` tags are used in the configuration to set hints and lists of possible completions. These tags must be nested relative to the corresponding commands and parameters. For simplicity, hints for a parameter or command can be specified as an attribute of the main tag if the hint is static text and does not need to be generated. If the hint is dynamic, its content is generated by ACTION actions nested inside HELP. For COMPL completions, ACTION actions generate a list of possible parameter values separated by a newline.

### Conditional Element

Commands and parameters can be hidden from the operator based on dynamic conditions. The condition is set using the `COND` tag nested inside the corresponding command or parameter. Inside COND, there are one or more ACTION actions. If the ACTION return code is `0`, i.e., success, then the parameter is available to the operator. If ACTION returns an error, the element will be hidden.

### Plugin

The klishd process does not load all existing function plugins indiscriminately, but only those specified in the configuration using the `PLUGIN` tag. The data inside the tag can be interpreted by the plugin's initialization function at its discretion, in particular, as configuration for the plugin.

### Hotkeys

For the operator's convenience, "hotkeys" can be set in the klish command configuration. The tag for setting hotkeys is `HOTKEY`. The list of hotkeys is transmitted by the klishd server to the client. The client uses this information at its discretion or does not use it. For example, for an automated management client, information about hotkeys does not make sense.

When defining a hotkey, the text command that should be executed when the operator presses the hotkey is specified. This could be a quick display of the current device configuration, exiting the current VIEW, or any other command. The klish client "catches" hotkey presses and sends the command corresponding to the pressed hotkey to the server.

### References to Elements

Some tags have attributes that are references to other elements defined in the configuration files. For example, `VIEW` has a `ref` attribute, with which a "copy" of an external `VIEW` is created at the current location in the scheme. Or the `PARAM` tag has a `ptype` attribute, with which it references a `PTYPE`, defining the parameter type. Klish has its own "language" for organizing references. It can be said that this is a greatly simplified analog of paths in a file system or XPath.

The path to an element in klish is formed by specifying all its parent elements with the separator `/`. The element's name is considered to be the value of its `name` attribute. The path also starts with the `/` character, denoting the root element.

> Relative paths are currently not supported

```
<KLISH>

<PTYPE name="PTYPE1">
	<ACTION sym="sym1"\>
</PTYPE>

<VIEW name="view1">

	<VIEW name="view1_2">
		<COMMAND name="cmd1">
			<PARAM name="par1" ptype="/PTYPE1"/>
		</COMMAND>
	</VIEW>

</VIEW>

<VIEW name="view2">

	<VIEW ref="/view1/view1_2"/>

</VIEW>

</KLISH>
```

The parameter "par1" references the `PTYPE` using the path `/PTYPE1`. Type names are conventionally denoted in uppercase letters to make it easier to distinguish types from other elements. Here the type is defined at the very top level of the scheme. Basic types are usually defined this way, but `PTYPE` does not have to be at the top level and can be nested inside a `VIEW` or even a `PARAM`. In that case, it will have a more complex path.

The `VIEW` named "view2" imports commands from the `VIEW` named "view1_2" using the path `/view1/view1_2`.

If, suppose, a reference to the parameter "par1" is needed, the path would look like this - `/view1/view1_2/cmd1/par1`.

The following elements cannot be referenced. They do not have a path:

* `KLISH`
* `PLUGIN`
* `HOTKEY`
* `ACTION`

> Do not confuse the [session current path](#scopes) with the path for creating references. The path for creating references uses the internal structure of the scheme defined when writing the configuration files. This is the path of the element within the scheme, uniquely identifying it. This is a static path. The nesting of elements when defining the scheme only forms the necessary sets of commands; this nesting is not visible to the operator and is not nesting in terms of his work with the command line. He only sees ready-made linear sets of commands.
>
> The session current path is dynamic. It is formed by the operator's commands and implies the possibility of going deeper, i.e., adding another level of nesting to the path and gaining access to an additional set of commands, or moving up. Using the current path, you can combine the linear command sets created at the scheme writing stage. Also, navigation commands allow you to completely replace the current set of commands with another by changing the `VIEW` at the current path level. Thus, the existence of the session current path can create the illusion of a branched configuration tree for the operator.

## Tags

### KLISH

Any XML file with klish configuration must begin with the opening tag `KLISH` and end with the closing tag `KLISH`.

```
<?xml version="1.0" encoding="UTF-8"?>
<KLISH
	xmlns="https://klish.libcode.org/klish3"
	xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	xsi:schemaLocation="https://src.libcode.org/pkun/klish/src/master/klish.xsd">

<!-- Any configuration for klish here -->

</KLISH>
```

The header introduces its default XML namespace "https://klish.libcode.org/klish3". The header is standard and most often there is no reason to change it.

### PLUGIN

Klish itself does not load any executable function plugins. Accordingly, the user does not have access to any symbols for use in ACTION actions. Plugin loading must be explicitly specified in the configuration files. The `PLUGIN` tag is used for this.

Note that even the basic plugin named "klish" is not loaded automatically and must be specified in the configuration files. This plugin, in particular, implements navigation. A typical configuration will contain the line:

```
<PLUGIN name="klish"/>
```

The `PLUGIN` tag cannot contain other nested tags.

#### Attribute `name`

The attribute defines the name by which the plugin will be recognized inside the configuration files. When `ACTION` references a symbol, either just the symbol name can be specified, or it can be clarified in which plugin to search for the symbol.

```
<ACTION sym="my_sym"\>

<ACTION sym="my_sym@my_plugin"\>
```

In the first case, klish will search for "my_sym" in all plugins and use the first one found. In the second case, the search will be performed only in the "my_plugin" plugin. Additionally, different plugins can contain symbols with the same name, and specifying the plugin will allow identifying which symbol was meant.

If the `id` and `file` attributes are not specified, then `name` is also used to form the plugin filename. The plugin must have the name `kplugin-<name>.so` and be located in the directory `/usr/lib/klish/plugins` (if klish was configured with `--prefix=/usr`).

#### Attribute `id`

The attribute is used to form the plugin filename if the `file` attribute is not specified. The plugin must have the name `kplugin-<id>.so` and be located in the directory `/usr/lib/klish/plugins` (if klish was configured with `--prefix=/usr`).

If the `id` attribute is specified, then `name` will not be used to form the plugin filename, but only for identification inside the configuration files.

```
<PLUGIN name="alias_for_klish" id="klish"\>

<ACTION sym="nav@alias_for_klish"\>
```

#### Attribute `file`

The full filename of the plugin (shared library) can be specified explicitly. If the `file` attribute is specified, then no other attributes will be used to form the plugin filename; the value of `file` will be taken "as is" and passed to the dlopen() function. This means that either an absolute path or just a filename can be specified, but in the latter case, the file must be located along the standard paths used when searching for a shared library.

```
<PLUGIN name="my_plugin" file="/home/ttt/my_plugin.so"\>

<ACTION sym="my_sym@my_plugin"\>
```

#### Data inside the tag

The data inside the `PLUGIN` tag can be processed by the plugin's initialization function. The data format remains at the discretion of the plugin itself. For example, the data could be settings for the plugin.

```
<PLUGIN name="sysrepo">
	JuniperLikeShow = y
	FirstKeyWithStatement = n
	MultiKeysWithStatement = y
	Colorize = y
</PLUGIN>
```

### HOTKEY

For more convenient work in the command-line interface, "hotkeys" can be set for frequently used commands. A hotkey is defined using the `HOTKEY` tag.

For hotkeys to work, support is needed in the client program that connects to the klishd server. The "klish" client has such support.

The `HOTKEY` tag cannot have nested tags. Any additional data inside the tag is not provided.

#### Attribute `key`

The attribute defines what exactly the operator must press to activate the hotkey. Combinations with the "Ctrl" key are supported. For example, `^Z` means that the key combination "Ctrl" and "Z" should be pressed.

```
<HOTKEY key="^Z" cmd="exit"\>
```

#### Attribute `cmd`

The attribute defines what action will be performed when the operator presses the hotkey. The value of the `cmd` attribute is parsed according to the same rules as any other command manually entered by the operator.

```
<COMMAND name="exit" help="Exit view or shell">
	<ACTION sym="nav">pop</ACTION>
</COMMAND>

<HOTKEY key="^Z" cmd="exit"\>
```

The command used as the value of the `cmd` attribute must be defined in the configuration files. The given example will execute the previously defined `exit` command when the key combination "Ctrl^Z" is pressed.

### ACTION

The `ACTION` tag defines an action that needs to be performed. The typical use of the tag is inside a `COMMAND` command. When the operator enters the corresponding command, the actions defined in `ACTION` will be executed.

The main attribute of `ACTION` is `sym`. An action can only execute symbols (functions) defined in plugins. The `sym` attribute references such a symbol.

Actions can be executed not only by a command. The following is a list of tags inside which `ACTION` can occur:

* `ENTRY` - what the `ACTION` will be used for is determined by the parameters of `ENTRY`.
* `COMMAND` - the action defined in `ACTION` is executed when the operator enters the corresponding command.
* `PARAM` - same as for `COMMAND`.
* `PTYPE` - `ACTION` defines actions for validating the value of a parameter entered by the operator that has the corresponding type.

Inside the listed elements, there can be several `ACTION` elements simultaneously. Let's call this a block of `ACTION` elements. Actions are executed sequentially, one after another, unless otherwise defined by the `exec_on` attribute.

Inside one command, several blocks of actions can be defined. This is possible if the command has parameter branching or optional parameters. Actions defined inside one element are considered a block. Actions defined in different elements, including nested ones, belong to different blocks. Only one block of actions is always executed.

```
<COMMAND name="cmd1">
	<ACTION sym="sym1"\>
	<SWITCH min="0">
		<COMMAND name="opt1">
			<ACTION sym="sym2"\>
		</COMMAND>
		<COMMAND name="opt2"\>
		<PARAM name="opt3" ptype="/STRING">
			<ACTION sym="sym3"\>
		</PARAM>
	</SWITCH>
</COMMAND>
```

The example declares a command "cmd1" that has three alternative (only one of the three can be specified) optional parameters. The search for actions to execute goes from the end to the beginning when parsing the entered command line.

So if the operator entered the command `cmd1`, the parsing mechanism will recognize the command named "cmd1" and will look for `ACTION` directly in this element. The `ACTION` with the symbol "sym1" will be found.

If the operator entered the command `cmd1 opt1`, the string "opt1" will be recognized as a parameter (which is also a subcommand) named "opt1". The search goes from the end, so first the `ACTION` with the symbol "sym2" will be found. As soon as an action block is found, no further search for actions is performed, and "sym1" will not be found.

If the operator entered the command `cmd1 opt2`, the action with the symbol "sym1" will be found, because the "opt2" element does not have its own nested actions, and the search moves up to the parent elements.

If the operator entered the command `cmd1 arbitrary_string`, the action with the symbol "sym3" will be found.

#### Attribute `sym`

The attribute references a symbol (function) from a plugin. This function will be executed when the `ACTION` is performed. The value can be the symbol name, for example `sym="my_sym"`. In this case, the symbol search will occur across all loaded plugins. If the plugin in which to search for the symbol is specified, for example `sym="my_sym@my_plugin"`, then the search will not be performed in other plugins.

In different situations, it may be advantageous to use different approaches regarding whether to specify the plugin name for the symbol. On the one hand, several plugins can contain symbols with the same name, and for unambiguous symbol identification, specifying the plugin will be mandatory. Additionally, when specifying the plugin, the symbol search will be slightly faster. On the other hand, you can write some universal commands where symbols are specified without belonging to a plugin. Then several plugins can implement an "interface," i.e., all the symbols used, and their actual content will depend on which plugin is loaded.

#### Attribute `lock`

> Attention: the attribute is not yet implemented

Some actions require atomicity and exclusive access to a resource. When working in klish, this is not automatically ensured. Two operators can independently but simultaneously launch the same command for execution. To ensure atomicity and/or exclusive access to a resource, `lock` locks can be used. Locks in klish are named. The `lock` attribute specifies which lock the `ACTION` will acquire during execution. For example `lock="my_lock"`, where "my_lock" is the lock name. Acquiring a lock with one name does not affect locks with another name. Thus, the system can implement not one global lock but several separate ones, based, for example, on which resource the lock protects.

If a lock is acquired by one process, another process, when trying to acquire the same lock, will suspend its execution until the lock is released.

#### Attribute `interrupt`

A flag determining whether the current action can be interrupted by the user using the `Ctrl^C` combination. Possible values - `true`, `false`. Default value - `false`.

If the action must be atomic, the attribute should be set to `false`, or not specified at all, since by default all actions are uninterruptible. Long informational commands are recommended to be declared as interruptible.

#### Attribute `in`

The attribute shows whether the action can accept data through standard input (stdin) from the user. Possible values - `true`, `false`, `tty`. Default value `false`. Value `false` means the action does not accept data through standard input. Value `true` means the action accepts data through standard input. Value `tty` means the action accepts data through standard input and, at the same time, standard input is a terminal.

When executing an action, the client is informed about the status of standard input and, accordingly, waits for user input or not.

If a command implies sequential execution of several actions at once, and at least one action has the attribute with value `tty`, then the entire command is considered to have the attribute `tty`. If the command has no actions with the attribute value `tty`, then the attribute with value `true` is searched for. If at least one action has the attribute value `true`, then the entire command has the attribute `true`. Otherwise, the command has the attribute `false`.

If the user sent not one command for execution but a chain of commands (via the `|` sign), then the following logic is applied to notify the client whether standard input is required. If at least one command in the chain has the attribute value `tty`, then standard input is required. If the first command in the chain has the attribute `true`, then standard input is required. Otherwise, standard input is not required.

When executing an action, the handling server can create a pseudo-terminal and provide it to the action as standard input. Whether standard input will be a pseudo-terminal or not is determined by whether standard input on the client side is a terminal or not. This information is transmitted from the client to the server in the status word.

#### Attribute `out`

The attribute shows whether the action outputs data through standard output (stdout). Possible values - `true`, `false`, `tty`. Default value `true`. Value `false` means the action does not output data through standard output. Value `true` means the action outputs data through standard output. Value `tty` means the action outputs data through standard output and, at the same time, standard output is a terminal.

When executing an action, the client is informed about the status of standard output and, accordingly, puts standard output into raw terminal mode or not.

Usually, the text client launches a pager on its side to process the output of each command. If the command attribute has the value `tty`, the pager is not launched. It is assumed that an interactive command is being executed.

If a command implies sequential execution of several actions at once, and at least one action has the attribute with value `tty`, then the entire command is considered to have the attribute `tty`. If the command has no actions with the attribute value `tty`, then the attribute with value `true` is searched for. If at least one action has the attribute value `true`, then the entire command has the attribute `true`. Otherwise, the command has the attribute `false`.

If the user sent not one command for execution but a chain of commands (via the `|` sign), then the following logic is applied to notify the client about the status of standard output. If at least one command in the chain has the attribute value `tty`, then the "interactivity" flag is transmitted to the client. Otherwise, the client operates in normal mode.

When executing an action, the handling server can create a pseudo-terminal and provide it to the action as standard output. Whether standard output will be a pseudo-terminal or not is determined by whether standard output on the client side is a terminal and whether the command attribute is set to `tty`. Information about whether standard output on the client side is a terminal is transmitted from the client to the server in the status word.

#### Attribute `permanent`

The klish system can be started in "dry-run" mode, when all actions will not actually be executed, and their return code will always have the value "success". Such a mode can be used for testing, checking the correctness of incoming data, etc.

However, some symbols should always be executed, regardless of the mode. An example of such a symbol is the navigation function. That is, changing the current session path should always happen, regardless of the operating mode.

The `permanent` flag can change the behavior of an action in "dry-run" mode. Possible attribute values - `true` and `false`. By default `false`, i.e., the action is not "permanent" and will be disabled in "dry-run" mode.

Symbols, when declared in a plugin, already have a permanence attribute. A symbol can be forcibly declared permanent, forcibly declared non-permanent, or the plugin can leave the decision about permanence to the user. If the permanence flag is forcibly declared in the plugin, then setting the `permanent` attribute will not affect anything. It cannot override the permanence flag forcibly defined inside the plugin.

#### Attribute `sync`

A symbol can be executed "synchronously" or "asynchronously". In synchronous mode, the symbol code will be launched directly in the context of the current process - the klishd server. In asynchronous mode, a separate process will be forked to launch the symbol code. Launching a symbol in asynchronous mode is safer because errors in the code will not affect the klishd process. Using asynchronous mode is recommended.

Possible attribute values - `true` and `false`. By default `false`, i.e., the symbol will be executed asynchronously.

Symbols, when declared in a plugin, already have a synchronicity attribute. A symbol can be forcibly declared synchronous, asynchronous, or the plugin can leave the decision about synchronicity to the user. If the permanence flag is forcibly declared in the plugin, then setting the `sync` attribute will not affect anything. It cannot override the synchronicity flag forcibly defined inside the plugin.

#### Attribute `update_retcode`

One command can contain several `ACTION` elements. This is called an "action block". Each of the actions has its own return code. However, the command as a whole must also have a return code, and this code must be a single value, not an array.

By default, `ACTION` actions are executed sequentially, and as soon as one of the actions returns an error, the block execution stops, and the overall return code is considered an error. If no action in the block returned an error, the return code is considered to be the return code of the last action in the block.

Sometimes it is required that, regardless of the return code of a certain action, the block execution continues. For this, the `update_retcode` attribute can be used. The attribute can take the value `true` or `false`. By default, `true` is used. This means that the return code of the current action affects the overall return code. At this stage, the overall return code will be assigned the value of the current return code. If the flag value is set to `false`, then the current return code is ignored and will not affect the formation of the overall return code. Also, the execution of the action block will not be interrupted in case of an error at the stage of executing the current action.

#### Attribute `exec_on`

When executing an action block (several `ACTION` inside one element), actions are executed sequentially until all actions are executed, or until one of the actions returns an error. In such a case, block execution is interrupted. However, this behavior can be regulated by the `exec_on` attribute. The attribute can take the following values:

* `success` - the current action will be sent for execution if the value of the overall return code at the moment is "success".
* `fail` - the current action will be sent for execution if the value of the overall return code at the moment is "error".
* `always` - the current action will be executed regardless of the overall return code.
* `never` - the action will not be executed under any conditions.

By default, the value `success` is used, i.e., actions are executed if there were no errors before.

```
<ACTION sym="printl">1</ACTION>
<ACTION sym="printl" exec_on="never">2</ACTION>
<ACTION sym="printl">3</ACTION>
<ACTION sym="printl" exec_on="fail">4</ACTION>
<ACTION sym="script">/bin/false</ACTION>
<ACTION sym="printl">6</ACTION>
<ACTION sym="printl" exec_on="fail" update_retcode="false">7</ACTION>
<ACTION sym="printl" exec_on="always">8</ACTION>
<ACTION sym="printl" exec_on="fail">9</ACTION>
```

This example will output to the screen:

```
1
3
7
8
```

The string "1" will be output because at the beginning of the action block execution, the overall return code is assumed to be "success", and also the `exec_on` value by default is `success`.

The string "2" will not be output because `on_exec="never"`, i.e., do not execute under any conditions.

The string "3" will execute because the previous action (string "1") executed successfully.

The string "4" will not execute because the condition `on_exec="fail"` is set, and at this point the previous action "3" executed successfully and set the overall return code to "success".

The string "5" will execute and change the overall return code to "error".

The string "6" will not execute because the current overall return code is "error", and the string should only execute if the overall return code is successful.

The string "7" will output because the condition `on_exec="fail"` is set, and the current overall return code is indeed "error". Note that although the action itself executes successfully, the overall return code will not be changed because the attribute `update_retcode="false"` is used.

The string "8" will output because the condition `on_exec="always"` is set, which means execute the action regardless of the current overall return code.

The string "9" will not output because string "8" changed the overall return code to "success".

#### Data inside the tag

The data inside the `ACTION` tag is used at the discretion of the symbol itself, specified by the `sym` attribute. As an example, consider the `script` symbol from the `script` plugin. This symbol uses the data inside the tag as the script code that it must execute.

```
<COMMAND name="ls" help="List root dir">
	<ACTION sym="script">
	ls /
	</ACTION>
</COMMAND>

<COMMAND name="pytest" help="Test for Python script">
	<ACTION sym="script">#!/usr/bin/python
	import os
	print('ENV', os.getenv("KLISH_COMMAND"))
	</ACTION>
</COMMAND>
```

Note that in the "pytest" command, the shebang `#!/usr/bin/python` is specified, which indicates which interpreter should be used to execute the script.

### ENTRY

> Usually, the `ENTRY` tag is not used explicitly in configuration files.
> However, the tag is basic for most other tags, and most of its attributes are inherited.

If you look at the internal implementation of klish, you won't find all the multitude of tags available when writing XML configuration. Actually, there is a basic element `ENTRY` that implements the functions of most other tags. The element "turns into" other tags depending on the value of its attributes. The following tags are internally implemented as the `ENTRY` element:

* [`VIEW`](#view)
* [`COMMAND`](#command)
* [`FILTER`](#filter)
* [`PARAM`](#param)
* [`PTYPE`](#ptype)
* [`COND`](#cond)
* [`HELP`](#help)
* [`COMPL`](#compl)
* [`PROMPT`](#prompt)
* [`SWITCH`](#switch)
* [`SEQ`](#seq)

In this section, the attributes of the `ENTRY` element will be discussed in quite detail, often being attributes of other elements as well. Other elements will reference these descriptions in the `ENTRY` section. Configuration examples, when describing attributes, are not necessarily based on the `ENTRY` element but use other, more typical wrapper tags.

The basis of the `ENTRY` element is attributes that determine the features of its behavior and the ability to nest other `ENTRY` elements inside the `ENTRY` element. Thus, the entire configuration scheme is built.

#### Attribute `name`

The `name` attribute is the element identifier. Among elements of the current nesting level, the identifier must be unique. Elements with the same name can be present in different branches of the scheme. It is important that the absolute path of the element is unique, i.e., the combination of the element's own name and the names of all its "ancestors".

For the `COMMAND` tag, the attribute also serves as the value of the positional parameter if the `value` attribute is not defined. That is, the operator enters a string equal to the name of the `COMMAND` element to invoke this command.

#### Attribute `value`

If the command identifier (the `name` attribute) differs from the command name for the operator, then the `value` attribute contains the command name as it appears to the operator.

Used for the following tags:

* `COMMAND`
* `PARAM`
* `PTYPE`

```
<COMMAND name="cmdid" value="next"\>
```

In the example, the command identifier is "cmdid". It will be used if you need to create a reference to this element inside XML configs. But the user, to launch the command for execution, enters the text `next` in the command line.

#### Attribute `help`

When working with the command line, the operator can get a hint about possible commands, parameters, and their purpose. In the "klish" client, the hint will be shown when pressing the `?` key. The simplest way to set the hint text for an element is to specify the value of the `help` attribute.

The following tags support the `help` attribute:

* `COMMAND`
* `PARAM`
* `PTYPE`

The hint set using the `help` attribute is static. Another way to set a hint for an element is to create a nested `HELP` element. The `HELP` element generates the hint text dynamically.

```
<COMMAND name="simple" help="Command with static help"/>

<COMMAND name="dyn">
	<HELP>
		<ACTION sym="script">
		ls -la "/etc/passwd"
		</ACTION>
	<HELP>
</COMMAND>
```

If for an element both the `help` attribute and a nested `HELP` element are set, the dynamic nested `HELP` element will be used, and the attribute will be ignored.

The `PTYPE` element has its own hints. Both as a static attribute and as a dynamic element. These hints will be used for the `PARAM` parameter using this data type, in case neither the attribute nor the dynamic `HELP` element is set for the parameter.

#### Attribute `container`

A "container" in klish terms is an element that contains other elements but is not itself visible to the operator. That is, this element is not matched to any positional parameters when parsing the entered command line. It only organizes other elements. Examples of containers are `VIEW`, `SWITCH`, `SEQ`. The `VIEW` tag organizes commands into scopes, but the element itself is not a command or parameter for the operator. The operator cannot enter the name of this element in the command line; they can immediately access the elements nested in the container (if they themselves are not containers).

The `SWITCH` and `SEQ` elements are also invisible to the operator. They define how the elements nested within them should be processed.

The `container` attribute can take the values `true` or `false`. By default, `false` is used, i.e., the element is not a container. It should be noted that for all wrapper elements inherited from `ENTRY`, the attribute value is pre-set to the correct value. That is, using the attribute in real configs is usually not necessary. Only in specific cases is it really required.

#### Attribute `mode`

The `mode` attribute determines how nested elements will be processed when parsing the entered command line. Possible values:

* `sequence` - nested elements will be matched to "words" entered by the operator sequentially, one after another. Thus, all nested elements can receive values during parsing.
* `switch` - only one of the nested elements must be matched to the operator's input. This is a choice of one of many. Elements that were not chosen will not receive values.
* `empty` - the element cannot have nested elements.

So the `VIEW` element is forcibly given the attribute `mode="switch"`, assuming that it contains commands inside itself and it should not be possible to enter many commands at once in one line. The operator chooses only one command at the moment.

The `COMMAND` and `PARAM` elements by default have the setting `mode="sequence"`, since most often parameters that should be entered one after another are placed inside them. However, there is an opportunity to override the `mode` attribute in the `COMMAND`, `PARAM` tags.

The `SEQ` and `SWITCH` elements are containers and are created only to change the way nested elements are processed. The `SEQ` element has `mode="sequence"`, the `SWITCH` element has `mode="switch"`.

Below are examples of branching inside a command:

```
<!-- Example 1 -->
<COMMAND name="cmd1">
	<PARAM name="param1" ptype="/INT"/>
	<PARAM name="param2" ptype="/STRING"/>
</COMMAND>
```

The command by default has `mod="sequence"`, so the operator must enter both parameters, one after the other.

```
<!-- Example 2 -->
<COMMAND name="cmd2" mode="switch">
	<PARAM name="param1" ptype="/INT"/>
	<PARAM name="param2" ptype="/STRING"/>
</COMMAND>
```

The value of the `mode` attribute is overridden, so the operator must enter only one of the parameters. Which parameter the entered characters correspond to in this case will be determined as follows. First, the first parameter "param1" is checked for correctness. If the string matches the integer format, then the parameter takes its value, and the second parameter is not checked for match, although it would also fit, since the `STRING` type can contain numbers as well. Thus, when choosing one parameter from many, the order of specifying parameters is important.

```
<!-- Example 3 -->
<COMMAND name="cmd3">
	<SWITCH>
		<PARAM name="param1" ptype="/INT"/>
		<PARAM name="param2" ptype="/STRING"/>
	</SWITCH>
</COMMAND>
```

This example is identical to example "2". Only instead of the `mode` attribute, a nested `SWITCH` tag is used. The notation in example "2" is shorter; in example "3" it is more visual.

```
<!-- Example 4 -->
<COMMAND name="cmd4">
	<SWITCH>
		<PARAM name="param1" ptype="/INT">
			<PARAM name="param3" ptype="/STRING">
			<PARAM name="param4" ptype="/STRING">
		</PARAM>
		<PARAM name="param2" ptype="/STRING"/>
	</SWITCH>
	<PARAM name="param5" ptype="/STRING"/>
</COMMAND>
```

This demonstrates how command line arguments are parsed. If the parameter "param1" is selected, then its nested elements "param3" and "param4" are used, and then only the element "param5" following the `SWITCH`. The parameter "param2" is not used at all.

If "param2" was chosen first, then "param5" is processed, and the process ends there.

```
<!-- Example 5 -->
<COMMAND name="cmd5">
	<SWITCH>
		<SEQ>
			<PARAM name="param1" ptype="/INT">
			<PARAM name="param3" ptype="/STRING">
			<PARAM name="param4" ptype="/STRING">
		</SEQ>
		<PARAM name="param2" ptype="/STRING"/>
	</SWITCH>
	<PARAM name="param5" ptype="/STRING"/>
</COMMAND>
```

The example is completely analogous in behavior to example "4". Only instead of nesting, the `SEQ` tag is used, which says that if the first parameter from the block of sequential parameters was chosen, then the rest must also be entered one after the other.

```
<!-- Example 6 -->
<COMMAND name="cmd6">
	<COMMAND name="cmd6_1">
		<PARAM name="param3" ptype="/STRING">
	</COMMAND>
	<PARAM name="param1" ptype="/INT"/>
	<PARAM name="param2" ptype="/STRING"/>
</COMMAND>
```

The example demonstrates a nested "subcommand" named "cmd1_6". Here the subcommand is no different from a parameter, except that the criterion for matching command line arguments to the `COMMAND` command is that the entered argument matches the command name.

#### Attribute `purpose`

Some nested elements must have a special meaning. For example, inside a `VIEW` an element can be defined that generates the prompt text for the operator. To separate the element for generating the prompt from nested commands, it needs to be given a special attribute. Later, when the klishd server needs to get the user prompt for this `VIEW`, the code will look through the nested elements of the `VIEW` and select the element that is specifically intended for this.

The `purpose` attribute sets a special purpose for the element. Possible purposes:

* `common` - no special purpose. Ordinary tags have this attribute value.
* `ptype` - the element defines the type of the parent parameter. `PTYPE` tag.
* `prompt` - the element serves to generate the user prompt for the parent element. `PROMPT` tag. The parent element is `VIEW`.
* `cond` - the element checks the condition and, in case of failure, the parent element becomes unavailable to the operator. `COND` tag. Implemented in version 3.2.0.
* `completion` - the element generates possible auto-completions for the parent element. `COMPL` tag.
* `help` - the element generates a hint for the parent element. `HELP` tag.

Usually, the `purpose` attribute is not used directly in configuration files, since for each special purpose its own tag has been introduced, which is more visual.

#### Attribute `ref`

A scheme element can be a reference to another scheme element. Creating element "#1", which is a reference to element "#2", is equivalent to declaring element "#2" at the location in the scheme where element "#1" is located. More precisely, it is equivalent to creating a copy of element "#2" at the location where element "#1" is defined.

If an element is a reference, then the `ref` attribute is defined in it. The attribute value is a reference to the target element of the scheme.

```
<KLISH>

<PTYPE name="ptype1">
	<ACTION sym="INT"/>
</PTYPE>

<VIEW name="view1">
	<COMMAND name="cmd1"/>
</VIEW>

<VIEW name="view2">
	<VIEW ref="/view1"/>
	<COMMAND name="cmd2">
		<PARAM name="param1" ptype="/ptype1"/>
		<PARAM name="param2">
			<PTYPE ref="/ptype1"/>
		</PARAM>
	</COMMAND>
</VIEW>

<VIEW name="view3">
	<COMMAND ref="/view2/cmd2"/>
</VIEW>

<VIEW name="view4">
	<COMMAND name="do">
		<VIEW ref="/view1"/>
	</COMMAND>
</VIEW>

</KLISH>
```

In the example, "view2" contains a reference to "view1", which is equivalent to declaring a copy of "view1" inside "view2". This in turn means that the command "cmd1" will become available in "view2".

In another `VIEW` named "view3", a reference to the command "cmd2" is declared. Thus, a separate command becomes available inside "view3".

The parameters "param1" and "param2" have the same type `/ptype1`. The reference to the type can be written using the `ptype` attribute, or using a nested `PTYPE` tag, which is a reference to a previously declared `PTYPE`. As a result, the types of the two declared parameters are completely equivalent.

In the last example, the reference to `VIEW` is enclosed inside the `COMMAND` tag. In this case, it will mean that if we are working with "view4", then the commands from "view1" will be available with the prefix "do". That is, if the operator is in the `VIEW` named "view4", they must write `do cmd1` in the command line to execute the command "cmd1".

Using references, code reuse can be organized. For example, declare a block of "standard" parameters and then use references to insert this block into all commands where the parameters repeat.

#### Attribute `restore`

Assume the current path consists of several levels. That is, the operator has entered a nested `VIEW`. While in a nested `VIEW`, the operator has access to commands not only of the current `VIEW` but also of all parent ones. The operator enters a command defined in a parent `VIEW`. The `restore` attribute determines whether the current path will be changed before executing the command.

Possible values of the `restore` attribute - `true` or `false`. By default, `false` is used. This means that the command will be executed, and the current path will remain the same. If `restore="true"`, then before executing the command, the current path will be changed. The nested levels of the path will be removed to the level of the `VIEW` in which the entered command is defined.

The `restore` attribute is used in the `COMMAND` element.

Behavior with restoring the command's "native" path can be used in a system where the configuration mode is implemented according to the "Cisco" router principle. In such a mode, transitioning to one configuration section is possible without first exiting another - adjacent section. This requires first changing the current path, rising to the level of the entered command, and then immediately moving to the new section, but based on the current path corresponding to the command for entering the new section.

#### Attribute `transparent`

The `transparent` attribute determines whether elements from higher path levels will be available when parsing the entered string. This affects not only command search but also obtaining help and auto-completion.

The attribute is used in `ENTRY` and `VIEW` elements.

By default, all elements are "transparent", i.e., the search goes through all elements of the path. If at one of the path levels an element with `transparent="false"` is encountered, the search stops.

#### Attribute `order`

The `order` attribute determines whether the order is important when entering declared consecutive optional parameters. Suppose three consecutive optional parameters are declared. By default `order="false"` and this means that the operator can enter these three parameters in any order. Say, first the third, then the first, and then the second. If the second parameter has the flag `order="true"`, then having entered the second parameter first, the operator will no longer be able to enter the first after it. Only the third parameter will remain available.

The `order` attribute is used in the `PARAM` element.

#### Attribute `filter`

Some commands are filters. This means they cannot be used independently. Filters only process the output of other commands. Filters are specified after the main command and the separator symbol `|`. A filter cannot be used before the first `|` sign. At the same time, a command that is not a filter cannot be used after the `|` symbol.

The `filter` attribute can take the values `true` or `false`. By default, `filter="false"` is used. This means the command is not a filter. If `filter="true"`, the command is declared a filter.

The `filter` attribute is rarely used explicitly in configs. The `FILTER` tag has been introduced, which declares a command that is a filter. Apart from the specified usage restrictions, filters are no different from regular commands.

A typical example of a filter is the standard "grep" utility.

#### Attributes `min` and `max`

The `min` and `max` attributes are used in the `PARAM` element and determine how many arguments entered by the operator can be matched to the current parameter.

The `min` attribute determines the minimum number of arguments that must match the parameter, i.e., must pass the correctness check relative to the data type of this parameter. If `min="0"`, the parameter becomes optional. That is, if it is not entered, it is not an error. By default, `min="1"` is assumed.

The `max` attribute determines the maximum number of same-type arguments that can be matched to the parameter. If the operator enters more arguments than specified in the `max` attribute, these arguments will not be checked for match to the current parameter but may be checked for match to other parameters defined after the current one. By default, `max="1"` is assumed.

The `min="0"` attribute can be used in the `COMMAND` element to declare a subcommand optional.

```
<PARAM name="param1" ptype="/INT" min="0"/>
<PARAM name="param2" ptype="/INT" min="3" max="3"/>
<PARAM name="param3" ptype="/INT" min="2" max="5"/>
<COMMAND name="subcommand" min="0"/>
```

In the example, the parameter "param1" is declared optional. The parameter "param2" must correspond to exactly three arguments entered by the operator. The parameter "param3" can correspond to from two to five arguments. The subcommand "subcommand" is declared optional.

### VIEW

`VIEW`s are intended for organizing commands and other configuration elements into ["scopes"](#scopes). When the operator works with klish, there is a current session path. The elements of the path are `VIEW` elements. The current path can be changed using [navigation commands](#navigation). If the operator is in a nested `VIEW`, i.e., the current session path contains several levels, similar to nested directories in a file system, then the operator has access to all commands belonging not only to the `VIEW` at the very top level of the path but also to all "previous" `VIEW`s that make up the path.

You can create "shadow" `VIEW`s that never become elements of the current path. These `VIEW`s can be accessed by [references](#references-to-elements) and thus add their content to the location in the scheme where the reference is created.

`VIEW`s can be defined inside the following elements:

* `KLISH`
* `VIEW`
* `COMMAND`
* `PARAM`

#### Attributes of the `VIEW` element

* [`name`](#attribute-name) - element identifier.
* [`help`](#attribute-help) - element description.
* [`ref`](#attribute-ref) - reference to another `VIEW`.

#### Examples

```
<VIEW name="view1">
	<COMMAND name="cmd1"/>
	<VIEW name="view1_2">
		<COMMAND name="cmd2"/>
	</VIEW>
</VIEW>

<VIEW name="view2">
	<COMMAND name="cmd3"/>
	<VIEW ref="/view1"/>
</VIEW>

<VIEW name="view3">
	<COMMAND name="cmd4"/>
	<VIEW ref="/view1/view1_2"/>
</VIEW>

<VIEW name="view4">
	<COMMAND name="cmd5"/>
</VIEW>
```

The example demonstrates how scopes work relative to available operator commands.

If the current session path is `/view1`, then the operator has access to commands "cmd1" and "cmd2".

If the current session path is `/view2`, then the operator has access to commands "cmd1", "cmd2", "cmd3".

If the current session path is `/view3`, then the operator has access to commands "cmd2" and "cmd4".

If the current session path is `/view4`, then the operator has access to command "cmd5".

If the current session path is `/view4/view1`, then the operator has access to commands "cmd1", "cmd2", "cmd5".

If the current session path is `/view4/{/view1/view1_2}`, then the operator has access to commands "cmd2", "cmd5". Here, curly braces denote the fact that any `VIEW` can be a path element, and it does not matter whether it is nested when defining the scheme. Inside the curly braces is the unique path of the element in the scheme. The section ["References to Elements"](#references-to-elements) explains the difference between the path that uniquely identifies an element within the scheme and the current session path.

### COMMAND

The `COMMAND` tag declares a command. Such a command can be executed by the operator. The [`name`](#attribute-name) attribute is used to match the argument entered by the operator with the command. If it is required that the command identifier inside the scheme differs from the command name as it appears to the operator, then `name` will contain the internal identifier, and the [`value`](#attribute-value) attribute will contain the "user" name of the command. The `name` and `value` attributes can contain only one word, without spaces.

A command is little different from the [`PARAM`](#param) element. A command can contain subcommands. That is, inside one command, another command can be declared, which is actually a parameter of the first command, only this parameter is identified by a fixed text string.

A typical command contains a hint [`help`](#attribute-help) or [`HELP`](#help) and, if necessary, a set of nested parameters [`PARAM`](#param). Also, the command should specify the actions [`ACTION`](#action) that it performs.

#### Attributes of the `COMMAND` element

* [`name`](#attribute-name) - element identifier.
* [`value`](#attribute-value) - "user" name of the command.
* [`help`](#attribute-help) - element description.
* [`mode`](#attribute-mode) - mode of processing nested elements.
* [`min`](#attributes-min-and-max) - minimum number of command line arguments matched to the command name.
* [`max`](#attributes-min-and-max) - maximum number of command line arguments matched to the command name.
* [`restore`](#attribute-restore) - flag to restore the command's "native" level in the current session path.
* [`ref`](#attribute-ref) - reference to another `COMMAND`.

#### Examples

```
<PTYPE name="ptype1">
	<ACTION sym="INT"/>
</PTYPE>

<VIEW name="view1">

	<COMMAND name="cmd1" help="First command">
		<PARAM name="param1" ptype="/ptype1"/>
		<ACTION sym="sym1"/>
	</COMMAND>

	<COMMAND name="cmd2">
		<HELP>
			<ACTION sym="script">
			echo "Second command"
			</ACTION>
		</HELP>
		<COMMAND name="cmd2_1" value="sub2" min="0" help="Subcommand">
			<PARAM name="param1" ptype="ptype1" help="Par 1"/>
		</COMMAND>
		<COMMAND ref="/view1/cmd1"/>
		<PARAM name="param2" ptype="ptype1" help="Par 2"/>
		<ACTION sym="sym2"/>
	</COMMAND>

</VIEW>
```

Command "cmd1" is the simplest variant of a command with a hint, one mandatory parameter of type "ptype1", and an executed action.

Command "cmd2" is more complex. The hint is generated dynamically. The first parameter is an optional subcommand with a "user" name "sub2" and one mandatory nested parameter. That is, the operator, wanting to use the subcommand, must start their command line like this: `cmd2 sub2 ...`. If the optional subcommand "cmd2_1" is used, the operator must specify the value of its mandatory parameter. The second subcommand is a reference to another command. To understand what this means, it is enough to imagine that at this location the command pointed to by the reference is fully described, i.e., "cmd1". After the subcommands comes a mandatory numeric parameter and the action performed by the command.

### FILTER

A filter is a [`COMMAND`](#command) with the difference that a filtering command cannot be used independently. It only processes the output of other commands and can be used after the main command and the separating symbol `|`. The use of filters is described in more detail in the ["Filters"](#filters) section.

For the `FILTER` tag, the [`filter`](#attribute-filter) attribute is forcibly set to `true`. The remaining attributes and features of operation coincide with the [`COMMAND`](#command) element.

### PARAM

The `PARAM` element describes a command parameter. The parameter has a type, which is set either by the `ptype` attribute or by a nested [`PTYPE`](#ptype) element. When the operator enters an argument, its value is checked for correctness by the code of the corresponding `PTYPE`.

In general, the value for a parameter can be either a string without spaces or a string with spaces enclosed in quotes.

#### Attributes of the `PARAM` element

* [`name`](#attribute-name) - element identifier.
* [`value`](#attribute-value) - arbitrary value that can be analyzed by the `PTYPE` code. For the `COMMAND` element, which is a special case of a parameter, this field is used as the name of the "user" command.
* [`help`](#attribute-help) - element description.
* [`mode`](#attribute-mode) - mode of processing nested elements.
* [`min`](#attributes-min-and-max) - minimum number of command line arguments matched to the parameter.
* [`max`](#attributes-min-and-max) - maximum number of command line arguments matched to the parameter.
* [`order`](#attribute-order) - mode of processing optional parameters.
* [`ref`](#attribute-ref) - reference to another `COMMAND`.
* `ptype` - reference to the parameter type.

#### Examples

```
<PTYPE name="ptype1">
	<ACTION sym="INT"/>
</PTYPE>

<VIEW name="view1">

	<COMMAND name="cmd1" help="First command">

		<PARAM name="param1" ptype="/ptype1" help="Param 1"/>

		<PARAM name="param2" help="Param 2">
			<PTYPE ref="/ptype1"/>
		</PARAM>

		<PARAM name="param3" help="Param 3">
			<PTYPE>
				<ACTION sym="INT"/>
			</PTYPE>
		</PARAM>

		<PARAM name="param4" ptype="/ptype1" help="Param 4">
			<PARAM name="param5" ptype="/ptype1" help="Param 5"/>
		</PARAM>

		<ACTION sym="sym1"/>
	</COMMAND>

</VIEW>
```

Parameters "param1", "param2", "param3" are identical. In the first case, the type is set by the `ptype` attribute. In the second - by a nested `PTYPE` element, which is a reference to the same type "ptype1". In the third - the parameter type is defined "on the spot", i.e., a new `PTYPE` type is created. But in all three cases, the type is an integer (see the `sym` attribute). Defining the type directly inside the parameter can be convenient if such a type is not needed anywhere else.

Parameter "param4" has a nested parameter "param5". After entering an argument for parameter "param4", the operator must enter an argument for the nested parameter.

### SWITCH

The `SWITCH` element is a [container](#attribute-container). Its only task is to set the processing mode for nested elements. `SWITCH` sets such a processing mode for nested elements that only one element must be selected from many.

In the `SWITCH` element, the [`mode`](#attribute-mode) attribute is forcibly set to `switch`.

Additional information about processing modes for nested elements can be found in the ["Attribute `mode`"](#attribute-mode) section.

#### Attributes of the `SWITCH` element

* [`name`](#attribute-name) - element identifier.
* [`help`](#attribute-help) - element description.

Usually, the `SWITCH` element is used without attributes.

#### Examples

```
<COMMAND name="cmd1" help="First command">
	<SWITCH>
		<COMMAND name="sub1"/>
		<COMMAND name="sub2"/>
		<COMMAND name="sub3"/>
	</SWITCH>
</COMMAND>
```

By default, the `COMMAND` element has the attribute `mode="sequence"`. If there were no `SWITCH` element in the example, the operator would have to sequentially set all subcommands one after another: `cmd1 sub1 sub2 sub3`. The `SWITCH` element changed the processing mode of nested elements, and as a result, the operator must choose only one subcommand out of three. For example `cmd1 sub2`.

### SEQ

The `SEQ` element is a [container](#attribute-container). Its only task is to set the processing mode for nested elements. `SEQ` sets such a processing mode for nested elements that all nested elements must be set one after another.

In the `SEQ` element, the [`mode`](#attribute-mode) attribute is forcibly set to `sequence`.

Additional information about processing modes for nested elements can be found in the ["Attribute `mode`"](#attribute-mode) section.

#### Attributes of the `SWITCH` element

* [`name`](#attribute-name) - element identifier.
* [`help`](#attribute-help) - element description.

Usually, the `SEQ` element is used without attributes.

#### Examples

```
<VIEW name="view1">
	<SEQ>
		<PARAM name="param1" ptype="/ptype1"/>
		<PARAM name="param2" ptype="/ptype1"/>
		<PARAM name="param3" ptype="/ptype1"/>
	</SEQ>
</VIEW>

<VIEW name="view2">
	<COMMAND name="cmd1" help="First command">
		<VIEW ref="/view1">
	</COMMAND>
</VIEW>
```

Suppose we created an auxiliary `VIEW` containing a list of frequently used parameters and named it "view1". All parameters should be used sequentially, one after another. Then in another `VIEW` we declared a command that should contain all these parameters. For this, inside the command, a reference to "view1" is created, and all parameters "fall" inside the command. However, the `VIEW` element by default has the attribute `mode="switch"`, and it turns out that the operator will not enter all declared parameters but must choose only one of them. To change the parsing order of nested parameters, the `SEQ` element is used. It changes the parameter parsing order to sequential.

The same result could be achieved by adding the attribute `mode="sequence"` to the declaration of "view1". Using the attribute is shorter; using the `SEQ` element is more visual.

### PTYPE

The `PTYPE` element describes a data type for parameters. Parameters [`PARAM`](#param) reference a data type using the `ptype` attribute or contain a nested `PTYPE` element. The task of `PTYPE` is to check the argument passed by the operator for correctness and return the result as "success" or "error". More precisely, the result of the check can be expressed as "fits" or "does not fit". Inside the `PTYPE` element, actions [`ACTION`](#action) are specified that perform the check of the argument for compliance with the declared data type.

#### Attributes of the `PTYPE` element

* [`name`](#attribute-name) - element identifier.
* [`help`](#attribute-help) - element description.
* [`ref`](#attribute-ref) - reference to another `PTYPE`.

#### Examples

```
<PTYPE name="ptype1" help="Integer">
	<ACTION sym="INT"/>
</PTYPE>

<PARAM name="param1" ptype="/ptype1" help="Param 1"/>
```

The example declares a data type "ptype1". This is an integer, and the `INT` symbol from the standard "klish" plugin checks that the entered argument is indeed an integer.

Other variants of using the `PTYPE` tag and the `ptype` attribute are considered in the examples section of the [`PARAM`](#param) element.

### PROMPT

The `PROMPT` element has a special purpose and is nested for the `VIEW` element. The purpose of the element is to form the prompt for the user if the parent `VIEW` is the current session path. If the current path is multi-level, then not finding the `PROMPT` element in the last element of the path, the search mechanism will rise one level up and look for `PROMPT` there. And so on up to the root element. If `PROMPT` is not found there either, then the standard prompt at the discretion of the klish client is used. By default, the klish client uses the prompt `$`.

Inside the `PROMPT` element, actions [`ACTION`](#action) are specified that form the prompt text for the user.

The `PROMPT` element is forcibly assigned the value of the attribute `purpose="prompt"`. Also, `PROMPT` is a container.

#### Attributes of the `PROMPT` element

* [`name`](#attribute-name) - element identifier.
* [`help`](#attribute-help) - element description.
* [`ref`](#attribute-ref) - reference to another `PROMPT`.

Usually, `PROMPT` is used without attributes.

#### Examples

```
<VIEW name="main">

	<PROMPT>
		<ACTION sym="prompt">%u@%h&gt; </ACTION>
	</PROMPT>

</VIEW>
```

In the example, for the `VIEW` named "main", which is the current path by default when klish starts, the user prompt is defined. In `ACTION`, the `prompt` symbol from the standard "klish" plugin is used, which helps in forming the prompt by replacing constructs like `%u` or `%h` with substitutions. In particular, `%u` is replaced by the current user's name, and `%h` by the host name.

### HELP

The `HELP` element has a special purpose and is nested for the elements `COMMAND`, `PARAM`, `PTYPE`. The purpose of the element is to form the "help" text, i.e., a hint for the parent element. Inside the `HELP` element, actions [`ACTION`](#action) are specified that form the hint text.

The `HELP` element is forcibly assigned the value of the attribute `purpose="help"`. Also, `HELP` is a container.

The klish client shows hints when pressing the `?` key.

#### Attributes of the `HELP` element

* [`name`](#attribute-name) - element identifier.
* [`help`](#attribute-help) - element description.
* [`ref`](#attribute-ref) - reference to another `HELP`.

Usually, `HELP` is used without attributes.

#### Examples

```
<PARAM name="param1" ptype="/ptype1">
	<HELP>
		<ACTION sym="sym1"/>
	</HELP>
</PARAM>
```

### COMPL

The `COMPL` element has a special purpose and is nested for the elements `PARAM`, `PTYPE`. The purpose of the element is to form auto-completion options, i.e., possible value variants for the parent element. Inside the `COMPL` element, actions [`ACTION`](#action) are specified that form the output. Separate variants in the output are separated from each other by a newline.

The `COMPL` element is forcibly assigned the value of the attribute `purpose="completion"`. Also, `COMPL` is a container.

The klish client shows auto-completion options when pressing the `Tab` key.

#### Attributes of the `COMPL` element

* [`name`](#attribute-name) - element identifier.
* [`help`](#attribute-help) - element description.
* [`ref`](#attribute-ref) - reference to another `COMPL`.

#### Examples

```
<PARAM name="proto" ptype="/STRING" help="Protocol">
	<COMPL>
		<ACTION sym="printl">tcp</ACTION>
		<ACTION sym="printl">udp</ACTION>
		<ACTION sym="printl">icmp</ACTION>
	</COMPL>
</PARAM>
```

The parameter specifying the protocol has the type `/STRING`, i.e., an arbitrary string. The operator can enter arbitrary text, but for convenience, the parameter has auto-completion options.

### COND

> The `COND` element is implemented in version 3.2.0.

The `COND` element has a special purpose and is nested for the elements `VIEW`, `COMMAND`, `PARAM`. The purpose of the element is to hide the element from the operator in case the condition specified in `COND` is not met. Inside the `COND` element, actions [`ACTION`](#action) are specified that check the condition.

The `COND` element is forcibly assigned the value of the attribute `purpose="cond"`. Also, `COND` is a container.

#### Attributes of the `COND` element

* [`name`](#attribute-name) - element identifier.
* [`help`](#attribute-help) - element description.
* [`ref`](#attribute-ref) - reference to another `COND`.

#### Examples

```
<COMMAND name="cmd1" help="Command 1">
	<COND>
		<ACTION sym="script">
		test -e /tmp/cond_file
		</ACTION>
	</COND>
</COMMAND>
```

If the file `/tmp/cond_file` exists, the command "cmd1" is available to the operator; if it does not exist, the command is hidden.

### LOG

The `LOG` element sets the method and area of logging. Inside the `LOG` element, actions [`ACTION`](#action) are specified that will be launched after the user command completes. These actions have access to the command's completion code. How logging is implemented is determined by the code of the `ACTION` element of `LOG`.

The `LOG` element itself can occur inside the elements [`KLISH`](#klish) (i.e., in the global space), [`VIEW`](#view), [`COMMAND`](#command). When executing a command, the `LOG` element closest to the command definition will be used. This means that if `LOG` is defined inside the `COMMAND` element, it will be selected. If the `LOG` element is not defined inside the command, the search will go up the hierarchy. That is, first it will be performed in the `VIEW` element inside which the command is declared, and if `LOG` is not found there, then in the global space.

The absence of the `LOG` element in the configuration file is not an error. In this case, logging simply will not be performed. At one hierarchy level, more than one `LOG` element cannot be defined.

The flexibility of the `LOG` element and the possibilities of its placement allow logging in various ways for different commands or groups of commands.

#### Examples

```
<LOG>
  <ACTION sym="syslog"/>
</LOG>

<VIEW name="main">

  <COMMAND name="cmd1" help="Command 1">
    <ACTION sym="script">
    ls
    </ACTION>
  </COMMAND>

</VIEW>
```

The fact of executing the command "cmd1" will be recorded in syslog. The `syslog` symbol is defined in the standard "klish" plugin.

## Plugin "klish"

The klish source code tree includes the code of the standard plugin "klish". The plugin contains basic data types, a navigation command, and other auxiliary symbols. In the vast majority of cases, this plugin should be used. However, it is not connected automatically, as in some rare specific cases, the ability to work without it may be needed.

The standard way to connect the "klish" plugin in configuration files:

```
<PLUGIN name="klish"/>
```

Along with the plugin comes the file `ptypes.xml`, where basic data types are declared in the form of [`PTYPE`](#ptype) elements. The declared data types use plugin symbols to check the argument's compliance with the data type.

### Data Types

All symbols in this section are intended for use in `PTYPE` elements, unless otherwise specified, and check the compliance of the entered argument with the data type.

#### Symbol `COMMAND`

The `COMMAND` symbol checks that the entered argument matches the command name. That is, with the `name` or `value` attributes of the `COMMAND` or `PARAM` elements. If the `value` attribute is set, its value is used as the command name. If not set, the value of the `name` attribute is used. The case of characters in the command name is not taken into account.

Sometimes it’s convenient to have shortcuts for commands so you don’t have to
type the full names. For example, for a command with the full name
`environment`, you can define allowed abbreviations, and then entering `env`
will be enough to invoke it. To define allowed abbreviations, the `value`
attribute is used (it won’t work with the `name` attribute). In the attribute’s
value, the mandatory prefix of the command is separated by the `|` symbol from
the optional part. For example:

```
<COMMAND name="environment" value="env|ironment" />
```

For such a command, you can use abbreviations like `env`, `envi`, `envir`, and
so on. However, strings like `e` and `en` won’t work because they are shorter
than the minimum required prefix `env`.


#### Symbol `completion_COMMAND`

The `completion_COMMAND` symbol is intended for the `COMPL` element nested in `PTYPE`. The symbol is used to form an auto-completion string for a command. The auto-completion for a command is the name of the command itself. If the `value` attribute is set, its value is used as the command name. If not set, the value of the `name` attribute is used.

#### Symbol `help_COMMAND`

The `help_COMMAND` symbol is intended for the `HELP` element nested in `PTYPE`. The symbol is used to form a description (help) string for a command. If the `value` attribute is set, its value is used as the command name. If not set, the value of the `name` attribute is used. The hint itself is the value of the command's `help` attribute.

#### Symbol `COMMAND_CASE`

The `COMMAND_CASE` symbol is completely analogous to the [`COMMAND`](#symbol-command) symbol, except that it takes into account the case of characters in the command name.

#### Symbol `INT`

The `INT` symbol checks that the entered argument is an integer. The bit depth of the number corresponds to the `long long int` type in the C language. Inside the [`ACTION`](#action) element, a valid range for the integer can be defined. The minimum and maximum values are specified, separated by a space.

```
<ACTION sym="INT">-30 80</ACTION>
```

The number can take values in the range from "-30" to "80".

#### Symbol `UINT`

The `UINT` symbol checks that the entered argument is an unsigned integer. The bit depth of the number corresponds to the `unsigned long long int` type in the C language. Inside the [`ACTION`](#action) element, a valid range for the number can be defined. The minimum and maximum values are specified, separated by a space.

```
<ACTION sym="UINT">30 80</ACTION>
```

The number can take values in the range from "30" to "80".

#### Symbol `STRING`

The `STRING` symbol checks that the entered argument is a string. Currently, there are no specific requirements for strings.

### Navigation

Using navigation commands, the operator changes the current session path.

#### Symbol `nav`

The `nav` symbol is intended for navigation. Using the subcommands of the `nav` symbol, you can change the current session path. All subcommands of the `nav` symbol are specified inside the `ACTION` element - each command on a separate line.

Subcommands of the `nav` symbol:

* `push <view>` - enter the specified `VIEW`. One more level of nesting is added to the current session path.
* `pop [num]` - exit the specified number of nesting levels. That is, exclude `num` top levels from the current session path. The `num` argument is optional. By default `num=1`. If we are already in the root `VIEW`, i.e., the current path contains only one level, then `pop` will end the session and exit klish.
* `top` - go to the root level of the current session path. That is, exit all nested `VIEW`s.
* `replace <view>` - replace the `VIEW` at the current nesting level with the specified `VIEW`. The number of nesting levels does not increase. Only the last component of the path changes.
* `exit` - end the session and exit klish.

```
<ACTION sym="nav">
pop
push /view_name1
</ACTION>
```

The example shows how you can repeat the `replace` subcommand using other subcommands.

### Auxiliary Functions

#### Symbol `nop`

Empty command. The symbol does nothing. Always returns the value `0` - "success".

#### Symbol `print`

Prints the text specified in the body of the `ACTION` element. A newline is not output at the end of the text.

```
<ACTION sym="print">Text to print</ACTION>
```

#### Symbol `printl`

Prints the text specified in the body of the `ACTION` element. A newline is added at the end of the text.

```
<ACTION sym="printl">Text to print</ACTION>
```

#### Symbol `pwd`

Prints the current session path. Needed mainly for debugging.

#### Symbol `prompt`

The `prompt` symbol helps form the prompt text for the operator. In the body of the `ACTION` element, the prompt text is specified, which can contain substitutions of the form `%s`, where "s" is some printable character. Instead of such a construct, a specific string is substituted into the text. The list of implemented substitutions:

* `%%` - the `%` character itself.
* `%h` - host name.
* `%u` - current user name.

```
<ACTION sym="prompt">%u@%h&gt; </ACTION>
```

## Plugin "script"

The "script" plugin contains only one symbol `script` and serves to execute scripts. The script is contained in the body of the `ACTION` element. The script can be written in different scripting programming languages. By default, it is assumed that the script is written for the shell interpreter and is launched using `/bin/sh`. To choose another interpreter, a "shebang" is used. A shebang is text of the form `#!/path/to/binary`, located in the very first line of the script. The text `/path/to/binary` is the path where the script interpreter is located.

The `script` plugin is part of the klish project source code, and the plugin can be connected as follows:

```
<PLUGIN name="script"/>
```

The name of the executed command and parameter values are passed to the script using environment variables. The following environment variables are supported:

* `KLISH_COMMAND` - name of the executed command.
* `KLISH_PARAM_<name>` - value of the parameter with the name `<name>`.
* `KLISH_PARAM_<name>_<index>` - one parameter can have many values if the `max` attribute is set for it and the value of this attribute is greater than one. Then the values can be obtained by index.

Examples:

```
<COMMAND name="ls" help="List path">
	<PARAM name="path" ptype="/STRING" help="Path"/>
	<ACTION sym="script">
	echo "$KLISH_COMMAND"
	ls "$KLISH_PARAM_path"
	</ACTION>
</COMMAND>

<COMMAND name="pytest" help="Test for Python script">
	<ACTION sym="script">#!/usr/bin/python
	import os
	print('ENV', os.getenv("KLISH_COMMAND"))
	</ACTION>
</COMMAND>
```

The "ls" command uses the shell interpreter and outputs to the screen a list of files along the path specified in the "path" parameter. Note that using the shell can be unsafe due to the potential possibility for the operator to inject arbitrary text into the script and, accordingly, execute an arbitrary command. Using the shell is available but not recommended. It is very difficult to write a safe shell script.

The "pytest" command executes a script in the Python language. Note where the shebang is defined. The first line of the script is the line immediately following the `ACTION` element. The line following the line in which `ACTION` is declared is already considered the second, and defining a shebang in it is not allowed.

## Plugin "lua"

The "lua" plugin contains only one symbol `lua` and serves to execute scripts in the "Lua" language. The script is contained in the body of the `ACTION` element. Unlike the `script` symbol from the ["script"](#plugin-script) plugin, the `lua` symbol does not call an external program-interpreter to execute scripts but uses internal mechanisms for this.

The `lua` plugin is part of the klish project source code, and the plugin can be connected as follows:

```
<PLUGIN name="lua"/>
```

The tag content can set the configuration.

### Configuration Parameters

Let's consider the configuration parameters of the plugin.

#### autostart

```
autostart="/usr/share/lua/klish/autostart.lua"
```

When the plugin is initialized, a Lua machine state is created which (after fork) will be used to call Lua `ACTION` code. If it is necessary to load any libraries, this can be done by setting an autostart file. There can be only one parameter.

#### package.path

```
package.path="/usr/lib/lua/clish/?.lua;/usr/lib/lua/?.lua"
```

Sets the Lua package.path (paths along which module search goes). There can be only one parameter.

#### backtrace

```
backtrace=1
```

Whether to show backtrace on Lua code crashes. 0 or 1.

### API

When executing Lua `ACTION`, the following functions are available:

#### klish.pars()

Returns information about parameters. There are two possible ways to use this function.

```
local pars = klish.pars()
for k, v in ipairs(pars) do
  for i, p in ipairs(pars[v]) do
    print(string.format("%s[%d] = %s", v, i, p))
  end
end
```

Getting information about all parameters. In this case, the function is called without arguments and returns a table of all parameters. The table contains both a list of names and an array of values for each name. The example above shows an iterator over all parameters with output of their values.

Additionally, klish.pars can be called to get the values of a specific parameter, for example:

```
print("int_val = ", klish.pars('int_val')[1])
```

#### klish.ppars()

Works exactly the same as `klish.ppars()`, but only for parent parameters, if they exist in this context.

#### klish.par() and klish.ppar()

Work the same as `klish.pars()`, `klish.ppars()` with specifying a specific parameter, but return not an array but a value. If there are several parameters with that name, the first one is returned.

#### klish.path()

Returns the current path as an array of strings. For example:

```
print(table.concat(klish.path(), "/"))
```

#### klish.context()

Allows getting some parameters of the command context. Takes a string as input -- the name of the context parameter:

- val;
- cmd;
- pcmd;
- uid;
- pid;
- user.

Without a parameter, returns a table with all available context parameters.
