# FS-Search
This is a small command line utility to seach through a directory tree recursively in search for a file who's contents match a supplied pattern.
I made this utility after having a hard time searching through a directory tree for a file that I didn't know the name of, but I knew the contents of.

This project is tiny, anyone can compile it if they so choose, though Windows bianries are still avalible.
If you want to compile it yourself, make sure you have a compiler that supports the C++17 library, as well as the experimental filesystem library, which this project makes heavy use of. If you have Clang, and C++17 along with it, then running the makefile is all you really need to do to compile it.

## Usage
The command follows a relatively simple syntax. It has one required argument, that being the pattern of the content it should search for, and then option arguments to narrow down the search, or to display more information.
The `-cp` _(content pattern)_ being said required argument. Every argument following `-cp` will be treated as a pattern, as such, `-cp` should always be placed after every other arugment, otherwise the arguments will be interpreted as patterns.

The `-fp` argument is for specifying filename patterns. Any file that does not match one or more filename pattern will be ignored. The filename pattern includes the absolute path, so the names of all of the parent directories can match a pattern just like a file name can. 
You can specify multiple filename patterns by separating them with a colon `:`, like this `-fp .cpp:.h` 

The `-d` argument specifies target directory where the search should be done from. If not specified, this is the current working directory. 
If the directory contains spaces in the name and you must surround it in quotes, make sure that there is no trailing `\` at the end of the path, otherwise that will escape the quote.
E.g. *DON'T** do this: `-d "C:\Program Files\"` instead do this `-d "C:\Program Files"`. 

The `-ms` argument specifies the maximum allowed size of a file. Even if a file matches all the prior conditions, it will be skipped if it exceeds this size. 
The size itself is specified in bytes, e.g. for 10KB you'd do `-ms 10000`

The `-mc` argument specifies the maximum number of files to seach through before breaking the operation. This only includes actual read operations. If previous conditions weren't met, and the file was never opened, it does not count towards this limit.
E.g. `-mc 1300`

The `-v` argument enables verbose mode, which gives you a list of every file that failed due to exceeding the maximum file size, or files that failed due to a stream error, or another unknown exception.

The `-dbg` argument enabled debug mode, which displays how the program interpreted the command line arguments.

If you don't specify a content pattern via `-cp` then it will display a reference.


## Sample Output

This searches through the source directory for any files with .cxx or .hxx in the name, that are less than 100KB, and contain either `include` `int` or `const` inside of them.
You can see here `{include, int, const} | Matched in > .\source\main.cxx` that all patterns that are matched are noted at the beggining. 

`fsearch.exe -ms 100000 -fp .cxx:.hxx -d .\source -cp include int const`

```
1 / 1 | 100%

The following files matched one or more patterns.
==================================================
{include, int, const} | Matched in > .\source\main.cxx
==================================================

Searched through 1 file(s) for 3 pattern(s).
Skipped 0 file(s) due to excess size.
Skipped 0 file(s) due to exception(s).
Skipped 0 file(s) due to stream failure(s).

Matched 1 file(s).

```

