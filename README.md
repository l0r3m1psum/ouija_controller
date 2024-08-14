# Grbl interface for Ouja board

After some reverse engineering with the amazing [Free Serial
Analyzer](https://freeserialanalyzer.com), I have discovered that the firmware
on the Ouja board probably used [Grbl](https://github.com/grbl/grbl) to drive
the motors. Below there are some useful references:

  * [Jogging](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Jogging)
  * [Commands](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Commands)
  * [Interface](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Interface)

There are already some off the shelf applications that drive Grbl boards, like
[OpenBuilds CONTROL](https://openbuildspartstore.com/openbuilds-control/) which
is [open source](https://github.com/OpenBuilds/OpenBuilds-CONTROL), and can be
a good source of examples. The company that makes CONTROL also has a forum with
Grbl threads like:

  * [prompt grbl for position](https://openbuilds.com/threads/14836/)
  * [how to check the status of a grbl
	command](https://openbuilds.com/threads/18417/)

A possible way to test this software is to use
[com0com](https://com0com.sourceforge.net), which is a [null
modem](https://en.wikipedia.org/wiki/Null_modem) emulator, attaching on the
other hand of the vurtual COM port an implementation of a Grbl compatible
software that can run on Windows, like
[uCNC](https://github.com/Paciente8159/uCNC/blob/master/makefiles/virtual/makefile).

com0com can also be used as an open source alternative to Free Serial Analyzer
using [hub4com](https://com0com.sourceforge.net/hub4com/ReadMe.txt).

There is a [FAQ on
com0com](https://www.magsys.co.uk/comcap/onlinehelp/null_modem_emulator_com0com.htm)
and a good samaritan has also made [signed drivers for
com0com](https://pete.akeo.ie/2011/07/com0com-signed-drivers.html).

## Windows' stuff

[Integer types](https://learn.microsoft.com/en-us/windows/win32/learnwin32/windows-coding-conventions)
[String types](https://learn.microsoft.com/en-us/windows/win32/learnwin32/working-with-strings)

[Hungarian notation](https://learn.microsoft.com/en-us/windows/win32/stg/coding-style-conventions)
[Win32 windows basic](https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window)
