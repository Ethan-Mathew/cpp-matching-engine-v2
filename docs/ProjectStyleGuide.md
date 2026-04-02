# Project Style Guide

I thought it would be prudent to summarize the style choices made while moving forward with this project. It's meant to serve as a tracker for the style choices, giving me something to refer back to as I progress and add new features. I try to include one of these in each of my personal projects, not just to solidify the standard used during project-creation, but also to monitor how I change my style as I become more experienced.

## Includes

I've been using the following hierarchy to order my library/header inclusions:

1. Inclusion of external libraries
2. Includes from include/
3. Includes from the local directory
4. Includes from the STL

## Blocks & Braces

For the sake of readability, I prefer to place both the opening and closing braces of function, class, struct, conditional, and enumeration bodies on separate lines. This has been my preference for some time, though I may eventually transition to using inline opening braces because of their undeniable popularity.

## Text Style

Also for the sake of readability, I tend to camel case where I can. Snake casing is a little unattractive, so I usually try to keep it to a minimum. The rules I used in this project were as follows:

- All file names use pascal casing, except those with standardized names (e.g. README or CMakeLists).
- Directory names are purely lowercase unless automatically generated (e.g. build files by CMake).
- Namespaces (e.g. lob and core) are purely lowercase.
- Class names use pascal case.
  - The member initializer list is tabbed in once, with left-justified colon and commas.
- Variable/member data names are camel case.
  - Member data names are suffixed with an underscore.
- Member function names are snake case to cleanly differentiate from their associated objects.
  - Function parameters are camel case, like regular variables.

## Classes & Structs

- Classes receive their public declarations first, then followed by private members.
- Member functions (apart from the special member functions) are declared in snake-case.

## Enumerations

- Enumerated types are in all-caps.
- I'll prefer scoped enums.
